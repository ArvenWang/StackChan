package main

import (
	"log"
	"net/http"

	"stackChan/internal/aida"
)

func main() {
	cfg := aida.LoadConfigFromEnv()
	manager := aida.NewManager(cfg)

	log.Printf("Aida Bridge listening on %s", cfg.Addr)
	if cfg.DefaultWorkspace != "" {
		log.Printf("Default workspace: %s", cfg.DefaultWorkspace)
	}

	err := http.ListenAndServe(cfg.Addr, aida.NewHandler(cfg, manager))
	if err != nil {
		log.Fatalf("aida bridge stopped: %v", err)
	}
}
