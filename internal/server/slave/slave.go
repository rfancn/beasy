package slave

import (
	"context"
	"encoding/json"
	"os"
	"os/signal"
	"path"
	"syscall"

	"github.com/hdget/sdk"
	"github.com/pkg/errors"
	"github.com/r3labs/sse/v2"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/internal/server"
	"github.com/rfancn/beasy/pkg/event"
	"github.com/rfancn/beasy/pkg/filetransfer"
	"github.com/rfancn/beasy/pkg/filewatch"
)

type slaveServerImpl struct {
	watch        filewatch.FileWatch
	fileTransfer filetransfer.FileTransfer
	eventClient  *event.Client

	ctx        context.Context
	cancelFunc context.CancelFunc
}

const (
	dirSlave = "slave"
)

func New() server.Server {
	return &slaveServerImpl{}
}

func (s *slaveServerImpl) GetRootDir() string {
	hostname, _ := os.Hostname()
	return path.Join(dirSlave, hostname)
}

func (s *slaveServerImpl) Run() error {
	s.ctx, s.cancelFunc = context.WithCancel(context.Background())
	defer s.cancelFunc()

	go s.handleSignals()

	// 文件同步
	{
		fileTransfer, err := filetransfer.New()
		if err != nil {
			return errors.Wrap(err, "create file transfer")
		}

		s.fileTransfer = fileTransfer
	}

	// 文件监控
	{
		fileWatcher, err := filewatch.New(
			filewatch.WithOnChange(s.handleFileChanges),
		)
		if err != nil {
			return errors.Wrap(err, "initialize file watch")
		}

		s.watch = fileWatcher
		go s.watch.Run()

		if g.Debug {
			sdk.Logger().Debug("file watch started")
		}
	}

	// 事件通知处理
	{
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
}

// handleFileChanges file changes on master server will sync to fileTransfer and notify all slaves
func (s *slaveServerImpl) handleFileChanges(changedPaths []string) {
	sdk.Logger().Debug("file changes detected", "paths", changedPaths)

	// 备份文件：sync from local => remote
	if err := s.fileTransfer.SyncToRemote(changedPaths, s.GetRootDir()); err != nil {
		sdk.Logger().Error("sync remote", "err", err)
		return
	}
}

func (s *slaveServerImpl) handleMessage(msg *sse.Event) {
	sdk.Logger().Debug("receive message", "topic", string(msg.Event), "content", string(msg.Data))

	switch string(msg.Event) {
	case event.TopicFileChanges:
		var files []string
		_ = json.Unmarshal(msg.Data, &files)
		sdk.Logger().Debug("file changes detected", "files", files)
	}
}

// handleSignals 处理信号
func (s *slaveServerImpl) handleSignals() {
	// 监听中断信号
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	<-sigChan
	sdk.Logger().Debug("receive signals, try stop service...")
	s.shutdown()
}

// shutdown 优雅关闭
func (s *slaveServerImpl) shutdown() {
	s.cancelFunc()

}
