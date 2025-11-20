package filetransfer

import (
	"github.com/pkg/errors"
	"github.com/rfancn/beasy/g"
)

// FileTransfer file transfer
type FileTransfer interface {
	SyncToRemote(localPath string, remotePath string) error // sync from local => remote
	Download(remotePath, localPath string) error            // copyToRemote from remote => local
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
