package cmd

import (
	"github.com/hdget/sdk"
	"github.com/rfancn/beasy/internal/server"
	"github.com/rfancn/beasy/internal/server/master"
	"github.com/rfancn/beasy/internal/server/slave"
	"github.com/spf13/cobra"
)

var (
	configCommand = &cobra.Command{
		Use:   "config",
		Short: "generate config",
		Run: func(cmd *cobra.Command, args []string) {
			config()
		},
	}
)

func config() {
	var srv server.Server

	switch argServerMode {
	case ServerModeMaster:
		srv = master.New()
	default:
		srv = slave.New()
	}

	err := srv.GenConfig()
	if err != nil {
		sdk.Logger().Fatal("generate config", "err", err)
	}

}
