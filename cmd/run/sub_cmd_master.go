package run

import (
	"github.com/hdget/sdk"
	"github.com/hdget/utils/logger"
	"github.com/rfancn/beasy/g"
	"github.com/rfancn/beasy/internal/server/master"
	"github.com/spf13/cobra"
)

var (
	subCmdRunMaster = &cobra.Command{
		Use:   "master",
		Short: "run master server",
		PreRun: func(cmd *cobra.Command, args []string) {
			err := sdk.New(g.App, sdk.WithConfigFile(argConfigFile)).UseConfig(&g.Config).Initialize()
			if err != nil {
				logger.Fatal("sdk initialize", "err", err)
			}
		},
		Run: func(cmd *cobra.Command, args []string) {
			runMasterSever()
		},
	}
)

func runMasterSever() {
	err := master.New().Run()
	if err != nil {
		sdk.Logger().Fatal("run master server", "err", err)
	}
}
