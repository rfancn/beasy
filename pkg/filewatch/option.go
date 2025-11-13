package filewatch

type Option func(impl *fileWatcherImpl)

func WithOnChange(callback func(changedPaths []string)) Option {
	return func(impl *fileWatcherImpl) {
		impl.onChange = callback
	}
}
