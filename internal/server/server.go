package server

type Server interface {
	Run() error
	GetRemoteRoot() string
}
