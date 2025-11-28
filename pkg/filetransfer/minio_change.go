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

func (m minioSyncerImpl) SyncChange(item *filewatch.ChangedItem) ([]string, []string, error) {
	if item.FileInfo.IsDir() {
		if item.Operation&(fsnotify.Remove|fsnotify.Rename) != 0 {
			// delete dir
			fmt.Println("remove all files under remote dir", item.Path)
			deleted, err := m.deleteRemoteDir(item.Path)
			if err != nil {
				return nil, nil, err
			}
			return nil, deleted, nil
		}

		fmt.Println("nothing to do for remote dir", item.Path)
		return nil, nil, nil
	}

	if item.Operation&(fsnotify.Create|fsnotify.Write|fsnotify.Chmod) != 0 {
		// upload
		fmt.Println("upload file", item.Path)
	} else if item.Operation&(fsnotify.Remove|fsnotify.Rename) != 0 {
		fmt.Println("delete remote file")
		relPath, err := filepath.Rel(item.LocalBaseDir, item.Path)
		if err != nil {
			return nil, nil, err
		}
		deleted, err := m.deleteRemoteFile(item.RemoteBaseDir, relPath)
		if err != nil {
			return nil, nil, err
		}

		return nil, []string{deleted}, nil
	}

	return nil
}

func (m minioSyncerImpl) uploadRemoteFile(remoteDir string, relPath string) (string, error) {

}

func (m minioSyncerImpl) deleteRemoteFile(remoteDir string, relPath string) (string, error) {
	prefix := cleanPrefix(remoteDir)
	objectKey := path.Join(prefix, relPath)

	err := m.client.RemoveObject(m.ctx, g.Config.App.OSS.Bucket, objectKey, minio.RemoveObjectOptions{})
	if err != nil {
		return "", err
	}
	return objectKey, nil
}

// deleteRemoteDir 对象存储如果需要删除目录需要目录下所有文件
func (m minioSyncerImpl) deleteRemoteDir(remoteDir string) ([]string, error) {
	prefix := cleanPrefix(remoteDir)

	s3Objects, err := m.getRemoteObjects(prefix)
	if err != nil {
		return nil, errors.Wrap(err, "get remote objects")
	}

	// 执行删除
	removed := make([]string, 0)
	if len(s3Objects) > 0 {
		sdk.Logger().Debug("deleting remote object(s)", "total", len(s3Objects))

		objectsCh := make(chan minio.ObjectInfo)
		go func() {
			defer close(objectsCh)
			for relPath := range s3Objects {
				// 给机会中断可能长时间运行的动作
				if m.ctx.Err() != nil {
					return
				}

				key := filepath.ToSlash(filepath.Join(prefix, relPath))
				objectsCh <- minio.ObjectInfo{Key: key}
			}
		}()

		for result := range m.client.RemoveObjectsWithResult(m.ctx, g.Config.App.OSS.Bucket, objectsCh, minio.RemoveObjectsOptions{}) {
			if result.Err != nil {
				return removed, fmt.Errorf("delete remote object: %s, err: %w", result.ObjectName, result.Err)
			}

			removed = append(removed, result.ObjectName)
		}

	}

	return removed, nil
}

func (m minioSyncerImpl) syncDir(localPath string, localFileInfo os.FileInfo, remoteDir string) ([]string, []string, error) {
	prefix := cleanPrefix(remoteDir)

	localFiles, err := m.getLocalFiles(localPath, localFileInfo)
	if err != nil {
		return nil, nil, errors.Wrap(err, "get local files")
	}

	s3Objects, err := m.getRemoteObjects(prefix)
	if err != nil {
		return nil, nil, errors.Wrap(err, "get remote objects")
	}

	// 准备工作：需要上传的文件 + 需要删除的对象
	var toUpload []string
	var toDelete []string

	// 找出需要上传的（本地有，远端无 或 内容不同）
	for relPath, fileInfo := range localFiles {
		if s3Obj, exists := s3Objects[relPath]; exists {
			// 比较大小和修改时间（简单策略，也可用 ETag）
			if fileInfo.Size() == s3Obj.Size && fileInfo.ModTime().Unix() <= s3Obj.LastModified.Unix() {
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
				return nil, nil, nil
			}

			if errRemove.Err != nil {
				return nil, nil, fmt.Errorf("delete object: %s, err: %w", errRemove.ObjectName, errRemove.Err)
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

				localFullPath := path.Join(filepath.ToSlash(localPath), rel)
				s3Key := filepath.Join(prefix, rel)
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
			return nil, nil, uploadErr
		}
	}

	return toUpload, toDelete, nil
}
