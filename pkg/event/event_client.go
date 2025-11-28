package event

import (
	"context"
	"fmt"
	"time"

	"github.com/hdget/sdk"
	"github.com/r3labs/sse/v2"
	"github.com/rfancn/beasy/g"
	"gopkg.in/cenkalti/backoff.v1"
)

type clientImpl struct {
	sseClient *sse.Client
	eventChan chan *sse.Event
}

type MessageHandler func(msg *sse.Event)

func NewClient() Client {
	instance := &clientImpl{
		eventChan: make(chan *sse.Event),
	}

	url := fmt.Sprintf("%s:%d%s", g.Config.App.Event.Url, g.Config.App.Event.Port, sseEndpoint)
	instance.sseClient = sse.NewClient(url)
	// 设置secret
	instance.sseClient.Headers[headerAccessSecret] = g.Config.App.Event.Secret
	// 设置callback
	instance.sseClient.OnConnect(onConnect)
	instance.sseClient.OnDisconnect(onDisconnect)

	// 永远尝试连接，即使连接断了也重连
	reconnectStrategy := backoff.NewExponentialBackOff()
	reconnectStrategy.MaxElapsedTime = 3 * time.Second
	instance.sseClient.ReconnectStrategy = reconnectStrategy

	instance.sseClient.ReconnectNotify = func(err error, duration time.Duration) {
		if g.Debug {
			sdk.Logger().Debug("reconnect notify", "err", err)
		}
	}

	return instance
}

func (c *clientImpl) Subscribe(ctx context.Context, msgHandler MessageHandler) error {
	if g.Debug {
		sdk.Logger().Debug("try subscribe message")
	}

	// IMPORTANT: 每次重新订阅需要清空LastEventID
	c.sseClient.LastEventID.Store([]byte(""))

	err := c.sseClient.SubscribeWithContext(ctx, streamMessage, msgHandler)
	if err != nil {
		return err
	}

	if g.Debug {
		sdk.Logger().Debug("quit subscribe message")
	}

	return nil
}

func onConnect(c *sse.Client) {
	sdk.Logger().Debug("event server connected", "url", c.URL)
}

func onDisconnect(c *sse.Client) {
	sdk.Logger().Debug("event server disconnected", "url", c.URL)
}
