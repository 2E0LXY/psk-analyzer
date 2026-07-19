#pragma once

#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QString>

class QWebSocketServer;
class QWebSocket;

// WebSocket server exposing PSKedge's control surface to the Android remote
// app (see android/ and PROTOCOL.md for the client and wire format). Every
// command this accepts maps to exactly the same MainWindow methods the
// desktop UI buttons call - there is no separate/reduced code path, so the
// remote app can't drift out of sync with what the desktop actually does.
//
// Security: a single shared token, set in Settings, is required as the
// first message on every connection; connections that don't authenticate
// within a short timeout - or that send the wrong token - are closed. This
// is a real transmitter control surface, not a read-only status feed, so
// authentication is not optional. There is no TLS here - if this is
// exposed to the internet rather than just the LAN, that is only as safe
// as the token itself over a plaintext connection, which is why the
// Settings dialog says so explicitly. A VPN or an SSH/WireGuard tunnel is
// the safer way to reach this remotely; this server does not implement
// one itself.
class RemoteControlServer : public QObject {
    Q_OBJECT

public:
    explicit RemoteControlServer(QObject *parent = nullptr);
    ~RemoteControlServer() override;

    bool start(quint16 port, const QString &authToken);
    void stop();
    bool isListening() const;
    quint16 port() const;
    int connectedClientCount() const;

public slots:
    // Called by MainWindow whenever the state it already tracks changes,
    // so every connected remote sees the same live picture the desktop
    // operator does. Broadcasting an unchanged value is harmless (clients
    // just re-render the same state) - callers don't need to diff first.
    void broadcastState(const QString &band, const QString &mode, double frequencyHz, bool afcEnabled,
                         bool catConnected, bool txActive, double snrDb, int qualityPercent,
                         double rxLevelDb, double txLevelDb);
    void broadcastRxText(const QString &text);
    void broadcastLogMessage(const QString &message);

signals:
    void statusChanged(const QString &status);

    // One signal per remotely-triggerable action, each mirroring an
    // existing MainWindow slot/action exactly (see MainWindow's
    // connect() calls for these) rather than reimplementing behaviour
    // here.
    void bandChangeRequested(const QString &band);
    void afcToggleRequested(bool enabled);
    void macroRequested(const QString &text);
    void txTextRequested(const QString &text);
    void abortTxRequested();
    void stateRequested();

private slots:
    void handleNewConnection();
    void handleTextMessage(const QString &message);
    void handleDisconnected();

private:
    struct ClientInfo {
        bool authenticated = false;
    };

    void sendJson(QWebSocket *client, const QJsonObject &object);
    void closeWithError(QWebSocket *client, const QString &reason);

    QWebSocketServer *m_server = nullptr;
    QString m_authToken;
    QHash<QWebSocket *, ClientInfo> m_clients;
    // Cached so a newly-authenticated client can be sent the current
    // decode immediately (see handleTextMessage's auth handling) instead
    // of only ever seeing rx_text broadcasts that happen to arrive after
    // they connect - previously a client connecting mid-QSO, or after
    // the signal had already stopped, would see nothing until (if ever)
    // new text streamed in, which looked exactly like "no live RX" even
    // though the desktop itself had decoded text sitting right there.
    QString m_lastRxText;
};
