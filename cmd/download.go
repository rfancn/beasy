package cmd

import (
	"os"
	"path"

	"github.com/hdget/sdk"
	"github.com/hdget/utils/logger"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/internal/server/master"
	"github.com/rfancn/beasy/pkg/filetransfer"
	"github.com/spf13/cobra"
)

var (
	argConfigFile   string
	downloadCommand = &cobra.Command{
		Use:   "download",
		Short: "download files from master",
		PreRun: func(cmd *cobra.Command, args []string) {
			err := sdk.New(g.App, sdk.WithConfigFile(argConfigFile)).UseConfig(&g.Config).Initialize()
			if err != nil {
				logger.Fatal("sdk initialize", "err", err)
			}
		},
		Run: func(cmd *cobra.Command, args []string) {
			download(args...)
		},
	}
)

func init() {
	downloadCommand.PersistentFlags().StringVarP(&argConfigFile, "config", "c", "", "config file")
}

func download(args ...string) {
	// 获取remotePath和localDir
	localDir, err := os.Getwd()
	if err != nil {
		sdk.Logger().Fatal("get current directory", "err", err)
	}

	var remotePath string
	switch len(args) {
	case 0:
		remotePath = "*"
	case 1:
		remotePath = args[0]
	case 2:
		remotePath = args[0]
		localDir = args[1]
	default:
		sdk.Logger().Fatal("usage: download <remote_path> <local_dir>")
	}

	remotePath = path.Join(master.New().GetRootDir(), remotePath)

	// download
	fileTransfer, err := filetransfer.New()
	if err != nil {
		sdk.Logger().Fatal("create file transfer", "err", err)
	}

	err = fileTransfer.Download(remotePath, localDir)
	if err != nil {
		sdk.Logger().Fatal("download file", "err", err)
	}
}
