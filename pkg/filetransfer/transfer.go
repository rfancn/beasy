package filetransfer

// FileTransfer file transfer
type FileTransfer interface {
	SyncWithRemote(localPaths []string) error          // sync from local => remote
	CopyToRemote(localPaths []string) error            // copyToRemote from local => remote
	CopyFromRemote(remotePath, localPath string) error // copyToRemote from remote => local
}

// New 创建新的rclone同步器
func New() (FileTransfer, error) {
	return newRcloneFileTransfer()
}
