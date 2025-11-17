package sync

import (
	"context"
	"fmt"
	"log"
	"os"

	_ "github.com/rclone/rclone/backend/local" // 导入本地文件系统后端
	_ "github.com/rclone/rclone/backend/s3"    // 导入S3/OSS后端
	"github.com/rclone/rclone/fs"
	"github.com/rclone/rclone/fs/sync"
	"github.com/rfancn/beasy/g"
)

// rcloneSyncerImpl 实现同步器接口
type rcloneSyncerImpl struct {
	ctx context.Context
}

// Sync 同步本地路径到远程OSS
func (s *rcloneSyncerImpl) Sync(localPath string) error {
	if g.Config.App.Sync.RemotePath == "" {
		return fmt.Errorf("远程路径未配置")
	}

	// 确保本地路径存在
	if _, err := os.Stat(localPath); os.IsNotExist(err) {
		return fmt.Errorf("本地路径不存在: %s", localPath)
	}

	// 解析源文件系统（本地路径）
	srcFs, err := fs.NewFs(s.ctx, localPath)
	if err != nil {
		return fmt.Errorf("创建源文件系统失败: %v", err)
	}

	// 解析目标文件系统（远程OSS路径）
	dstFs, err := fs.NewFs(s.ctx, g.Config.App.Sync.RemotePath)
	if err != nil {
		return fmt.Errorf("创建目标文件系统失败: %v", err)
	}

	log.Printf("开始同步: %s -> %s", localPath, g.Config.App.Sync.RemotePath)

	// 执行同步操作 - 根据rclone v1.65.0的函数签名添加必要参数
	err = sync.Sync(s.ctx, dstFs, srcFs, true)
	if err != nil {
		return fmt.Errorf("同步失败: %v", err)
	}

	log.Printf("同步完成: %s -> %s", localPath, g.Config.App.Sync.RemotePath)

	return nil
}

// BatchSync 批量同步多个路径
func (s *rcloneSyncerImpl) BatchSync(localPaths []string) error {
	for _, path := range localPaths {
		if err := s.Sync(path); err != nil {
			return fmt.Errorf("同步路径 %s 失败: %v", path, err)
		}
	}
	return nil
}
