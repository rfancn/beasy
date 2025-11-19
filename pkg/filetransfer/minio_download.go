package filetransfer

import (
	"fmt"
	"io"
	"net/url"
	"os"
	"path"
	"path/filepath"
	"strings"
	"time"

	"github.com/hdget/sdk"
	"github.com/minio/minio-go/v7"
	"github.com/pkg/errors"
	"github.com/rfancn/beasy/g"
)

func (m minioSyncerImpl) Download(remotePath, localDir string) error {
	prefix, pattern := m.getPrefixAndPattern(remotePath)

	// 列出指定前缀下的所有对象（包括子目录）
	objectCh := m.client.ListObjects(m.ctx, g.Config.App.OSS.Bucket, minio.ListObjectsOptions{
		Prefix:    prefix,
		Recursive: true, // 递归获取所有子目录
	})
	// 处理每个对象
	for {
		select {
		case obj, ok := <-objectCh:
			if !ok {
				return nil
			}

			if err := m.handleObj(obj, prefix, pattern, localDir); err != nil {
				sdk.Logger().Error("handle object", "err", err)
			}
		case <-m.ctx.Done():
			return nil
		}
	}
}

func (m minioSyncerImpl) handleObj(obj minio.ObjectInfo, prefix, pattern, localDir string) error {
	if obj.Key == "" || strings.HasSuffix(obj.Key, "/") { // 跳过目录本身（OSS中目录是虚拟的，不实际存在）
		return nil
	}

	if obj.Err != nil {
		return errors.Wrap(obj.Err, "list object")
	}

	// 计算相对路径（相对于prefix）
	relKey := obj.Key[len(prefix):]

	// 6. 通配符匹配（支持*和?）
	matched := false
	if pattern == "*" {
		matched = true // 无通配符时匹配所有
	} else {
		matched, _ = path.Match(pattern, relKey)
	}

	if !matched {
		return nil
	}

	// 生成本地文件路径
	localPath := filepath.Join(localDir, relKey)
	dir := filepath.Dir(localPath)

	// 创建本地目录
	if err := os.MkdirAll(dir, os.ModePerm); err != nil {
		return errors.Wrapf(err, "create local dir, dir: %s", dir)
	}

	// 检查文件是否已存在，如果存在则重命名
	if _, err := os.Stat(localPath); err == nil {
		newFilePath, err := m.backupFile(localPath)
		if err != nil {
			return errors.Wrapf(err, "backup file: %s", localPath)
		}
		sdk.Logger().Debug("duplicate file exists, rename it", "old", localPath, "new", newFilePath)
	}

	// 下载文件
	if err := m.downloadFile(obj.Key, localPath); err != nil {
		return errors.Wrapf(err, "download file %s", obj.Key)
	}

	sdk.Logger().Debug("download file", "remote", obj.Key, "local", localPath)
	return nil
}

func (m minioSyncerImpl) backupFile(localPath string) (string, error) {
	// 文件已存在，重命名
	dir := filepath.Dir(localPath)
	base := filepath.Base(localPath)
	ext := filepath.Ext(base)
	nameWithoutExt := base[:len(base)-len(ext)]
	timestamp := time.Now().Format("20060102150405") // 格式: YYYYMMDDHHMMSS
	newBase := fmt.Sprintf("%s_%s%s", nameWithoutExt, timestamp, ext)
	newFilePath := filepath.Join(dir, newBase)
	return newFilePath, os.Rename(localPath, newFilePath)
}

func (m minioSyncerImpl) downloadFile(objectKey, localPath string) error {
	// 解码OSS Key（处理中文文件名）
	decodedKey, err := url.PathUnescape(objectKey)
	if err != nil {
		return fmt.Errorf("unescape object key, err: %v", err)
	}

	reader, err := m.client.GetObject(m.ctx, g.Config.App.OSS.Bucket, decodedKey, minio.GetObjectOptions{})
	if err != nil {
		return errors.Wrapf(err, "get object, name: %s", objectKey)
	}
	defer func() {
		_ = reader.Close()
	}()

	// 创建本地文件
	localFile, err := os.Create(localPath)
	if err != nil {
		return errors.Wrapf(err, "create file, path: %s", localPath)
	}
	defer func() {
		_ = localFile.Close()
	}()

	// 复制内容到本地文件
	if _, err := io.Copy(localFile, reader); err != nil {
		return errors.Wrapf(err, "copy file, path: %s", localPath)
	}

	return nil
}

func (m minioSyncerImpl) getPrefixAndPattern(remotePath string) (string, string) {
	// 1. 处理通配符和路径类型
	hasWildcard := strings.ContainsAny(remotePath, "*?")

	var prefix, pattern string
	if hasWildcard {
		// 通配符路径：提取前缀和通配符模式
		prefix = filepath.Dir(remotePath)
		pattern = filepath.Base(remotePath)
	} else {
		// 普通路径：判断是文件还是目录
		if strings.HasSuffix(remotePath, "/") {
			// 目录：使用完整路径作为前缀，通配符模式为"*"
			prefix = remotePath
			pattern = "*"
		} else {
			// 文件：使用父目录作为前缀，文件名作为通配符
			prefix = filepath.Dir(remotePath)
			pattern = filepath.Base(remotePath)
		}
	}

	prefix = cleanPrefix(prefix)

	return prefix, pattern
}
