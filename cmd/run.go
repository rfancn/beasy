package cmd

import (
	"github.com/hdget/sdk"
	"github.com/hdget/utils/logger"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/internal/server"
	"github.com/rfancn/beasy/internal/server/master"
	"github.com/rfancn/beasy/internal/server/slave"
	"github.com/spf13/cobra"
)

var (
	argConfigFile string
	runCommand    = &cobra.Command{
		Use:   "run",
		Short: "run server",
		PreRun: func(cmd *cobra.Command, args []string) {
			err := sdk.New(g.App, sdk.WithConfigFile(argConfigFile)).UseConfig(&g.Config).Initialize()
			if err != nil {
				logger.Fatal("sdk initialize", "err", err)
			}
		},
		Run: func(cmd *cobra.Command, args []string) {
			runSever()
		},
	}
)

func init() {
	runCommand.PersistentFlags().StringVarP(&argConfigFile, "config", "c", "", "config file")
}

func runSever() {
	var srv server.Server

	switch argServerMode {
	case ServerModeMaster:
		srv = master.New()
	default:
		srv = slave.New()
	}

	err := srv.Run()
	if err != nil {
		sdk.Logger().Fatal("run server", "err", err)
	}
}
