package event

import (
	"encoding/json"
	"fmt"
	"net/http"
	"time"

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
	// IMPORTANT: all configuration need to be done before create stream
	sseServer := sse.New()

	// no auto replay
	sseServer.AutoReplay = false

	// on subscribe
	sseServer.OnSubscribe = func(streamID string, sub *sse.Subscriber) {
		sdk.Logger().Debug("stream subscribed")
	}

	// on unsubscribe
	sseServer.OnUnsubscribe = func(streamID string, sub *sse.Subscriber) {
		sdk.Logger().Debug("stream unsubscribed")
	}

	sseServer.CreateStream(streamMessage)

	mux := http.NewServeMux()
	mux.HandleFunc(sseEndpoint, func(w http.ResponseWriter, r *http.Request) {
		if r.Header.Get(headerAccessSecret) != g.Config.App.Event.Secret {
			w.WriteHeader(http.StatusForbidden)
			return
		}

		sseServer.ServeHTTP(w, r)
	})

	return &Server{
		sseServer:    sseServer,
		httpSeverMux: mux,
	}
}

func (s *Server) Run() {
	url := fmt.Sprintf(":%d", g.Config.App.Event.Port)
	err := http.ListenAndServe(url, s.httpSeverMux)
	if err != nil {
		sdk.Logger().Fatal("listen event: ", "err", err, "url", url)
	}
}

// PublishMessage 发布消息处理
func (s *Server) PublishMessage(topic string, msg any) error {
	if !s.sseServer.StreamExists(streamMessage) {
		return fmt.Errorf("stream %s does not exist", streamMessage)
	}

	data, err := json.Marshal(msg)
	if err != nil {
		return err
	}

	// 推送消息
	if ok := s.sseServer.TryPublish(streamMessage, &sse.Event{
		ID:    []byte(time.Now().Format(time.RFC3339)),
		Event: []byte(topic),
		Data:  data,
	}); !ok {
		return errors.New("message not published")
	}

	return nil
}

func (s *Server) Stop() {
	s.sseServer.Close()
}
