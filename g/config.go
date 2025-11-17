package g

type confRoot struct {
	App confApp `toml:"app"`
}

// Config 根配置结构
type confApp struct {
	FileWatch confFileWatch `mapstructure:"watcher"`
	Sync      confSync      `mapstructure:"sync"`
	Event     confEvent     `mapstructure:"event"`
}

// confFileWatch 监控配置
type confFileWatch struct {
	Paths    []string `mapstructure:"paths"`
	Debounce int      `mapstructure:"debounce"`
}

// confSync 同步配置
type confSync struct {
	Endpoint     string `mapstructure:"endpoint"`
	RemotePath   string `mapstructure:"remote_path"`
	AccessKey    string `mapstructure:"access_key"`
	AccessSecret string `mapstructure:"access_secret"`
	acl          string `mapstructure:"private"`
}

type confEvent struct {
	Url    string `mapstructure:"url"`
	Port   int    `mapstructure:"port"`
	Secret string `mapstructure:"secret"`
}
