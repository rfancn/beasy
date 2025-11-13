package sync

import (
	"context"
)

// Syncer 同步器接口
type Syncer interface {
	Sync(localPath string) error         // 同步
	BatchSync(localPaths []string) error // 批量同步
}

// New 创建新的rclone同步器
func New() Syncer {
	return &rcloneSyncerImpl{
		ctx: context.Background(),
	}
}
