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
