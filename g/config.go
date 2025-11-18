package g

type confRoot struct {
	App confApp `toml:"app"`
}

// Config 根配置结构
type confApp struct {
	FileWatch confFileWatch `mapstructure:"file_watch"`
	Remote    confRemote    `mapstructure:"remote"`
	Event     confEvent     `mapstructure:"event"`
}

// confFileWatch 监控配置
type confFileWatch struct {
	Paths []string `mapstructure:"paths"`
}

type confRemote struct {
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
