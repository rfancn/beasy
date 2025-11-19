package g

type confRoot struct {
	App confApp `toml:"app"`
}

// Config 根配置结构
type confApp struct {
	FileWatch confFileWatch `mapstructure:"file_watch"`
	OSS       confOSS       `mapstructure:"oss"`
	Event     confEvent     `mapstructure:"event"`
}

// confFileWatch 监控配置
type confFileWatch struct {
	LocalPaths   []string `mapstructure:"local_paths"`
	RemotePrefix string   `mapstructure:"remote_prefix"`
}

type confOSS struct {
	Provider     string `mapstructure:"provider"`
	Bucket       string `mapstructure:"bucket"`
	Endpoint     string `mapstructure:"endpoint"`
	AccessKey    string `mapstructure:"access_key"`
	AccessSecret string `mapstructure:"access_secret"`
	ACL          string `mapstructure:"acl"`
}

type confEvent struct {
	Url    string `mapstructure:"url"`
	Port   int    `mapstructure:"port"`
	Secret string `mapstructure:"secret"`
}
