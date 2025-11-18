package filetransfer

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/hdget/sdk"
	"github.com/pkg/errors"
	"github.com/rclone/rclone/fs"
	"github.com/rclone/rclone/fs/operations"
	"github.com/rclone/rclone/fs/sync"
	"github.com/rfancn/beasy/g"
)

// SyncWithRemote 同步本地路径到远程
func (s *rcloneSyncerImpl) SyncWithRemote(localPaths []string) error {
	for _, localPath := range localPaths {
		remotePath := fmt.Sprintf("%s:%s", g.Config.App.Remote.Bucket, filepath.ToSlash(filepath.Dir(localPath)))

		err := s.sync(localPath, remotePath)
		if err != nil {
			return errors.Wrapf(err, "failed to sync %s to %s", localPath, remotePath)
		}
	}
	return nil
}

// CopyToRemote 复制本地路径到远程
func (s *rcloneSyncerImpl) CopyToRemote(localPaths []string) error {
	for _, localPath := range localPaths {
		remotePath := fmt.Sprintf("%s:%s", g.Config.App.Remote.Bucket, filepath.ToSlash(filepath.Dir(localPath)))

		err := s.copyToRemote(localPath, remotePath)
		if err != nil {
			return errors.Wrapf(err, "failed to copyToRemote %s to %s", localPath, remotePath)
		}
	}
	return nil
}

// toRemote local => remote
func (s *rcloneSyncerImpl) copyToRemote(localPath, remotePath string) error {
	return filepath.Walk(localPath, func(currentPath string, info os.FileInfo, err error) error {
		if err != nil {
			sdk.Logger().Error("failed to walk local path", "path", currentPath, "err", err)
			return nil // 返回 nil 可跳过错误项继续遍历
		}

		if info.IsDir() {
			if err = s.copyDirToRemote(currentPath, remotePath); err != nil {
				return err
			}
		} else {
			if err = s.copyFileToRemote(currentPath, remotePath); err != nil {
				return err
			}
		}
		return nil
	})
}

func (s *rcloneSyncerImpl) copyDirToRemote(localDir, remotePath string) error {
	if localDir == "" {
		localDir = "." // 如果当前目录，设为 "."
	}

	// parse local fs
	localFs, err := fs.NewFs(s.ctx, localDir)
	if err != nil {
		return errors.Wrap(err, "create local file system")
	}

	remoteFs, err := fs.NewFs(s.ctx, remotePath)
	if err != nil {
		return errors.Wrapf(err, "create dest file system: %s", remotePath)
	}

	if g.Debug {
		sdk.Logger().Debug("sync from local to remote: %s -> %s", localDir, remotePath)
	}

	// do sync operation
	err = sync.CopyDir(s.ctx, remoteFs, localFs, true)
	if err != nil {
		return errors.Wrapf(err, "sync from local to remote, local: %s, remote: %s", localDir, remotePath)
	}

	if g.Debug {
		sdk.Logger().Debug("successfully sync from local to remote: %s -> %s", localDir, remotePath)
	}

	return nil
}

func (s *rcloneSyncerImpl) copyFileToRemote(localPath, remotePath string) error {
	localDir, localFile := splitPath(localPath)

	// parse local fs
	localFs, err := fs.NewFs(s.ctx, localDir)
	if err != nil {
		return errors.Wrap(err, "create local file system")
	}

	remoteFs, err := fs.NewFs(s.ctx, remotePath)
	if err != nil {
		return errors.Wrapf(err, "create dest file system: %s", remotePath)
	}

	if g.Debug {
		sdk.Logger().Debug("copyToRemote from local to remote: %s -> %s", localDir, remotePath)
	}

	// do sync operation
	err = operations.CopyFile(s.ctx, remoteFs, localFs, localFile, localFile)
	if err != nil {
		return errors.Wrapf(err, "copyToRemote from local to remote, local: %s, remote: %s", localDir, remotePath)
	}

	if g.Debug {
		sdk.Logger().Debug("successfully copyToRemote from local to remote: %s -> %s", localDir, remotePath)
	}

	return nil
}

func (s *rcloneSyncerImpl) sync(localPath, remotePath string) error {
	if _, err := os.Stat(localPath); os.IsNotExist(err) {
		return errors.Wrapf(err, "local path does not exist, path: %s", localPath)
	}

	// parse local oss path
	srcFs, err := fs.NewFs(s.ctx, localPath)
	if err != nil {
		return errors.Wrap(err, "create source file system")
	}

	dstFs, err := fs.NewFs(s.ctx, remotePath)
	if err != nil {
		return errors.Wrapf(err, "create dest file system: %s", remotePath)
	}

	if g.Debug {
		sdk.Logger().Debug("sync from local to remote: %s -> %s", localPath, remotePath)
	}

	// do sync operation
	err = sync.Sync(s.ctx, dstFs, srcFs, true)
	if err != nil {
		return errors.Wrapf(err, "sync from local to remote, local: %s, remote: %s", localPath, remotePath)
	}

	if g.Debug {
		sdk.Logger().Debug("successfully sync from local to remote: %s -> %s", localPath, remotePath)
	}

	return nil
}

func (s *rcloneSyncerImpl) validateConfig() error {
	if g.Config.App.Remote.Bucket == "" {
		return errors.New("remote bucket not provided")
	}

	if g.Config.App.Remote.ACL == "private" {
		if g.Config.App.Remote.AccessKey == "" || g.Config.App.Remote.AccessSecret == "" {
			return errors.New("remote access info not provided")
		}
	}

	if g.Config.App.Remote.Endpoint == "" {
		return errors.New("remote endpoint not provided")
	}
	return nil
}

// 分解路径和文件名的通用方法
func splitPath(p string) (dir, file string) {
	return filepath.Dir(p), filepath.Base(p)
}
