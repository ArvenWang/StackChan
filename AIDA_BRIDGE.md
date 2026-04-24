# Aida Bridge

`Aida Bridge` is the stable desktop automation path for StackChan `AI.AGENT` mode.

It is designed around two rules:

1. The Mac bridge only uses the supported `codex exec` interface.
2. The robot only uses local LAN control plus `xiaozhi` MCP tools.

Because of that, it does **not** depend on private Codex IDE state or the `AVATAR` app.

## What It Can Do

- Let the robot call Codex tasks through MCP tools in `AI.AGENT`
- Keep task state on the Mac side (`queued/running/completed/failed/canceled`)
- Push reliable completion notifications back to the robot over LAN

## Stable Scope

This implementation tracks **tasks started through Aida Bridge**.

It does **not** try to scrape or mirror arbitrary in-progress Codex Desktop sessions, because that is not a stable public integration point.

## Mac Bridge

Entry point:

```bash
cd /Users/nefish/Desktop/WorkSpace/Coding/StackChan/server
go run ./cmd/aida_bridge
```

Supported environment variables:

- `AIDA_BRIDGE_ADDR`
  Default: `127.0.0.1:7826`
- `AIDA_BRIDGE_TOKEN`
  Optional bearer token required by robot MCP calls
- `AIDA_CODEX_BIN`
  Default: `codex`
- `AIDA_DEFAULT_WORKSPACE`
  Optional default workspace when the robot does not specify one
- `AIDA_ALLOWED_WORKSPACES`
  Optional comma-separated allowlist of workspace roots
- `AIDA_ROBOT_NOTIFY_URL`
  Preferred: `http://ROBOT_IP:7830/aida/notify`
- `AIDA_ROBOT_NOTIFY_TOKEN`
  Optional bearer token for robot notification POSTs

Example:

```bash
export AIDA_BRIDGE_ADDR=0.0.0.0:7826
export AIDA_BRIDGE_TOKEN=change-me
export AIDA_DEFAULT_WORKSPACE=/Users/nefish/Desktop/WorkSpace/Coding/StackChan
export AIDA_ALLOWED_WORKSPACES=/Users/nefish/Desktop/WorkSpace/Coding
export AIDA_ROBOT_NOTIFY_URL=http://ROBOT_IP:7830/aida/notify
export AIDA_ROBOT_NOTIFY_TOKEN=change-me-too

go run ./cmd/aida_bridge
```

## Robot Firmware

New Kconfig items:

- `AIDA_NOTIFY_SERVER_ENABLED`
- `AIDA_NOTIFY_SERVER_PORT`
- `AIDA_NOTIFY_SERVER_TOKEN`
- `AIDA_BRIDGE_BASE_URL`
- `AIDA_BRIDGE_AUTH_TOKEN`

Suggested values:

- `AIDA_NOTIFY_SERVER_ENABLED=y`
- `AIDA_NOTIFY_SERVER_PORT=7830`
- `AIDA_NOTIFY_SERVER_TOKEN=<same as AIDA_ROBOT_NOTIFY_TOKEN>`
- `AIDA_BRIDGE_BASE_URL=http://your-mac.local:7826`
- `AIDA_BRIDGE_AUTH_TOKEN=<same as AIDA_BRIDGE_TOKEN>`

The robot notification endpoint is:

```text
POST /aida/notify
GET  /aida/health
```

## MCP Tools Added To AI.AGENT

- `self.desktop.codex_run`
- `self.desktop.codex_list`
- `self.desktop.codex_status`
- `self.desktop.codex_cancel`

Typical flow:

1. You say a task to the robot in `AI.AGENT`
2. The robot calls `self.desktop.codex_run`
3. Aida Bridge launches `codex exec`
4. When the task finishes, Aida Bridge POSTs to the robot's local `/aida/notify`
5. The robot shows a local reminder-style notification with sound

## HTTP API

Create task:

```bash
curl -X POST http://127.0.0.1:7826/v1/tasks \
  -H 'Content-Type: application/json' \
  -H 'Authorization: Bearer change-me' \
  -d '{
    "title": "Review StackChan wake word code",
    "prompt": "Inspect the wake word implementation and summarize where it is configured.",
    "workspace": "/Users/nefish/Desktop/WorkSpace/Coding/StackChan",
    "notify": true
  }'
```

Get task:

```bash
curl -H 'Authorization: Bearer change-me' \
  http://127.0.0.1:7826/v1/tasks/<task-id>
```

List recent tasks:

```bash
curl -H 'Authorization: Bearer change-me' \
  'http://127.0.0.1:7826/v1/tasks?limit=5'
```

Cancel task:

```bash
curl -X POST -H 'Authorization: Bearer change-me' \
  http://127.0.0.1:7826/v1/tasks/<task-id>/cancel
```
