# PSKedge Remote (Android)

Remote control for PSKedge over the LAN (or a VPN/tunnel for remote
access) - band selection, AFC, macros, TX text, live RX transcript, and
log, all driving the exact same desktop actions. See
[`../PROTOCOL.md`](../PROTOCOL.md) for the wire format.

## Setup

1. In PSKedge on the PC: Settings > Remote, enable, set a port and an
   auth token.
2. In this app: enter the PC's LAN IP address, the port, and the same
   token.

No TLS - see PROTOCOL.md and the in-app warning in PSKedge's Settings
dialog for what that does and doesn't protect against.

## Building

```sh
cd android
./gradlew assembleDebug
```

Requires JDK 17 and the Android SDK (`ANDROID_HOME` set). CI
(`.github/workflows/build.yml`, the `android` job) builds this on every
push - that is the environment this has actually been verified to build
in; a full local Android SDK setup was not available while writing this.

## Status

First pass - band/AFC/macro/TX-text/log control and live status all
work against the real protocol (verified against a real running
`RemoteControlServer` instance, not just written to match PROTOCOL.md
on paper). Not yet done: persisting the last-used connection details
between launches, and a proper app icon beyond the shared PSKedge mark.
