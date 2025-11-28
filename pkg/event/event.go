package event

import "context"

const (
	headerAccessSecret = "hd-beasy-token"
	TopicFileChanges   = "file:changes"
)

type Client interface {
	Subscribe(ctx context.Context, msgHandler MessageHandler) error
}

type Server interface {
	PublishMessage(topic string, msg any) error
	Run()
	Stop()
}
