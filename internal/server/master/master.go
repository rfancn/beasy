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
	watcher filewatch.FileWatcher
	event   *event.Server
	syncer  pkgsync.Syncer
	wg      sync.WaitGroup
	ctx     context.Context
	cancel  context.CancelFunc
}

func New() server.Server {
	return &masterServerImpl{}
}

func (m *masterServerImpl) Run() error {
	// 创建上下文用于控制生命周期
	m.ctx, m.cancel = context.WithCancel(context.Background())

	// 初始化组件
	if err := m.initializeComponents(); err != nil {
		return fmt.Errorf("初始化组件失败: %v", err)
	}

	// 处理信号
	m.handleSignals()

	// 等待退出信号
	m.wg.Wait()
	return nil
}

// initializeComponents 初始化组件
func (m *masterServerImpl) initializeComponents() error {
	// 初始化文件变动监控器
	fileWatcher, err := filewatch.New(
		filewatch.WithOnChange(m.handleFileChanges),
	)
	if err != nil {
		return fmt.Errorf("创建监控器失败: %v", err)
	}

	m.watcher = fileWatcher

	if g.Debug {
		sdk.Logger().Debug("file watcher initialized")
	}

	//// 初始化事件服务
	//m.event = event.NewServer()
	//go func() {
	//	err = m.event.Run()
	//	if err != nil {
	//		panic(err)
	//	}
	//}()
	//
	//if g.Debug {
	//	sdk.Logger().Debug("event server initialized")
	//}

	sdk.Logger().Debug("组件初始化完成")
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
		case sig := <-sigChan:
			sdk.Logger().Debug("收到信号, 正在停止服务...", "sig", sig)
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
	if m.event != nil {
		for _, path := range changedPaths {
			if err := m.event.PublishMessage("", path); err != nil {
				sdk.Logger().Error("publish event message", "err", err)
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
	m.event.Stop()

	// 取消上下文
	m.cancel()
	sdk.Logger().Debug("服务已停止")
}
