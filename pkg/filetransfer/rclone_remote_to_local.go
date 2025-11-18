package filetransfer

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/pkg/errors"
	"github.com/rclone/rclone/fs"
	"github.com/rclone/rclone/fs/operations"
	"github.com/rclone/rclone/fs/sync"
	"github.com/rfancn/beasy/g"
)

func (s *rcloneSyncerImpl) CopyFromRemote(remotePath, localPath string) error {
	remotePath = fmt.Sprintf("%s:%s", g.Config.App.Remote.Bucket, filepath.ToSlash(filepath.Dir(remotePath)))

	// 创建远程文件系统
	remoteFs, err := fs.NewFs(s.ctx, remotePath)
	if err != nil {
		return errors.New("remote fs not accessible")
	}

	if _, err := os.Stat(localPath); os.IsNotExist(err) {
		if err = os.MkdirAll(localPath, 0755); err != nil {
			return errors.Wrapf(err, "create local dir, dir: %s", localPath)
		}

		return errors.Wrapf(err, "local path does not exist, path: %s", localPath)
	}

	// 创建本地文件系统
	localFs, err := fs.NewFs(s.ctx, localPath)
	if err != nil {
		return errors.Wrapf(err, "create local fs, path: %s", localPath)
	}

	// 尝试判断远程路径是文件还是目录
	isFile, err := s.isRemotePathAFile(remoteFs)
	if err != nil {
		return errors.Wrap(err, "check remote path type")
	}

	if isFile {
		return s.copyFileFromRemote(remotePath, remoteFs, localFs)
	} else {
		return s.copyDirFromRemote(remoteFs, localFs)
	}
}

func (s *rcloneSyncerImpl) copyFileFromRemote(remotePath string, remoteFs, localFs fs.Fs) error {
	// 获取远程文件名
	fileName := s.getRemoteFileName(remotePath)

	// 获取远程文件对象
	remoteObj, err := remoteFs.NewObject(s.ctx, fileName)
	if err != nil {
		return fmt.Errorf("获取远程文件对象失败: %v", err)
	}

	if g.Debug {
		fmt.Printf("复制文件: %s (大小: %d bytes)\n", fileName, remoteObj.Size())
	}

	// 执行文件复制
	err = operations.CopyFile(s.ctx, localFs, remoteFs, fileName, fileName)
	if err != nil {
		return fmt.Errorf("文件复制失败: %v", err)
	}

	return nil
}

func (s *rcloneSyncerImpl) copyDirFromRemote(remoteFs, localFs fs.Fs) error {
	if g.Debug {
		fmt.Println("检测到远程路径指向目录，执行目录复制...")
	}

	return sync.CopyDir(s.ctx, localFs, remoteFs, true)
}

// getRemoteFileName 从远程路径中提取文件名
func (s *rcloneSyncerImpl) getRemoteFileName(remotePath string) string {
	// 远程路径格式: "remote:path/to/file.txt"
	// 我们需要提取文件名部分 "file.txt"

	// 分割远程路径
	parts := strings.Split(remotePath, ":")
	if len(parts) < 2 {
		return ""
	}

	pathPart := parts[1]
	return filepath.Base(pathPart)
}

func (s *rcloneSyncerImpl) isRemotePathAFile(remoteFs fs.Fs) (bool, error) {
	// 尝试列出根级内容
	entries, err := remoteFs.List(s.ctx, "")
	if err != nil {
		// 如果列出失败，可能是文件或权限问题
		return false, err
	}

	// 如果只有一个条目，并且这个条目是文件，则可能是文件路径
	if len(entries) == 1 {
		_, isFile := entries[0].(fs.Object)
		return isFile, nil
	}

	// 默认认为是目录
	return false, nil
}
