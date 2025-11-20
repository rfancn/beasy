package filewatch

type Option func(impl *fileWatchImpl)

func WithOnChange(callback func(changedPath2action map[string]string)) Option {
	return func(impl *fileWatchImpl) {
		impl.onChange = callback
	}
}
