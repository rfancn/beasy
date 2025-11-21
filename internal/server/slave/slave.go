package slave

import (
	"context"
	"encoding/json"
	"os"
	"os/signal"
	"path"
	"syscall"
	"time"

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

		// 强制修改监控目录的mTime, 触发全量同步
		for _, watch := range g.Config.App.FileWatches {
			now := time.Now()
			if err = os.Chtimes(watch.Path, now, now); err != nil {
				return errors.Wrapf(err, "trigger full sync for path: %s", watch.Path)
			}
		}

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
				_ = s.watch.Stop()
				return nil
			default:
				err := s.eventClient.Subscribe(s.ctx, s.handleNotify)
				if err != nil {
					sdk.Logger().Error("subscribe message", "err", err)
				}
			}
		}
	}
}

// handleFileChanges file changes on master server will sync to fileTransfer and notify all slaves
func (s *slaveServerImpl) handleFileChanges(changes []*filewatch.ChangedItem) {
	for _, item := range changes {
		// now only sync action supported
		// 备份文件：sync from local => remote
		uploads, deletes, err := s.fileTransfer.SyncRemote(item, s.GetRootDir())
		if err != nil {
			sdk.Logger().Error("sync remote", "err", err)
			return
		}

		if len(uploads) > 0 || len(deletes) > 0 {
			sdk.Logger().Debug("sync remote done", "path", item.Path, "uploads", uploads, "deletes", deletes)
		}
	}
}

func (s *slaveServerImpl) handleNotify(msg *sse.Event) {
	sdk.Logger().Debug("receive message", "topic", string(msg.Event), "content", string(msg.Data))

	switch string(msg.Event) {
	case event.TopicFileChanges:
		if err := s.onMasterFileChanges(msg.Data); err != nil {
			sdk.Logger().Error("handle master file changes event", "err", err)
		}
	}
}

func (s *slaveServerImpl) onMasterFileChanges(data []byte) error {
	var changedPaths []string
	err := json.Unmarshal(data, &changedPaths)
	if err != nil {
		return err
	}

	var foundPath, foundCommand string
	for _, notify := range g.Config.App.Notifies {
		for _, changedPath := range changedPaths {
			if g.Debug {
				sdk.Logger().Debug("match changed path", "notify", notify.ChangedPath, "changed", changedPath)
			}

			matched, err := path.Match(notify.ChangedPath, changedPath)
			if err != nil {
				return err
			}

			if matched {
				foundPath = changedPath
				foundCommand = notify.Command
				break
			}
		}
	}

	sdk.Logger().Debug("file changes detected", "path", foundPath, "command", foundCommand)
	return nil
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
