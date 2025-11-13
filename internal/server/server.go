package server

type Server interface {
	Run() error
	GenConfig() error
}
