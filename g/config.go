package g

type confRoot struct {
	App confApp `toml:"app"`
}

// Config 根配置结构
type confApp struct {
	OSS         confOSS         `mapstructure:"oss"`
	Event       confEvent       `mapstructure:"event"`
	FileWatches []confFileWatch `mapstructure:"file_watches"`
	Notifies    []confNotify    `mapstructure:"notifies"`
}

// confFileWatch 监控配置
type confFileWatch struct {
	Paths  []string `mapstructure:"paths"`
	Action string   `mapstructure:"action"`
}

type confOSS struct {
	Provider     string `mapstructure:"provider"`
	Bucket       string `mapstructure:"bucket"`
	Endpoint     string `mapstructure:"endpoint"`
	AccessKey    string `mapstructure:"access_key"`
	AccessSecret string `mapstructure:"access_secret"`
	ACL          string `mapstructure:"acl"`
	Prefix       string `mapstructure:"prefix"`
}

type confEvent struct {
	Url    string `mapstructure:"url"`
	Port   int    `mapstructure:"port"`
	Secret string `mapstructure:"secret"`
}

type confNotify struct {
	Path    string `mapstructure:"path"`
	Command string `mapstructure:"command"`
}
