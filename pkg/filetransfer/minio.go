package filetransfer

import (
	"context"
	"fmt"
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

func (m minioSyncerImpl) getRemoteObjects(prefix string) (map[string]minio.ObjectInfo, error) {
	// 获取远端已有对象列表
	s3Objects := make(map[string]minio.ObjectInfo)
	for obj := range m.client.ListObjects(m.ctx, g.Config.App.OSS.Bucket, minio.ListObjectsOptions{
		Prefix:    prefix,
		Recursive: true,
	}) {
		// 给机会中断可能长时间运行的动作
		if m.ctx.Err() != nil {
			return nil, nil
		}

		if obj.Err != nil {
			return nil, fmt.Errorf("list s3 objects error: %w", obj.Err)
		}

		// 移除前缀，得到相对路径并格式化
		keyWithoutPrefix := filepath.ToSlash(strings.TrimPrefix(obj.Key, prefix))
		if strings.HasSuffix(keyWithoutPrefix, "/") || keyWithoutPrefix == "" {
			continue // 跳过目录或者空对象
		}
		s3Objects[keyWithoutPrefix] = obj
	}

	return s3Objects, nil
}

//
//func cleanPrefix(remotePath string) string {
//	remotePath = filepath.ToSlash(remotePath)
//
//	// 去除前面的"/"
//	remotePath = strings.TrimPrefix(remotePath, "/")
//
//	if !strings.HasSuffix(remotePath, "/") && remotePath != "" {
//		remotePath += "/"
//	}
//
//	return remotePath
//}
