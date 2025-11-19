package server

type Server interface {
	Run() error
	GetRootDir() string
}
