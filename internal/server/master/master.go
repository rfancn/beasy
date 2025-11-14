package master

import (
	"context"
	"fmt"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"github.com/hdget/sdk"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/internal/server"
	"github.com/rfancn/beasy/pkg/event"
	"github.com/rfancn/beasy/pkg/filewatch"
	pkgsync "github.com/rfancn/beasy/pkg/sync"
)

type masterServerImpl struct {
	watcher     filewatch.FileWatcher
	eventServer *event.Server
	syncer      pkgsync.Syncer
	wg          sync.WaitGroup
	ctx         context.Context
	cancel      context.CancelFunc
}

func New() server.Server {
	return &masterServerImpl{}
}

func (m *masterServerImpl) Run() error {
	// 创建上下文用于控制生命周期
	m.ctx, m.cancel = context.WithCancel(context.Background())

	// 文件监控
	{
		fileWatcher, err := filewatch.New(
			filewatch.WithOnChange(m.handleFileChanges),
		)
		if err != nil {
			return fmt.Errorf("创建监控器失败: %v", err)
		}

		m.watcher = fileWatcher
		go m.watcher.Run()

		if g.Debug {
			sdk.Logger().Debug("文件监控初始化完成")
		}
	}

	{ // 启动事件服务器
		m.eventServer = event.NewServer()
		go m.eventServer.Run()

		if g.Debug {
			sdk.Logger().Debug("消息服务器初始化完成")
		}
	}

	// 处理信号
	m.handleSignals()

	// 等待退出信号
	m.wg.Wait()
	return nil
}

// handleSignals 处理信号
func (m *masterServerImpl) handleSignals() {
	m.wg.Add(1)
	go func() {
		defer m.wg.Done()

		// 监听中断信号
		sigChan := make(chan os.Signal, 1)
		signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

		select {
		case <-sigChan:
			sdk.Logger().Debug("收到信号, 正在停止服务...")
			m.shutdown()
		case <-m.ctx.Done():
			sdk.Logger().Debug("收到停止信号")
		}
	}()
}

// handleFileChanges 处理文件变化
func (m *masterServerImpl) handleFileChanges(changedPaths []string) {
	sdk.Logger().Debug("检测到文件变化", "path", changedPaths)

	// 执行同步
	if err := m.syncer.BatchSync(changedPaths); err != nil {
		sdk.Logger().Error("同步失败", "err", err)
		return
	}

	// 同步成功，发送消息到消息队列
	syncTime := time.Now().Format(time.RFC3339)
	if m.eventServer != nil {
		for _, path := range changedPaths {
			if err := m.eventServer.PublishMessage("", path); err != nil {
				sdk.Logger().Error("publish eventServer message", "err", err)
			}
		}
	}

	sdk.Logger().Debug("同步完成", "时间", syncTime)
}

// shutdown 优雅关闭
func (m *masterServerImpl) shutdown() {
	// 停止文件监控器
	if m.watcher != nil {
		if err := m.watcher.Stop(); err != nil {
			sdk.Logger().Debug("停止监控器失败", "err", err)
		}
	}

	// 停止event server
	if m.eventServer != nil {
		m.eventServer.Stop()
	}

	// 取消上下文
	m.cancel()
	sdk.Logger().Debug("服务已停止")
}
