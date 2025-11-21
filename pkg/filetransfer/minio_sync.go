package filetransfer

import (
	"fmt"
	"os"
	"path"
	"path/filepath"
	"strings"
	"sync"

	"github.com/fsnotify/fsnotify"
	"github.com/hdget/sdk"
	"github.com/minio/minio-go/v7"
	"github.com/pkg/errors"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/pkg/filewatch"
)

const (
	concurrent = 3 // 并发上传数
)

func (m minioSyncerImpl) getLocalFiles(changedItem *filewatch.ChangedItem) (map[string]os.FileInfo, error) {
	// 构建本地文件映射：relativePath -> os.FileInfo
	localFiles := make(map[string]os.FileInfo)

	// 如果是删除操作，返回空
	if changedItem.Operation&(fsnotify.Remove|fsnotify.Rename) != 0 {
		return nil, nil
	}

	if changedItem.FileInfo.IsDir() {
		// 遍历目录
		err := filepath.Walk(changedItem.Path, func(path string, info os.FileInfo, err error) error {
			// 给机会中断可能长时间运行的动作
			if m.ctx.Err() != nil {
				return nil
			}

			if err != nil {
				return err
			}
			if info.IsDir() {
				return nil
			}
			rel, err := filepath.Rel(changedItem.Path, path)
			if err != nil {
				return err
			}
			localFiles[filepath.ToSlash(rel)] = info
			return nil
		})
		if err != nil {
			return nil, fmt.Errorf("walk local dir: %w", err)
		}
	} else {
		filename := filepath.Base(changedItem.Path)
		localFiles[path.Join(changedItem.BaseDir, filename)] = changedItem.FileInfo
	}

	return localFiles, nil
}

func (m minioSyncerImpl) SyncToRemote(changedItem *filewatch.ChangedItem, remotePath string) error {
	prefix := cleanPrefix(remotePath)

	localFiles, err := m.getLocalFiles(changedItem)
	if err != nil {
		return errors.Wrap(err, "get local files")
	}

	// 获取远端已有对象列表
	s3Objects := make(map[string]minio.ObjectInfo)
	for obj := range m.client.ListObjects(m.ctx, g.Config.App.OSS.Bucket, minio.ListObjectsOptions{
		Prefix:    prefix,
		Recursive: true,
	}) {
		// 给机会中断可能长时间运行的动作
		if m.ctx.Err() != nil {
			return nil
		}

		if obj.Err != nil {
			return fmt.Errorf("list s3 objects error: %w", obj.Err)
		}
		// 移除前缀，得到相对路径
		keyWithoutPrefix := strings.TrimPrefix(obj.Key, prefix)
		if strings.HasSuffix(keyWithoutPrefix, "/") || keyWithoutPrefix == "" {
			continue // 跳过目录或者空对象
		}
		s3Objects[keyWithoutPrefix] = obj
	}

	// 准备工作：需要上传的文件 + 需要删除的对象
	var toUpload []string
	var toDelete []string

	// 找出需要上传的（本地有，远端无 或 内容不同）
	for relPath, localFileInfo := range localFiles {
		if s3Obj, exists := s3Objects[relPath]; exists {
			// 比较大小和修改时间（简单策略，也可用 ETag）
			if localFileInfo.Size() == s3Obj.Size && localFileInfo.ModTime().Unix() <= s3Obj.LastModified.Unix() {
				continue
			}
		}

		toUpload = append(toUpload, relPath)
	}

	// 找出需要删除的（远端有，本地无）
	for relPath := range s3Objects {
		if _, exists := localFiles[relPath]; !exists {
			toDelete = append(toDelete, relPath)
		}
	}

	if g.Debug {
		sdk.Logger().Debug("sync remote status", "to_upload", toUpload, "to_delete", toDelete)
	}

	// 执行删除
	if len(toDelete) > 0 {
		sdk.Logger().Debug("deleting remote object(s) not present locally", "total", len(toDelete))
		objectsCh := make(chan minio.ObjectInfo)
		go func() {
			defer close(objectsCh)
			for _, relPath := range toDelete {
				key := filepath.ToSlash(filepath.Join(prefix, relPath))
				objectsCh <- minio.ObjectInfo{Key: key}
			}
		}()

		for errRemove := range m.client.RemoveObjects(m.ctx, g.Config.App.OSS.Bucket, objectsCh, minio.RemoveObjectsOptions{}) {
			// 给机会中断可能长时间运行的动作
			if m.ctx.Err() != nil {
				return nil
			}

			if errRemove.Err != nil {
				return fmt.Errorf("delete object: %s, err: %w", errRemove.ObjectName, errRemove.Err)
			}
		}

	}

	// 执行上传（并发控制）
	if len(toUpload) > 0 {
		sdk.Logger().Debug("uploading file(s)", "total", len(toUpload))
		sem := make(chan struct{}, concurrent)
		var wg sync.WaitGroup
		var uploadErr error
		var mu sync.Mutex

		for _, relPath := range toUpload {
			wg.Add(1)
			go func(rel string) {
				defer wg.Done()
				sem <- struct{}{}
				defer func() { <-sem }()

				localFullPath := changedItem.Path
				if changedItem.FileInfo.IsDir() {
					localFullPath = filepath.Join(changedItem.Path, rel)
				}
				s3Key := filepath.ToSlash(filepath.Join(prefix, rel))
				_, err := m.client.FPutObject(m.ctx, g.Config.App.OSS.Bucket, s3Key, localFullPath, minio.PutObjectOptions{})
				if err != nil {
					mu.Lock()
					if uploadErr == nil {
						uploadErr = fmt.Errorf("upload %s to %s: %w", localFullPath, s3Key, err)
					}
					mu.Unlock()
				} else {
					sdk.Logger().Debug("success upload", "file", s3Key)
				}
			}(relPath)
		}

		wg.Wait()
		if uploadErr != nil {
			return uploadErr
		}
	}

	return nil
}
