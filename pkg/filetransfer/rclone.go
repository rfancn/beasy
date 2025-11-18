package filetransfer

import (
	"context"

	"github.com/pkg/errors"
	_ "github.com/rclone/rclone/backend/local" // 导入本地文件系统后端
	_ "github.com/rclone/rclone/backend/s3"    // 导入S3/OSS后端
)

// rcloneSyncerImpl 实现同步器接口
type rcloneSyncerImpl struct {
	ctx context.Context
}

func newRcloneFileTransfer() (FileTransfer, error) {
	impl := &rcloneSyncerImpl{}
	if err := impl.validateConfig(); err != nil {
		return nil, errors.Wrap(err, "invalid remote config")
	}
	return impl, nil
}
