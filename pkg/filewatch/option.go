package filewatch

type Option func(impl *fileWatchImpl)

func WithOnChange(callback func(changes []*ChangedItem)) Option {
	return func(impl *fileWatchImpl) {
		impl.onChange = callback
	}
}
