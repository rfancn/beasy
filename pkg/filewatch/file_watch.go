package filewatch

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/elliotchance/pie/v2"
	"github.com/fsnotify/fsnotify"
	"github.com/hdget/sdk"
	"github.com/pkg/errors"
	"github.com/rfancn/beasy/g"
)

// FileWatch 监控器接口
type FileWatch interface {
	Run()
	Stop() error
}

// fileWatchImpl 实现监控器接口
type fileWatchImpl struct {
	watcher  *fsnotify.Watcher
	onChange func(changedPaths []string)
	stopChan chan struct{}
}

const (
	defaultDebounceTime = 3 * time.Second
)

// New 创建新的文件监控器
func New(options ...Option) (FileWatch, error) {
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		return nil, err
	}

	impl := &fileWatchImpl{
		watcher:  watcher,
		stopChan: make(chan struct{}),
	}

	for _, option := range options {
		option(impl)
	}

	// 添加监控路径
	for _, path := range g.Config.App.FileWatch.Paths {
		if err = impl.addPath(path); err != nil {
			return nil, errors.Wrapf(err, "add watch path, path: %s", path)
		} else {
			sdk.Logger().Debug("add watch path", "path", path)
		}
	}

	return impl, nil
}

// Run 启动监控器
func (impl *fileWatchImpl) Run() {
	changedPaths := make(map[string]struct{})

	timer := time.NewTimer(defaultDebounceTime)

	for {
		select {
		case event, ok := <-impl.watcher.Events:
			if !ok { // 如果通道被关闭，则退出
				if g.Debug {
					sdk.Logger().Debug("file watch stopped")
				}
				return
			}

			// 处理文件变化事件
			if event.Op&(fsnotify.Write|fsnotify.Create|fsnotify.Remove|fsnotify.Rename|fsnotify.Chmod) != 0 {
				changedPaths[event.Name] = struct{}{}
			}
		case err, ok := <-impl.watcher.Errors:
			if !ok {
				if g.Debug {
					sdk.Logger().Debug("file watch stopped")
				}
				return
			}
			sdk.Logger().Error("receive file watch error", "err", err)
		case <-timer.C:
			// 定时器触发，收集所有变化的路径并调用回调
			if len(changedPaths) > 0 && impl.onChange != nil {
				// 将改变的内容发给onChange
				impl.onChange(pie.Keys(changedPaths))

				// 重新初始化
				changedPaths = make(map[string]struct{})
			}

			timer.Reset(defaultDebounceTime)
		case <-impl.stopChan:
			sdk.Logger().Debug("quit file watch")
			return
		}
	}
}

// Stop 停止监控器
func (impl *fileWatchImpl) Stop() error {
	close(impl.stopChan)
	return impl.watcher.Close()
}

func (impl *fileWatchImpl) addPath(path string) error {
	// 检查路径是否存在
	if _, err := os.Stat(path); os.IsNotExist(err) {
		return fmt.Errorf("path doesn't exist: %s", path)
	}

	// 获取文件信息判断类型
	info, err := os.Stat(path)
	if err != nil {
		return err
	}

	if info.IsDir() {
		// 如果是目录，递归添加所有子目录
		return impl.addDirectoryRecursive(path)
	} else {
		// 如果是文件，直接添加监控
		return impl.addFile(path)
	}
}

// watchFile watch one file
func (impl *fileWatchImpl) addFile(filePath string) error {
	return impl.watcher.Add(filePath)
}

func (impl *fileWatchImpl) addDirectoryRecursive(rootPath string) error {
	return filepath.Walk(rootPath, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			// 跳过无权限访问的目录
			if os.IsPermission(err) {
				sdk.Logger().Warn("permission denied, skipped...", "path", path)
				return filepath.SkipDir
			}
			return err
		}

		// 只监控目录（不监控单个文件）
		if info.IsDir() {
			// 跳过隐藏目录（如.git, .svn等）
			if strings.Contains(path, "/.") || strings.HasPrefix(filepath.Base(path), ".") {
				if path != rootPath { // 不跳过根目录
					return filepath.SkipDir
				}
			}

			err = impl.watcher.Add(path)
			if err != nil {
				sdk.Logger().Error("cannot watch", "path", path, "err", err)
				return err // 继续处理其他目录
			}
		}
		return nil
	})
}
