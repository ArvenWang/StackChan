package aida

import (
	"encoding/json"
	"net/http"
	"strconv"
	"strings"
	"time"
)

type handler struct {
	cfg     Config
	manager *Manager
}

type createTaskPayload struct {
	Title     string `json:"title"`
	Prompt    string `json:"prompt"`
	Workspace string `json:"workspace"`
	Notify    *bool  `json:"notify"`
}

func NewHandler(cfg Config, manager *Manager) http.Handler {
	h := &handler{cfg: cfg, manager: manager}
	mux := http.NewServeMux()
	mux.HandleFunc("/health", h.handleHealth)
	mux.HandleFunc("/v1/tasks", h.handleTasks)
	mux.HandleFunc("/v1/tasks/", h.handleTaskByID)
	return h.withAuth(mux)
}

func (h *handler) withAuth(next http.Handler) http.Handler {
	if h.cfg.AuthToken == "" {
		return next
	}

	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		token := strings.TrimSpace(strings.TrimPrefix(r.Header.Get("Authorization"), "Bearer "))
		if token != h.cfg.AuthToken {
			writeJSON(w, http.StatusUnauthorized, map[string]string{
				"error": "unauthorized",
			})
			return
		}
		next.ServeHTTP(w, r)
	})
}

func (h *handler) handleHealth(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "method not allowed"})
		return
	}

	writeJSON(w, http.StatusOK, map[string]any{
		"ok":        true,
		"timestamp": time.Now().UTC(),
	})
}

func (h *handler) handleTasks(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		limit := 10
		if rawLimit := strings.TrimSpace(r.URL.Query().Get("limit")); rawLimit != "" {
			parsedLimit, err := strconv.Atoi(rawLimit)
			if err != nil || parsedLimit < 1 || parsedLimit > 100 {
				writeJSON(w, http.StatusBadRequest, map[string]string{"error": "limit must be between 1 and 100"})
				return
			}
			limit = parsedLimit
		}
		writeJSON(w, http.StatusOK, h.manager.ListTasks(limit))
		return
	}

	if r.Method != http.MethodPost {
		writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "method not allowed"})
		return
	}

	var payload createTaskPayload
	if err := json.NewDecoder(http.MaxBytesReader(w, r.Body, 1<<20)).Decode(&payload); err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": "invalid json body"})
		return
	}

	notify := true
	if payload.Notify != nil {
		notify = *payload.Notify
	}

	task, err := h.manager.CreateTask(CreateTaskRequest{
		Title:     payload.Title,
		Prompt:    payload.Prompt,
		Workspace: payload.Workspace,
		Notify:    notify,
	})
	if err != nil {
		writeJSON(w, http.StatusBadRequest, map[string]string{"error": err.Error()})
		return
	}

	writeJSON(w, http.StatusAccepted, task)
}

func (h *handler) handleTaskByID(w http.ResponseWriter, r *http.Request) {
	trimmed := strings.TrimPrefix(r.URL.Path, "/v1/tasks/")
	parts := strings.Split(strings.Trim(trimmed, "/"), "/")
	if len(parts) == 0 || parts[0] == "" {
		writeJSON(w, http.StatusNotFound, map[string]string{"error": "task not found"})
		return
	}

	taskID := parts[0]
	if len(parts) == 1 && r.Method == http.MethodGet {
		task, ok := h.manager.GetTask(taskID)
		if !ok {
			writeJSON(w, http.StatusNotFound, map[string]string{"error": "task not found"})
			return
		}
		writeJSON(w, http.StatusOK, task)
		return
	}

	if len(parts) == 2 && parts[1] == "cancel" && r.Method == http.MethodPost {
		if err := h.manager.CancelTask(taskID); err != nil {
			writeJSON(w, http.StatusNotFound, map[string]string{"error": err.Error()})
			return
		}
		task, _ := h.manager.GetTask(taskID)
		writeJSON(w, http.StatusOK, task)
		return
	}

	writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "unsupported route"})
}

func writeJSON(w http.ResponseWriter, statusCode int, value any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(statusCode)
	_ = json.NewEncoder(w).Encode(value)
}
