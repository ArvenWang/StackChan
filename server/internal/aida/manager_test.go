package aida

import (
	"os"
	"path/filepath"
	"runtime"
	"testing"
	"time"
)

func TestCancelTaskImmediatelyLeavesTaskCanceled(t *testing.T) {
	t.Parallel()

	workspace := t.TempDir()
	codexBin := writeFakeCodexBin(t, "sleep 1\nprintf 'done\\n' > \"$5\"\n")

	manager := NewManager(Config{
		CodexBin:         codexBin,
		DefaultWorkspace: workspace,
		AllowedRoots:     []string{workspace},
	})

	task, err := manager.CreateTask(CreateTaskRequest{
		Title:  "test",
		Prompt: "say hi",
	})
	if err != nil {
		t.Fatalf("CreateTask() error = %v", err)
	}

	if err := manager.CancelTask(task.ID); err != nil {
		t.Fatalf("CancelTask() error = %v", err)
	}

	deadline := time.Now().Add(3 * time.Second)
	for time.Now().Before(deadline) {
		current, ok := manager.GetTask(task.ID)
		if !ok {
			t.Fatalf("task %s disappeared", task.ID)
		}
		if current.Status == StatusCanceled {
			return
		}
		time.Sleep(20 * time.Millisecond)
	}

	current, _ := manager.GetTask(task.ID)
	t.Fatalf("task status = %s, want %s", current.Status, StatusCanceled)
}

func TestListTasksReturnsNewestFirstAndAppliesLimit(t *testing.T) {
	t.Parallel()

	now := time.Now()
	manager := NewManager(Config{})
	manager.tasks["task-1"] = &Task{ID: "task-1", Title: "older", CreatedAt: now.Add(-2 * time.Minute)}
	manager.tasks["task-2"] = &Task{ID: "task-2", Title: "newer", CreatedAt: now.Add(-1 * time.Minute)}
	manager.tasks["task-3"] = &Task{ID: "task-3", Title: "newest", CreatedAt: now}

	tasks := manager.ListTasks(2)
	if len(tasks) != 2 {
		t.Fatalf("ListTasks(2) length = %d, want 2", len(tasks))
	}
	if tasks[0].ID != "task-3" || tasks[1].ID != "task-2" {
		t.Fatalf("ListTasks(2) order = [%s %s], want [task-3 task-2]", tasks[0].ID, tasks[1].ID)
	}
}

func writeFakeCodexBin(t *testing.T, scriptBody string) string {
	t.Helper()

	dir := t.TempDir()
	path := filepath.Join(dir, "codex")
	script := "#!/usr/bin/env bash\nset -euo pipefail\n" + scriptBody
	if runtime.GOOS == "windows" {
		t.Fatal("windows is not supported by this test")
	}
	if err := os.WriteFile(path, []byte(script), 0o755); err != nil {
		t.Fatalf("WriteFile(%s) error = %v", path, err)
	}
	return path
}
