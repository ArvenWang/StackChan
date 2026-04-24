#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

find_go_bin() {
  if [[ -n "${GO_BIN:-}" ]]; then
    printf '%s\n' "${GO_BIN}"
    return 0
  fi

  if command -v go >/dev/null 2>&1; then
    command -v go
    return 0
  fi

  if [[ -x "/Volumes/CSD9ZOU/chromium/src/third_party/dawn/tools/golang/mac-arm64/bin/go" ]]; then
    printf '%s\n' "/Volumes/CSD9ZOU/chromium/src/third_party/dawn/tools/golang/mac-arm64/bin/go"
    return 0
  fi

  return 1
}

find_codex_bin() {
  if [[ -n "${AIDA_CODEX_BIN:-}" ]]; then
    printf '%s\n' "${AIDA_CODEX_BIN}"
    return 0
  fi

  if command -v codex >/dev/null 2>&1; then
    command -v codex
    return 0
  fi

  if [[ -x "/Applications/Codex.app/Contents/Resources/codex" ]]; then
    printf '%s\n' "/Applications/Codex.app/Contents/Resources/codex"
    return 0
  fi

  return 1
}

if ! GO_BIN="$(find_go_bin)"; then
  echo "Cannot find a usable Go binary. Set GO_BIN=/absolute/path/to/go and retry." >&2
  exit 1
fi

if ! AIDA_CODEX_BIN="$(find_codex_bin)"; then
  echo "Cannot find Codex CLI. Set AIDA_CODEX_BIN=/absolute/path/to/codex and retry." >&2
  exit 1
fi

export AIDA_CODEX_BIN
export AIDA_BRIDGE_ADDR="${AIDA_BRIDGE_ADDR:-0.0.0.0:7826}"
export AIDA_BRIDGE_TOKEN="${AIDA_BRIDGE_TOKEN:-change-me}"
export AIDA_DEFAULT_WORKSPACE="${AIDA_DEFAULT_WORKSPACE:-${REPO_ROOT}}"
export AIDA_ALLOWED_WORKSPACES="${AIDA_ALLOWED_WORKSPACES:-$(cd "${REPO_ROOT}/.." && pwd)}"
export AIDA_ROBOT_NOTIFY_TOKEN="${AIDA_ROBOT_NOTIFY_TOKEN:-change-me-too}"

if [[ -z "${AIDA_ROBOT_NOTIFY_URL:-}" ]]; then
  cat >&2 <<'EOF'
AIDA_ROBOT_NOTIFY_URL is required.

Use the robot's fixed LAN IP for the most stable setup, for example:
  export AIDA_ROBOT_NOTIFY_URL=http://192.168.31.88:7830/aida/notify

Then rerun this script.
EOF
  exit 1
fi

echo "Starting Aida Bridge"
echo "  GO_BIN=${GO_BIN}"
echo "  AIDA_CODEX_BIN=${AIDA_CODEX_BIN}"
echo "  AIDA_BRIDGE_ADDR=${AIDA_BRIDGE_ADDR}"
echo "  AIDA_DEFAULT_WORKSPACE=${AIDA_DEFAULT_WORKSPACE}"
echo "  AIDA_ALLOWED_WORKSPACES=${AIDA_ALLOWED_WORKSPACES}"
echo "  AIDA_ROBOT_NOTIFY_URL=${AIDA_ROBOT_NOTIFY_URL}"

cd "${SCRIPT_DIR}"
exec "${GO_BIN}" run ./cmd/aida_bridge
