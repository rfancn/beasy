package g

import (
	"time"
)

type confRoot struct {
	App confApp `toml:"app"`
}

// Config 根配置结构
type confApp struct {
	FileWatch confFileWatch `toml:"file_watch"`
	Sync      confSync      `toml:"sync"`
	Event     confEvent     `toml:"event"`
}

// confFileWatch 监控配置
type confFileWatch struct {
	Paths        []string      `toml:"paths"`
	DebounceTime time.Duration `toml:"debounce_time"`
}

// confSync 同步配置
type confSync struct {
	RemotePath   string            `toml:"remote_path"`
	RcloneConfig map[string]string `toml:"rclone_config"`
}

type confEvent struct {
	Url    string `toml:"url"`
	Port   int    `toml:"port"`
	Secret string `toml:"secret"`
}
