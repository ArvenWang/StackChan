package aida

import (
	"bytes"
	"context"
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

type TaskStatus string

const (
	StatusQueued    TaskStatus = "queued"
	StatusRunning   TaskStatus = "running"
	StatusCompleted TaskStatus = "completed"
	StatusFailed    TaskStatus = "failed"
	StatusCanceled  TaskStatus = "canceled"
)

type Task struct {
	ID         string     `json:"id"`
	Title      string     `json:"title"`
	Prompt     string     `json:"prompt"`
	Workspace  string     `json:"workspace"`
	Notify     bool       `json:"notify"`
	Status     TaskStatus `json:"status"`
	Result     string     `json:"result,omitempty"`
	Error      string     `json:"error,omitempty"`
	ExitCode   int        `json:"exit_code,omitempty"`
	CreatedAt  time.Time  `json:"created_at"`
	StartedAt  *time.Time `json:"started_at,omitempty"`
	FinishedAt *time.Time `json:"finished_at,omitempty"`
}

type CreateTaskRequest struct {
	Title     string
	Prompt    string
	Workspace string
	Notify    bool
}

type Manager struct {
	cfg     Config
	client  *http.Client
	mu      sync.RWMutex
	tasks   map[string]*Task
	cancels map[string]context.CancelFunc
}

func NewManager(cfg Config) *Manager {
	return &Manager{
		cfg: cfg,
		client: &http.Client{
			Timeout: 10 * time.Second,
		},
		tasks:   make(map[string]*Task),
		cancels: make(map[string]context.CancelFunc),
	}
}

func (m *Manager) CreateTask(req CreateTaskRequest) (*Task, error) {
	prompt := strings.TrimSpace(req.Prompt)
	if prompt == "" {
		return nil, errors.New("prompt is required")
	}

	workspace, err := m.resolveWorkspace(req.Workspace)
	if err != nil {
		return nil, err
	}

	task := &Task{
		ID:        newTaskID(),
		Title:     strings.TrimSpace(req.Title),
		Prompt:    prompt,
		Workspace: workspace,
		Notify:    req.Notify,
		Status:    StatusQueued,
		CreatedAt: time.Now(),
	}

	m.mu.Lock()
	m.tasks[task.ID] = task
	response := cloneTask(task)
	m.mu.Unlock()

	go m.runTask(task.ID)

	return response, nil
}

func (m *Manager) GetTask(id string) (*Task, bool) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	task, ok := m.tasks[id]
	if !ok {
		return nil, false
	}
	return cloneTask(task), true
}

func (m *Manager) CancelTask(id string) error {
	m.mu.Lock()
	task, exists := m.tasks[id]
	if !exists {
		m.mu.Unlock()
		return errors.New("task not found")
	}

	if task.Status == StatusCompleted || task.Status == StatusFailed || task.Status == StatusCanceled {
		m.mu.Unlock()
		return nil
	}

	if cancel, cancelExists := m.cancels[id]; cancelExists {
		m.mu.Unlock()
		cancel()
		return nil
	}

	now := time.Now()
	task.Status = StatusCanceled
	task.FinishedAt = &now
	m.mu.Unlock()
	return nil
}

func (m *Manager) resolveWorkspace(workspace string) (string, error) {
	workspace = normalizePath(workspace)
	if workspace == "" {
		workspace = m.cfg.DefaultWorkspace
	}
	if workspace == "" {
		return "", errors.New("workspace is required")
	}

	info, err := os.Stat(workspace)
	if err != nil {
		return "", fmt.Errorf("workspace is unavailable: %w", err)
	}
	if !info.IsDir() {
		return "", errors.New("workspace must be a directory")
	}

	if len(m.cfg.AllowedRoots) == 0 {
		return workspace, nil
	}

	for _, root := range m.cfg.AllowedRoots {
		if workspace == root || strings.HasPrefix(workspace, root+string(os.PathSeparator)) {
			return workspace, nil
		}
	}

	return "", errors.New("workspace is outside allowed roots")
}

func (m *Manager) runTask(id string) {
	ctx, cancel := context.WithCancel(context.Background())

	m.mu.Lock()
	task := m.tasks[id]
	if task == nil {
		m.mu.Unlock()
		cancel()
		return
	}
	if task.Status == StatusCanceled {
		m.mu.Unlock()
		cancel()
		return
	}
	now := time.Now()
	task.Status = StatusRunning
	task.StartedAt = &now
	m.cancels[id] = cancel
	m.mu.Unlock()

	defer func() {
		m.mu.Lock()
		delete(m.cancels, id)
		m.mu.Unlock()
	}()

	tempDir, err := os.MkdirTemp("", "aida-bridge-*")
	if err != nil {
		m.failTask(id, 0, "failed to prepare task workspace: "+err.Error())
		return
	}
	defer os.RemoveAll(tempDir)

	lastMessagePath := filepath.Join(tempDir, "last_message.txt")
	args := []string{
		"exec",
		"-C", task.Workspace,
		"-o", lastMessagePath,
		task.Prompt,
	}

	cmd := exec.CommandContext(ctx, m.cfg.CodexBin, args...)
	output, execErr := cmd.CombinedOutput()

	lastMessage := readOptionalFile(lastMessagePath)
	rawOutput := strings.TrimSpace(string(output))

	if errors.Is(ctx.Err(), context.Canceled) {
		m.finishCanceledTask(id)
		return
	}

	exitCode := 0
	if execErr != nil {
		if exitErr, ok := execErr.(*exec.ExitError); ok {
			exitCode = exitErr.ExitCode()
		} else {
			exitCode = -1
		}
	}

	if execErr != nil {
		message := chooseMessage(lastMessage, rawOutput, execErr.Error())
		m.failTask(id, exitCode, message)
		return
	}

	result := chooseMessage(lastMessage, rawOutput, "Codex completed without a final message.")
	m.completeTask(id, exitCode, result)
}

func (m *Manager) completeTask(id string, exitCode int, result string) {
	var notifyTask *Task

	m.mu.Lock()
	task := m.tasks[id]
	now := time.Now()
	task.Status = StatusCompleted
	task.Result = strings.TrimSpace(result)
	task.ExitCode = exitCode
	task.FinishedAt = &now
	notifyTask = cloneTask(task)
	m.mu.Unlock()

	m.notifyRobot(notifyTask)
}

func (m *Manager) failTask(id string, exitCode int, message string) {
	var notifyTask *Task

	m.mu.Lock()
	task := m.tasks[id]
	now := time.Now()
	task.Status = StatusFailed
	task.Error = strings.TrimSpace(message)
	task.ExitCode = exitCode
	task.FinishedAt = &now
	notifyTask = cloneTask(task)
	m.mu.Unlock()

	m.notifyRobot(notifyTask)
}

func (m *Manager) finishCanceledTask(id string) {
	m.mu.Lock()
	task := m.tasks[id]
	now := time.Now()
	task.Status = StatusCanceled
	task.Error = "Task canceled."
	task.FinishedAt = &now
	m.mu.Unlock()
}

func (m *Manager) notifyRobot(task *Task) {
	if task == nil || !task.Notify || strings.TrimSpace(m.cfg.RobotNotifyURL) == "" {
		return
	}

	body := robotNotification{
		Source:  "aida-bridge",
		TaskID:  task.ID,
		Status:  string(task.Status),
		Title:   buildNotificationTitle(task),
		Message: buildNotificationMessage(task),
	}

	payload, err := json.Marshal(body)
	if err != nil {
		return
	}

	req, err := http.NewRequest(http.MethodPost, m.cfg.RobotNotifyURL, bytes.NewReader(payload))
	if err != nil {
		return
	}
	req.Header.Set("Content-Type", "application/json")
	if m.cfg.RobotNotifyToken != "" {
		req.Header.Set("Authorization", "Bearer "+m.cfg.RobotNotifyToken)
	}

	resp, err := m.client.Do(req)
	if err != nil {
		return
	}
	defer resp.Body.Close()
	io.Copy(io.Discard, resp.Body)
}

type robotNotification struct {
	Source  string `json:"source"`
	TaskID  string `json:"task_id"`
	Status  string `json:"status"`
	Title   string `json:"title"`
	Message string `json:"message"`
}

func buildNotificationTitle(task *Task) string {
	name := strings.TrimSpace(task.Title)
	if name == "" {
		name = "Codex 任务"
	}

	switch task.Status {
	case StatusCompleted:
		return name + "已完成"
	case StatusFailed:
		return name + "执行失败"
	case StatusCanceled:
		return name + "已取消"
	default:
		return name + "状态更新"
	}
}

func buildNotificationMessage(task *Task) string {
	var base string
	switch task.Status {
	case StatusCompleted:
		base = task.Result
	case StatusFailed, StatusCanceled:
		base = task.Error
	default:
		base = task.Result
	}

	base = strings.TrimSpace(base)
	if base == "" {
		base = "任务已结束。"
	}

	return truncateRunes(base, 180)
}

func cloneTask(task *Task) *Task {
	if task == nil {
		return nil
	}
	copied := *task
	return &copied
}

func newTaskID() string {
	var raw [8]byte
	if _, err := rand.Read(raw[:]); err != nil {
		return fmt.Sprintf("task-%d", time.Now().UnixNano())
	}
	return "task-" + hex.EncodeToString(raw[:])
}

func readOptionalFile(path string) string {
	data, err := os.ReadFile(path)
	if err != nil {
		return ""
	}
	return strings.TrimSpace(string(data))
}

func chooseMessage(values ...string) string {
	for _, value := range values {
		value = strings.TrimSpace(value)
		if value != "" {
			return value
		}
	}
	return ""
}

func truncateRunes(value string, limit int) string {
	runes := []rune(value)
	if len(runes) <= limit {
		return value
	}
	return strings.TrimSpace(string(runes[:limit])) + "..."
}
