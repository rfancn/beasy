package g

import (
	"time"
)

type confRoot struct {
	App confApp `toml:"app"`
}

// Config 根配置结构
type confApp struct {
	FileWatch confFileWatch `toml:"watcher"`
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
	Endpoint     string `toml:"endpoint"`
	RemotePath   string `toml:"remote_path"`
	AccessKey    string `toml:"access_key"`
	AccessSecret string `toml:"access_secret"`
	acl          string `toml:"private"`
}

type confEvent struct {
	Url    string `toml:"url"`
	Port   int    `toml:"port"`
	Secret string `toml:"secret"`
}
