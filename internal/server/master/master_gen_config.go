package master

import (
	"fmt"
	"os"
)

const (
	configFile = `
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
	[app.watcher]
		paths = ["/tmp/beasy"]
		debounce_time = "5"

	[app.sync]
		remote_path = "remote:bucket/data"

		[app.sync.rclone]
			endpoint = "oss-cn-hangzhou.aliyuncs.com"
			access_key_id = "your_access_key"
			secret_access_key = "your_secret_key"
			acl = "private"

	[app.event]
		url = "https://localhost"
		port = 8080
`
)

func (m *masterServerImpl) GenConfig() error {
	// 写入默认配置文件
	if err := os.WriteFile("beasy.toml", []byte(configFile), 0644); err != nil {
		return fmt.Errorf("生成配置文件失败: %v", err)
	}

	fmt.Println("配置文件已生成: beasy.toml")
	return nil
}
