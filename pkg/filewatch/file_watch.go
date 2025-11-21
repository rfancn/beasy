package filewatch

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"
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
	stopChan chan struct{}

	cache      map[string]os.FileInfo // 缓存路径信息：key为路径，value为os.FileInfo
	cacheMutex sync.RWMutex           // 保护缓存的读写锁

	onChange    FileChangeHandler // 路径变化后的回调函数
	path2action map[string]string // 监控路径变化后执行的动作
}

type ChangedItem struct {
	Path      string
	FileInfo  os.FileInfo
	Action    string
	Operation fsnotify.Op
	BaseDir   string
}

type ChangedEvent struct {
	FileInfo  os.FileInfo
	Operation fsnotify.Op
}

type FileChangeHandler func(changes []*ChangedItem)

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

		path2action: make(map[string]string),
		cache:       make(map[string]os.FileInfo),
	}

	for _, option := range options {
		option(impl)
	}

	// 添加监控路径
	for _, watch := range g.Config.App.FileWatches {
		for _, path := range watch.Paths {
			path = strings.TrimSpace(path)

			if path == "" {
				continue
			}

			if err = impl.addPath(path); err != nil {
				return nil, errors.Wrapf(err, "add watch path, path: %s", path)
			}

			impl.path2action[path] = watch.Action
			sdk.Logger().Debug("add watch path", "path", path)
		}
	}

	return impl, nil
}

// Run 启动监控器
func (impl *fileWatchImpl) Run() {
	changedPathMap := make(map[string]*ChangedEvent)

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

			if event.Op&(fsnotify.Write|fsnotify.Create|fsnotify.Chmod|fsnotify.Remove|fsnotify.Rename) != 0 {
				fileInfo, err := os.Stat(event.Name)
				// 如果取不到文件信息，则可能是删除动作，文件已不存在了，尝试从缓存中获取
				if err != nil {
					fileInfo = impl.cache[event.Name]
				} else { // 重新取的文件信息，需要更新到缓存中
					impl.cacheMutex.Lock()
					impl.cache[event.Name] = fileInfo
					impl.cacheMutex.Unlock()
				}

				if event.Op&fsnotify.Create != 0 && fileInfo.IsDir() {
					if err = impl.watcher.Add(event.Name); err != nil {
						sdk.Logger().Error("file watch add error", "path", event.Name, "error", err)
					}
					sdk.Logger().Debug("file watch add path", "path", event.Name)
				}

				if fileInfo != nil {
					changedPathMap[event.Name] = &ChangedEvent{
						FileInfo:  fileInfo,
						Operation: event.Op,
					}
				}
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
			if len(changedPathMap) > 0 && impl.onChange != nil {
				changedPaths := pie.Keys(changedPathMap)

				// 获取对应的action
				changes := make([]*ChangedItem, 0)
				for _, path := range changedPaths {

					var baseDir string
					for _, watch := range g.Config.App.FileWatches {
						for _, watchPath := range watch.Paths {
							if strings.HasPrefix(path, watchPath) {
								baseDir = watchPath
								break
							}
						}
					}

					changes = append(changes, &ChangedItem{
						Path:      path,
						Action:    impl.path2action[path],
						FileInfo:  changedPathMap[path].FileInfo,
						Operation: changedPathMap[path].Operation,
						BaseDir:   baseDir,
					})
				}

				// 将改变的内容发给onChange
				impl.onChange(changes)

				// 重新初始化
				changedPathMap = make(map[string]*ChangedEvent)
			}

			timer.Reset(defaultDebounceTime)
		case <-impl.stopChan:
			sdk.Logger().Debug("file watch stopped")
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
	// 获取文件信息判断类型
	fileInfo, err := os.Stat(path)
	if err != nil {
		if os.IsNotExist(err) {
			return fmt.Errorf("path doesn't exist: %s", path)
		}
		return err
	}

	if fileInfo.IsDir() {
		// 如果是目录，递归添加所有子目录
		return impl.addDirectoryRecursive(path)
	} else {
		impl.cacheMutex.Lock()
		impl.cache[path] = fileInfo
		impl.cacheMutex.Unlock()

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

		// 缓存fileInfo
		impl.cacheMutex.Lock()
		impl.cache[path] = info
		impl.cacheMutex.Unlock()

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
