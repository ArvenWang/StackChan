# Evening Flash Checklist

This is the shortest reliable path to get `AI.AGENT + Aida Bridge` running tonight.

## 1. Start The Mac Bridge First

Use the robot's **fixed LAN IP** for notifications.
That is the most stable option. Avoid depending on mDNS names unless you have already verified them on your network.

```bash
cd /Users/nefish/Desktop/WorkSpace/Coding/StackChan/server

export AIDA_ROBOT_NOTIFY_URL=http://ROBOT_IP:7830/aida/notify
./run_aida_bridge.sh
```

Expected:

- Terminal shows `Aida Bridge listening on 0.0.0.0:7826`

Optional quick check:

```bash
python3 - <<'PY'
import urllib.request
print(urllib.request.urlopen('http://127.0.0.1:7826/health', timeout=5).read().decode())
PY
```

## 2. Open ESP-IDF Environment

This machine currently does **not** have `idf.py` in `PATH`, so before building you need a working ESP-IDF `5.5.4` environment.

Typical flow:

```bash
cd /Users/nefish/Desktop/WorkSpace/Coding/StackChan/firmware
python3 ./fetch_repos.py

# Replace this with your real ESP-IDF path
source ~/esp/esp-idf/export.sh
```

Quick check:

```bash
idf.py --version
```

## 3. Configure The Firmware

```bash
cd /Users/nefish/Desktop/WorkSpace/Coding/StackChan/firmware
idf.py menuconfig
```

Check these items:

### Language / Wake Word

- `Xiaozhi Assistant -> Language`
  Set to Chinese
- `Xiaozhi Assistant -> Wake Word Implementation Type`
  Keep custom wake word
- `Xiaozhi Assistant -> Custom Wake Word`
  `hey aida`
- `Xiaozhi Assistant -> Custom Wake Word Display`
  `Hey Aida`

### Aida Bridge

- `Xiaozhi Assistant -> Aida Bridge -> Enable local Aida notification server`
  Enabled
- `Xiaozhi Assistant -> Aida Bridge -> Aida notification server port`
  `7830`
- `Xiaozhi Assistant -> Aida Bridge -> Aida notification server bearer token`
  `change-me-too`
- `Xiaozhi Assistant -> Aida Bridge -> Aida bridge base URL`
  `http://YOUR_MAC_IP:7826`
- `Xiaozhi Assistant -> Aida Bridge -> Aida bridge bearer token`
  `change-me`

Then save defaults:

```bash
idf.py save-defconfig
```

## 4. Build And Flash

Find your serial port:

```bash
ls /dev/cu.usbmodem* /dev/cu.wchusbserial* 2>/dev/null
```

Then:

```bash
cd /Users/nefish/Desktop/WorkSpace/Coding/StackChan/firmware
idf.py -p /dev/cu.usbmodemXXXX build flash monitor
```

## 5. What To Watch For In Logs

After booting into `AI.AGENT`, you want to see:

- normal `xiaozhi` startup
- no HTTP server startup errors
- a log similar to:
  `Aida notify server started on port 7830`

If the robot reaches standby/listening normally, the `AI.AGENT` side is alive.

## 6. Stable Smoke Test

After the robot is on the LAN, keep `run_aida_bridge.sh` running on the Mac and create a task manually:

```bash
curl -X POST http://127.0.0.1:7826/v1/tasks \
  -H 'Content-Type: application/json' \
  -H 'Authorization: Bearer change-me' \
  -d '{
    "title": "Bridge smoke test",
    "prompt": "Reply with one short sentence confirming the bridge is working.",
    "workspace": "/Users/nefish/Desktop/WorkSpace/Coding/StackChan",
    "notify": true
  }'
```

Expected:

- the bridge accepts the task and returns a task id
- after task completion, the robot shows a local notification popup with sound

## 7. First Real Voice Test

Once the robot is in `AI.AGENT`, try a short command like:

`Hey Aida，帮我让 Codex 检查这个项目里唤醒词是在哪里配置的。`

Expected:

- the robot can call `self.desktop.codex_run`
- the Mac bridge starts a task
- task completion comes back as a local notification

## 8. Matching Values

These values must match on both sides:

- robot `AIDA_BRIDGE_AUTH_TOKEN` == Mac `AIDA_BRIDGE_TOKEN`
- robot `AIDA_NOTIFY_SERVER_TOKEN` == Mac `AIDA_ROBOT_NOTIFY_TOKEN`
- robot `AIDA_BRIDGE_BASE_URL` points to the Mac
- Mac `AIDA_ROBOT_NOTIFY_URL` points to the robot

## 9. If Something Fails

- `Bridge health OK, but robot never notifies`
  Usually wrong robot IP or wrong notify token
- `Robot can talk, but cannot start Codex tasks`
  Usually wrong Mac IP or wrong bridge token
- `Firmware does not build`
  Usually ESP-IDF environment problem, not Aida Bridge logic
