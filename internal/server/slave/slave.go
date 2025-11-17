package slave

import (
	"context"
	"os"
	"os/signal"
	"syscall"

	"github.com/hdget/sdk"
	"github.com/r3labs/sse/v2"
	"github.com/rfancn/beasy/internal/server"
	"github.com/rfancn/beasy/pkg/event"
)

type slaveServerImpl struct {
	eventClient *event.Client
	ctx         context.Context
	cancelFunc  context.CancelFunc
}

func New() server.Server {
	return &slaveServerImpl{}
}

func (s *slaveServerImpl) Run() error {
	s.ctx, s.cancelFunc = context.WithCancel(context.Background())
	defer s.cancelFunc()

	go s.handleSignals()

	// 初始化事件服务
	s.eventClient = event.NewClient()

	for {
		select {
		case <-s.ctx.Done():
			return nil
		default:
			err := s.eventClient.Subscribe(s.ctx, s.handleMessage)
			if err != nil {
				sdk.Logger().Error("subscribe message", "err", err)
			}
		}
	}
}

func (s *slaveServerImpl) handleMessage(msg *sse.Event) {
	sdk.Logger().Debug("receive message", "topic", string(msg.Event), "content", string(msg.Data))
}

// handleSignals 处理信号
func (s *slaveServerImpl) handleSignals() {
	// 监听中断信号
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	<-sigChan
	sdk.Logger().Debug("收到信号, 正在停止服务...")
	s.shutdown()
}

// shutdown 优雅关闭
func (s *slaveServerImpl) shutdown() {
	s.cancelFunc()

}

func (s *slaveServerImpl) GenConfig() error {
	//TODO implement me
	panic("implement me")
}
