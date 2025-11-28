package filetransfer

import (
	"github.com/minio/minio-go/v7"
	"github.com/pkg/errors"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/pkg/filewatch"
)

// FileTransfer file transfer
type FileTransfer interface {
	Sync(localPath, remoteDir string) ([]string, []string, error)              // 全量同步
	SyncChange(item *filewatch.ChangedItem) (string, error)                    // 同步文件
	Download(remotePath, localPath string) error                               // 下载
	HandleChange(obj minio.ObjectInfo, prefix, pattern, localDir string) error // 处理变化
}

// New 创建新的rclone同步器
func New() (FileTransfer, error) {
	return newMinioTransfer()
}

func getEndpoint() (string, error) {
	switch g.Config.App.OSS.Provider {
	case "s3", "minio", "rustfs", "aliyun":
		return g.Config.App.OSS.Endpoint, nil
	default:
		return "", errors.New("oss provider not supported")
	}
}
