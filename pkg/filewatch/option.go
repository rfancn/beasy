package filewatch

type Option func(impl *fileWatchImpl)

func WithOnChange(callback func(changedPaths []string)) Option {
	return func(impl *fileWatchImpl) {
		impl.onChange = callback
	}
}
