package event

import (
	"fmt"
	"net/http"

	"github.com/hdget/sdk"
	"github.com/pkg/errors"
	"github.com/r3labs/sse/v2"
	"github.com/rfancn/beasy/g"
)

type Server struct {
	sseServer    *sse.Server
	httpSeverMux *http.ServeMux
}

const (
	streamMessage = "message"
	sseEndpoint   = "/event"
)

func NewServer() *Server {
	sseServer := sse.New()
	sseServer.CreateStream(streamMessage)
	sseServer.OnSubscribe = func(streamID string, sub *sse.Subscriber) {
		sdk.Logger().Debug("subscribed stream", "slave", sub.URL)
	}
	sseServer.OnUnsubscribe = func(streamID string, sub *sse.Subscriber) {
		sdk.Logger().Debug("unsubscribed stream", "slave", sub.URL)
	}

	mux := http.NewServeMux()
	mux.HandleFunc(sseEndpoint, sseServer.ServeHTTP)

	return &Server{
		sseServer:    sseServer,
		httpSeverMux: mux,
	}
}

func (s *Server) Run() error {
	return http.ListenAndServe(fmt.Sprintf(":%d", g.Config.Event.Port), s.httpSeverMux)
}

// PublishMessage 发布消息处理
func (s *Server) PublishMessage(topic, content string) error {
	if !s.sseServer.StreamExists(streamMessage) {
		return fmt.Errorf("stream %s does not exist", streamMessage)
	}

	// 推送消息
	if ok := s.sseServer.TryPublish(streamMessage, &sse.Event{
		Data:  []byte(content),
		Event: []byte(topic),
	}); !ok {
		return errors.New("message not published")
	}

	return nil
}

func (s *Server) Stop() {
	s.sseServer.Close()
}
