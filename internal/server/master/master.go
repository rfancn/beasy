package master

import (
	"context"
	"os"
	"os/signal"
	"path"
	"sync"
	"syscall"
	"time"

	"github.com/elliotchance/pie/v2"
	"github.com/hdget/sdk"
	"github.com/pkg/errors"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/internal/server"
	"github.com/rfancn/beasy/pkg/event"
	"github.com/rfancn/beasy/pkg/filetransfer"
	"github.com/rfancn/beasy/pkg/filewatch"
)

type masterServerImpl struct {
	wg     sync.WaitGroup
	ctx    context.Context
	cancel context.CancelFunc

	watch        filewatch.FileWatch
	eventServer  event.Server
	fileTransfer filetransfer.FileTransfer
}

const (
	dirMaster = "master"
)

func New() server.Server {
	return &masterServerImpl{}
}

func (m *masterServerImpl) GetRemoteRoot() string {
	return dirMaster
}

func (m *masterServerImpl) Run() error {
	// 创建上下文用于控制生命周期
	m.ctx, m.cancel = context.WithCancel(context.Background())
	defer m.cancel()

	// 处理信号
	m.handleSignals()

	// 文件同步
	{
		fileTransfer, err := filetransfer.New()
		if err != nil {
			return errors.Wrap(err, "initialize file transfer")
		}

		m.fileTransfer = fileTransfer
	}

	// 文件监控
	{
		fileWatcher, err := filewatch.New(
			filewatch.WithOnChange(m.handleChanges),
		)
		if err != nil {
			return errors.Wrap(err, "initialize file watch")
		}

		m.watch = fileWatcher
		go m.watch.Run()

		if g.Debug {
			sdk.Logger().Debug("file watch started")
		}
	}

	{ // 启动事件服务器
		m.eventServer = event.NewServer()
		go m.eventServer.Run()

		if g.Debug {
			sdk.Logger().Debug("event server started")
		}
	}

	// 等待退出信号
	m.wg.Wait()
	return nil
}

func (m *masterServerImpl) postStartup() error {
	// watch启动时会进行全量同步
	for _, watch := range g.Config.App.FileWatches {
		uploaded, deleted, err := m.fileTransfer.Sync(watch.Path, path.Join(m.GetRemoteRoot(), watch.RemoteDir))
		if err != nil {
			return errors.Wrapf(err, "sync files, path: %s", watch.Path)
		}

		if len(uploaded) != 0 || len(deleted) != 0 {
			sdk.Logger().Debug("sync files done", "uploaded", uploaded, "deleted", deleted)
		} else {
			sdk.Logger().Debug("nothing to sync, skip it")
		}
	}

}

// handleChanges file changes on master server will sync to fileTransfer and notify all slaves
func (m *masterServerImpl) handleChanges(changes []*filewatch.ChangedItem) {
	sdk.Logger().Debug("file changes detected", "paths", pie.Map(changes, func(v *filewatch.ChangedItem) string {
		return v.Path
	}))

	// sync from local => remote
	changedPaths := make([]string, 0)
	for _, item := range changes {
		switch item.Action {
		default: // case sync
			changedPath, err := m.fileTransfer.SyncChange(item)
			if err != nil {
				return
			}

			if changedPath != "" {
				changedPaths = append(changedPaths, changedPath)
			}
		}
	}

	// notify slaves
	if m.eventServer != nil {
		err := m.eventServer.PublishMessage(event.TopicFileChanges, changedPaths)
		if err != nil {
			sdk.Logger().Error("publish eventServer message", "err", err)
		} else {
			sdk.Logger().Debug("publish message", "time", time.Now())
		}
	}
}

// handleSignals handle signals
func (m *masterServerImpl) handleSignals() {
	m.wg.Add(1)
	go func() {
		defer m.wg.Done()

		// 监听中断信号
		sigChan := make(chan os.Signal, 1)
		signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

		select {
		case <-sigChan:
			sdk.Logger().Debug("receive signal, try stop service...")
			m.shutdown()
		case <-m.ctx.Done():
			sdk.Logger().Debug("receive stop signal")
			m.shutdown()
		}
	}()
}

// Stop 优雅关闭
func (m *masterServerImpl) shutdown() {
	// 停止文件监控器
	if m.watch != nil {
		if err := m.watch.Stop(); err != nil {
			sdk.Logger().Debug("stop file watch", "err", err)
		}
	}

	// 停止event server
	if m.eventServer != nil {
		m.eventServer.Stop()
	}

	// 取消上下文
	m.cancel()
	sdk.Logger().Debug("file watch service stopped")
}
