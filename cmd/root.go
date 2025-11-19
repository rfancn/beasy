package cmd

import (
	"fmt"
	"os"
	"runtime/debug"

	"github.com/rfancn/beasy/cmd/run"
	"github.com/rfancn/beasy/g"
	"github.com/spf13/cobra"
)

var (
	rootCommand = &cobra.Command{}
)

func init() {
	rootCommand.PersistentFlags().BoolVarP(&g.Debug, "debug", "d", false, "--debug")

	rootCommand.AddCommand(run.Command)
	rootCommand.AddCommand(configCommand)
	rootCommand.AddCommand(downloadCommand)
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

func fatalf(msg string, args ...interface{}) {
	fmt.Printf(msg+"\n", args...)
	os.Exit(1)
}
