package aida

import (
	"os"
	"path/filepath"
	"strings"
)

type Config struct {
	Addr             string
	AuthToken        string
	CodexBin         string
	DefaultWorkspace string
	AllowedRoots     []string
	RobotNotifyURL   string
	RobotNotifyToken string
}

func LoadConfigFromEnv() Config {
	cfg := Config{
		Addr:             envOrDefault("AIDA_BRIDGE_ADDR", "127.0.0.1:7826"),
		AuthToken:        strings.TrimSpace(os.Getenv("AIDA_BRIDGE_TOKEN")),
		CodexBin:         envOrDefault("AIDA_CODEX_BIN", "codex"),
		DefaultWorkspace: normalizePath(strings.TrimSpace(os.Getenv("AIDA_DEFAULT_WORKSPACE"))),
		RobotNotifyURL:   strings.TrimSpace(os.Getenv("AIDA_ROBOT_NOTIFY_URL")),
		RobotNotifyToken: strings.TrimSpace(os.Getenv("AIDA_ROBOT_NOTIFY_TOKEN")),
	}

	cfg.AllowedRoots = parseAllowedRoots(os.Getenv("AIDA_ALLOWED_WORKSPACES"))
	if cfg.DefaultWorkspace == "" && len(cfg.AllowedRoots) == 1 {
		cfg.DefaultWorkspace = cfg.AllowedRoots[0]
	}

	return cfg
}

func envOrDefault(key, fallback string) string {
	value := strings.TrimSpace(os.Getenv(key))
	if value == "" {
		return fallback
	}
	return value
}

func parseAllowedRoots(raw string) []string {
	if strings.TrimSpace(raw) == "" {
		return nil
	}

	fields := strings.FieldsFunc(raw, func(r rune) bool {
		return r == ',' || r == '\n'
	})

	roots := make([]string, 0, len(fields))
	for _, field := range fields {
		if path := normalizePath(field); path != "" {
			roots = append(roots, path)
		}
	}
	return roots
}

func normalizePath(path string) string {
	path = strings.TrimSpace(path)
	if path == "" {
		return ""
	}
	absPath, err := filepath.Abs(path)
	if err != nil {
		return filepath.Clean(path)
	}
	return filepath.Clean(absPath)
}
