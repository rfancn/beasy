package run

import (
	"github.com/hdget/sdk"
	"github.com/hdget/utils/logger"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/internal/server/slave"
	"github.com/spf13/cobra"
)

var (
	argConfigFile string
	Command       = &cobra.Command{
		Use:   "run",
		Short: "run server",
		PreRun: func(cmd *cobra.Command, args []string) {
			err := sdk.New(g.App, sdk.WithConfigFile(argConfigFile)).UseConfig(&g.Config).Initialize()
			if err != nil {
				logger.Fatal("sdk initialize", "err", err)
			}
		},
		Run: func(cmd *cobra.Command, args []string) {
			runSlaveServer()
		},
	}
)

func init() {
	Command.PersistentFlags().StringVarP(&argConfigFile, "config", "c", "", "config file")

	Command.AddCommand(subCmdRunMaster)
}

func runSlaveServer() {
	err := slave.New().Run()
	if err != nil {
		sdk.Logger().Fatal("run slave server", "err", err)
	}
}
