package utils

import (
	"path/filepath"
	"strings"
)

func CleanPrefix(remotePath string) string {
	remotePath = filepath.ToSlash(remotePath)

	// 去除前面的"/"
	remotePath = strings.TrimPrefix(remotePath, "/")

	if !strings.HasSuffix(remotePath, "/") && remotePath != "" {
		remotePath += "/"
	}

	return remotePath
}

func GetPrefixAndPattern(remotePath string) (string, string) {
	// 1. 处理通配符和路径类型
	hasWildcard := strings.ContainsAny(remotePath, "*?")

	var prefix, pattern string
	if hasWildcard {
		// 通配符路径：提取前缀和通配符模式
		prefix = filepath.Dir(remotePath)
		pattern = filepath.Base(remotePath)
	} else {
		// 普通路径：判断是文件还是目录
		if strings.HasSuffix(remotePath, "/") {
			// 目录：使用完整路径作为前缀，通配符模式为"*"
			prefix = remotePath
			pattern = "*"
		} else {
			// 文件：使用父目录作为前缀，文件名作为通配符
			prefix = filepath.Dir(remotePath)
			pattern = filepath.Base(remotePath)
		}
	}

	return CleanPrefix(prefix), pattern
}
