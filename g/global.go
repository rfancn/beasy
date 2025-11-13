package g

const (
	App        = "beasy"
	ConfigFile = App + ".toml"
)

var (
	Debug  bool // 是否开启debug模式
	Config confRoot
)
