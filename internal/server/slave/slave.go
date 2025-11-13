package slave

import (
	"context"
	"fmt"
	"os"
	"os/signal"
	"sync"
	"syscall"

	"github.com/hdget/sdk"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/internal/server"
	"github.com/rfancn/beasy/pkg/event"
)

type slaveServerImpl struct {
	eventClient *event.Client
	ctx         context.Context
	cancel      context.CancelFunc
	wg          sync.WaitGroup
}

func New() server.Server {
	return &slaveServerImpl{}
}

func (m *slaveServerImpl) Run() error {
	// 创建上下文用于控制生命周期
	m.ctx, m.cancel = context.WithCancel(context.Background())

	// 2. 初始化组件
	if err := m.initializeComponents(); err != nil {
		return fmt.Errorf("初始化组件失败: %v", err)
	}

	// 4. 处理信号
	m.handleSignals()

	// 等待退出信号
	m.wg.Wait()
	return nil
}

// initializeComponents 初始化组件
func (m *slaveServerImpl) initializeComponents() error {
	// 初始化事件服务
	m.eventClient = event.NewClient()
	err := m.eventClient.Subscribe(m.onMessage)
	if err != nil {
		return err
	}

	if g.Debug {
		sdk.Logger().Debug("event subscribe initialized")
	}

	sdk.Logger().Debug("组件初始化完成")
	return nil
}

func (m *slaveServerImpl) onMessage(topic, content string) {
	sdk.Logger().Debug("receive message", "topic", topic, "content:", content)
}

// handleSignals 处理信号
func (m *slaveServerImpl) handleSignals() {
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

// shutdown 优雅关闭
func (m *slaveServerImpl) shutdown() {
	// 取消上下文
	m.cancel()
	sdk.Logger().Debug("服务已停止")
}

func (m *slaveServerImpl) GenConfig() error {
	//TODO implement me
	panic("implement me")
}
