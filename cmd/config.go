package cmd

import (
	"fmt"
	"os"

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
	templateConfigFile = `
[sdk]
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
	[app.file_watch]
		paths = ["/root/local/ssl"]

	[app.remote]
		path = "remote:bucket/path/to"
		endpoint = "oss-cn-hangzhou.aliyuncs.com"
		access_key_id = "your_access_key"
		secret_access_key = "your_secret_key"
		acl = "private"

	[app.event]
		url = "https://localhost"
		port = 8080
		secret = ""

[app]
	[app.file_watch]
		paths = ["/etc/nginx/conf.d"]

	[app.remote]
		path = "remote:bucket/path/to"
		endpoint = "oss-cn-hangzhou.aliyuncs.com"
		access_key_id = "your_access_key"
		secret_access_key = "your_secret_key"
		acl = "private"

	[app.event]
		url = "https://localhost"
		port = 8080
		secret = ""

`
)

func genConfig() error {
	// 写入默认配置文件
	if err := os.WriteFile("beasy.toml", []byte(templateConfigFile), 0644); err != nil {
		return fmt.Errorf("生成配置文件失败: %v", err)
	}

	fmt.Println("配置文件已生成: beasy.toml")
	return nil
}
