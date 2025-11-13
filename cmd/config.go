package cmd

import (
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
	//var srv server.Server
	//
	//switch argServerMode {
	//case ServerModeMaster:
	//	srv = master.New()
	//default:
	//	srv = slave.New()
	//}
	//
	//err := srv.GenConfig()
	//if err != nil {
	//	sdk.Logger().Fatal("generate config", "err", err)
	//}

}
