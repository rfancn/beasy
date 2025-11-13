package cmd

import (
	"fmt"
	"os"
	"runtime/debug"

	"github.com/rfancn/beasy/g"
	"github.com/spf13/cobra"
)

const (
	ServerModeUnknown = iota
	ServerModeMaster
	ServerModeSlave
)

var (
	argServerMode int
	rootCommand   = &cobra.Command{}
)

func init() {
	rootCommand.PersistentFlags().BoolVarP(&g.Debug, "debug", "d", false, "--debug")
	rootCommand.PersistentFlags().IntVarP(&argServerMode, "master", "m", ServerModeMaster, "--master")

	rootCommand.AddCommand(runCommand)
	rootCommand.AddCommand(configCommand)
}

func Execute() {
	defer func() {
		if r := recover(); r != nil {
			fmt.Println(string(debug.Stack()))
		}
	}()
	if err := rootCommand.Execute(); err != nil {
		os.Exit(1)
	}
}
