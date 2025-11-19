package filetransfer

import (
	"context"
	"path/filepath"
	"strings"

	"github.com/minio/minio-go/v7"
	"github.com/minio/minio-go/v7/pkg/credentials"
	"github.com/pkg/errors"
	"github.com/rfancn/beasy/g"
)

// minioSyncerImpl 实现同步器接口
type minioSyncerImpl struct {
	client *minio.Client
	ctx    context.Context
}

func newMinioTransfer() (FileTransfer, error) {
	client, err := newMinioClient()
	if err != nil {
		return nil, err
	}
	return &minioSyncerImpl{
		client: client,
		ctx:    context.Background(),
	}, nil
}

func newMinioClient() (*minio.Client, error) {
	endpoint, err := getEndpoint()
	if err != nil {
		return nil, err
	}

	var options *minio.Options
	if g.Config.App.OSS.ACL == "private" {
		options = &minio.Options{
			Creds:  credentials.NewStaticV4(g.Config.App.OSS.AccessKey, g.Config.App.OSS.AccessSecret, ""),
			Secure: true,
		}
	} else {
		options = &minio.Options{
			Secure: true,
		}
	}

	client, err := minio.New(endpoint, options)
	if err != nil {
		return nil, errors.Wrap(err, "new oss client")
	}

	return client, nil
}

func cleanPrefix(remotePath string) string {
	remotePath = filepath.ToSlash(remotePath)

	// 去除前面的"/"
	remotePath = strings.TrimPrefix(remotePath, "/")

	if !strings.HasSuffix(remotePath, "/") && remotePath != "" {
		remotePath += "/"
	}

	return remotePath
}
