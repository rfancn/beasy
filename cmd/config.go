package cmd

import (
	"fmt"
	"net"
	"os"
	"path/filepath"
	"strings"
	"text/template"

	"github.com/chzyer/readline"
	"github.com/hdget/sdk"
	gonanoid "github.com/matoous/go-nanoid/v2"
	"github.com/pkg/errors"
	"github.com/rfancn/beasy/g"
	"github.com/spf13/cast"
	"github.com/spf13/cobra"
)

var (
	configCommand = &cobra.Command{
		Use:   "config",
		Short: "generate config",
		Run: func(cmd *cobra.Command, args []string) {
			genConfig()
		},
	}
)

const (
	templateConfigFile = `[sdk]
    [sdk.log]
        # 当前支持日志级别: "trace", "debug", "info", "warn", "error", "fatal", "panic"
        level = "debug"
        # 日志文件名称
        filename = "beasy.log"
        # 日志结转配置
    [sdk.log.rotate]
        # 日志最大保存时间7天(单位hour)
        max_age = 720
        # 日志切割时间间隔24小时（单位hour)
        rotation_time=24

[app]
    [app.oss]
        provider = "aliyun"
        bucket = "bucket"
        endpoint = "oss-cn-shanghai.aliyuncs.com"
        access_key = "your_access_key"
        access_secret = "your_secret_key"
        acl = "private"

    [app.event]
        url = "http://{{ .Host }}"
        port = {{ .Port }}
        secret = "{{ .Secret }}"

    # local file watches
    [[app.file_watches]]
        path = {{ .WatchPath }}
        remote_dir = {{ .RemoteDir }}

    # receive notifies (only applies to slave)
    [[app.notifies]]
        # relative changed path
        # changed_path = ""
        # customized command if changed_patch matched
        # command = ""
`
)

type configInput struct {
	Host      string
	Port      int
	Secret    string
	WatchPath string
}

func genConfig() {
	configType := getInput("What's config do you want to generate?", "slave", "master")

	switch configType {
	case "master":
		genMasterConfig()
	case "slave":
		genSlaveConfig()
	}
}

func genMasterConfig() {
	host := getInput("Please input host", getLocalIP())
	port := getInput("Please input port", "8080")

	secret, err := gonanoid.New(16)
	if err != nil {
		fatalf("error generate secret: %v\n", err)
	}

	tpl, err := template.New("").Parse(templateConfigFile)
	if err != nil {
		fatalf("parse config template: %v\n", err)
	}

	configFile := fmt.Sprintf("%s.toml", g.App)
	if existsFile(configFile) {
		fmt.Printf("%s exists, automatically saved as %s.bak\n", configFile, configFile)
		_ = os.Rename(configFile, configFile+".bak")
	}

	f, err := os.Create(configFile)
	if err != nil {
		fatalf("error create config file: %v", err)
	}
	defer func() {
		_ = f.Close()
	}()

	err = tpl.Execute(f, &configInput{
		Host:   host,
		Port:   cast.ToInt(port),
		Secret: secret,
	})
	if err != nil {
		fatalf("error render config template: %v", err)
	}

	fmt.Printf("config file generated: %s\n", configFile)
}

func genSlaveConfig() {
	host := getInput("Please input master host", "localhost")
	port := getInput("Please input master port", "8080")
	secret := getInput("Please input master secret")

	currentDir, err := os.Getwd()
	if err != nil {
		fatalf("error get current dir: %v", err)
	}

	watchPath := getInput("Please input watch path", filepath.ToSlash(currentDir))

	tpl, err := template.New("").Parse(templateConfigFile)
	if err != nil {
		fatalf("parse config template: %v\n", err)
	}

	configFile := fmt.Sprintf("%s.toml", g.App)
	if existsFile(configFile) {
		fmt.Printf("%s exists, automatically saved as %s.bak\n", configFile, configFile)
		_ = os.Rename(configFile, configFile+".bak")
	}

	f, err := os.Create(configFile)
	if err != nil {
		fatalf("error create config file: %v", err)
	}
	defer func() {
		_ = f.Close()
	}()

	err = tpl.Execute(f, &configInput{
		Host:      host,
		Port:      cast.ToInt(port),
		Secret:    secret,
		WatchPath: watchPath,
	})
	if err != nil {
		fatalf("error render config template: %v", err)
	}

	fmt.Printf("config file generated: %s\n", configFile)
}

// getInput 获取字符串输入
func getInput(prompt string, choices ...string) string {
	rlConfig := &readline.Config{}

	var defaultValue string
	if len(choices) > 0 {
		defaultValue = choices[0]
	}

	var fullPrompt string
	switch len(choices) {
	case 0:
		fullPrompt = fmt.Sprintf("%s: ", prompt)
	case 1:
		fullPrompt = fmt.Sprintf("%s[%s]: ", prompt, defaultValue)
	default:
		fullPrompt = fmt.Sprintf("%s[%s](%s): ", prompt, strings.Join(choices, "/"), defaultValue)
	}

	rlConfig.Prompt = fullPrompt
	rl, _ := readline.NewEx(rlConfig)
	defer func() {
		if rl != nil {
			_ = rl.Close()
		}
	}()

	var inputValue string
	line, err := rl.Readline()
	if err != nil {
		if errors.Is(err, readline.ErrInterrupt) {
			os.Exit(0)
		}
		inputValue = defaultValue
	} else {
		line = strings.TrimSpace(line)
		if line == "" {
			inputValue = defaultValue
		} else {
			inputValue = line
		}
	}
	return inputValue
}

func getLocalIP() string {
	addressList, err := net.InterfaceAddrs()
	if err != nil {
		sdk.Logger().Error("getLocalIP", "error", err)
		return ""
	}

	for _, a := range addressList {
		if ipNet, ok := a.(*net.IPNet); ok &&
			ipNet.IP.IsPrivate() &&
			ipNet.IP.To4() != nil {
			return ipNet.IP.To4().String()
		}
	}

	return ""
}

func existsFile(path string) bool {
	_, err := os.Stat(path)
	if err == nil {
		return true
	}
	if os.IsNotExist(err) {
		return false
	}
	// 其他类型的错误（如权限问题）也视为不存在
	return false
}
