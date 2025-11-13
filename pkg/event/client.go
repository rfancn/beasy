package event

import (
	"github.com/r3labs/sse/v2"
	"github.com/rfancn/beasy/g"
)

type Client struct {
	sseClient *sse.Client
}

func NewClient() *Client {
	return &Client{
		sseClient: sse.NewClient(g.Config.Event.Url + sseEndpoint + "?stream=" + streamMessage),
	}
}

func (c *Client) Subscribe(callback func(topic, content string)) error {
	return c.sseClient.Subscribe(streamMessage, func(msg *sse.Event) {
		callback(string(msg.Data), string(msg.Data))
	})
}
