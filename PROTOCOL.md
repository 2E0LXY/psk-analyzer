# PSKedge remote control protocol

WebSocket, JSON text frames. Server: `RemoteControlServer`
(src/remote/RemoteControlServer.cpp) inside the PSKedge desktop app,
enabled and configured (port + auth token) in Settings > Remote. Client:
the Android app in `android/`.

No TLS. Fine on a trusted LAN; for remote/internet access, use a VPN or
SSH/WireGuard tunnel rather than exposing the port directly - the auth
token is otherwise the only protection on a link that can key a real
transmitter.

## Connecting

1. Open a WebSocket connection to `ws://<host>:<port>` (default port 8765).
2. The **first message must be** an `auth` message. Any other first
   message, or a wrong token, closes the connection.
3. On success the server replies `auth_ok`, then immediately sends the
   current `state`.

```json
{"type": "auth", "token": "<the token set in Settings>"}
```

Server replies with `{"type": "auth_ok"}` or `{"type": "auth_failed"}`
(connection is closed either way after `auth_failed`).

## Client -> server (commands)

Every command here maps 1:1 to an existing desktop action - the remote
app cannot do anything the desktop UI itself can't.

| type | fields | desktop equivalent |
|---|---|---|
| `set_band` | `band`: string, e.g. `"40m"` | clicking a band button |
| `set_afc` | `enabled`: bool | toggling the AFC button |
| `send_macro` | `text`: string (a macro template, e.g. `"CQ CQ CQ DE {MYCALL} {MYCALL} K"`) | tapping a macro button, then Send - expanded and transmitted immediately, not just inserted |
| `send_tx_text` | `text`: string (already-composed text) | typing in the composer, then Send |
| `abort_tx` | - | clicking Abort |
| `request_state` | - | (no desktop equivalent - asks the server to immediately re-send `state`) |

## Server -> client (events)

### `state`

Broadcast once a second, and immediately after `auth_ok` / any
`request_state`.

```json
{
  "type": "state",
  "band": "20m",
  "mode": "BPSK31",
  "frequencyHz": 14070000,
  "afcEnabled": false,
  "catConnected": true,
  "txActive": false,
  "snrDb": 12.5,
  "qualityPercent": 80,
  "rxLevelDb": -20.0,
  "txLevelDb": -120.0
}
```

`mode` is always `"BPSK31"` currently - matching the desktop, where the
mode selector is a roadmap list and only BPSK31 actually decodes (see
FEATURE_ROADMAP.md). `rxLevelDb`/`txLevelDb` of `-120.0` reads as
"silent/unavailable", not a real measurement at the display floor.

### `rx_text`

Sent every time new decoded text arrives - the full accumulated decode
buffer, same as the desktop's Live RX transcript (the client diffs
against what it already has, same convention as the desktop).

```json
{"type": "rx_text", "text": "CQ CQ CQ DE 2E0LXY..."}
```

### `log`

Free-text status/log line, for a simple log view - not structured
QSO data (there is no logbook yet - see FEATURE_ROADMAP.md).

```json
{"type": "log", "message": "..."}
```

### `error`

Sent (then the connection is closed) when a client violates the
protocol before authenticating - e.g. malformed JSON, or a non-`auth`
first message.

```json
{"type": "error", "message": "first message must be auth"}
```

## Versioning

No version field yet - this is a single evolving protocol between one
server and one client implementation, both in this repository. Unknown
message `type` values are ignored by the server rather than treated as
errors, so new optional message types can be added without breaking
older clients.
