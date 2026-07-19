#include "RemoteControlServer.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>
#include <QWebSocketServer>

RemoteControlServer::RemoteControlServer(QObject *parent)
    : QObject(parent)
{
}

RemoteControlServer::~RemoteControlServer()
{
    stop();
}

bool RemoteControlServer::start(quint16 port, const QString &authToken)
{
    stop();
    m_authToken = authToken;

    m_server = new QWebSocketServer(QStringLiteral("PSKedge Remote"), QWebSocketServer::NonSecureMode, this);
    if (!m_server->listen(QHostAddress::Any, port)) {
        emit statusChanged(QStringLiteral("Remote control: failed to listen on port %1 (%2)")
                                .arg(port)
                                .arg(m_server->errorString()));
        m_server->deleteLater();
        m_server = nullptr;
        return false;
    }

    connect(m_server, &QWebSocketServer::newConnection, this, &RemoteControlServer::handleNewConnection);
    emit statusChanged(QStringLiteral("Remote control: listening on port %1").arg(port));
    return true;
}

void RemoteControlServer::stop()
{
    if (!m_server) {
        return;
    }
    for (QWebSocket *client : m_clients.keys()) {
        client->close();
    }
    m_clients.clear();
    m_server->close();
    m_server->deleteLater();
    m_server = nullptr;
    emit statusChanged(QStringLiteral("Remote control: stopped"));
}

bool RemoteControlServer::isListening() const
{
    return m_server && m_server->isListening();
}

quint16 RemoteControlServer::port() const
{
    return m_server ? m_server->serverPort() : 0;
}

int RemoteControlServer::connectedClientCount() const
{
    return m_clients.size();
}

void RemoteControlServer::handleNewConnection()
{
    QWebSocket *client = m_server->nextPendingConnection();
    if (!client) {
        return;
    }
    m_clients.insert(client, ClientInfo{});
    connect(client, &QWebSocket::textMessageReceived, this, &RemoteControlServer::handleTextMessage);
    connect(client, &QWebSocket::disconnected, this, &RemoteControlServer::handleDisconnected);
    emit statusChanged(QStringLiteral("Remote control: client connected from %1, awaiting auth")
                            .arg(client->peerAddress().toString()));
}

void RemoteControlServer::handleTextMessage(const QString &message)
{
    auto *client = qobject_cast<QWebSocket *>(sender());
    if (!client || !m_clients.contains(client)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        closeWithError(client, QStringLiteral("malformed JSON"));
        return;
    }
    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();

    ClientInfo &info = m_clients[client];
    if (!info.authenticated) {
        // The first message from any client MUST be auth, with no
        // exceptions - a client that sends a command before proving it
        // has the token gets disconnected, not quietly ignored, so a
        // misbehaving/malicious client can't probe the protocol without
        // the token.
        if (type != QLatin1String("auth")) {
            closeWithError(client, QStringLiteral("first message must be auth"));
            return;
        }
        if (m_authToken.isEmpty() || obj.value(QStringLiteral("token")).toString() != m_authToken) {
            QJsonObject reply;
            reply[QStringLiteral("type")] = QStringLiteral("auth_failed");
            sendJson(client, reply);
            client->close();
            return;
        }
        info.authenticated = true;
        QJsonObject reply;
        reply[QStringLiteral("type")] = QStringLiteral("auth_ok");
        sendJson(client, reply);
        if (!m_lastRxText.isEmpty()) {
            QJsonObject rxObj;
            rxObj[QStringLiteral("type")] = QStringLiteral("rx_text");
            rxObj[QStringLiteral("text")] = m_lastRxText;
            sendJson(client, rxObj);
        }
        emit statusChanged(QStringLiteral("Remote control: client %1 authenticated")
                                .arg(client->peerAddress().toString()));
        emit stateRequested();
        return;
    }

    // Every command here maps 1:1 to an existing MainWindow action - see
    // the header comment. This function only parses and forwards; it
    // never duplicates what those actions actually do.
    if (type == QLatin1String("set_band")) {
        emit bandChangeRequested(obj.value(QStringLiteral("band")).toString());
    } else if (type == QLatin1String("set_afc")) {
        emit afcToggleRequested(obj.value(QStringLiteral("enabled")).toBool());
    } else if (type == QLatin1String("send_macro")) {
        emit macroRequested(obj.value(QStringLiteral("text")).toString());
    } else if (type == QLatin1String("send_tx_text")) {
        emit txTextRequested(obj.value(QStringLiteral("text")).toString());
    } else if (type == QLatin1String("abort_tx")) {
        emit abortTxRequested();
    } else if (type == QLatin1String("request_state")) {
        emit stateRequested();
    }
    // Unknown message types are ignored rather than closing the
    // connection - lets the protocol grow (new optional message types)
    // without old servers rejecting newer clients outright.
}

void RemoteControlServer::handleDisconnected()
{
    auto *client = qobject_cast<QWebSocket *>(sender());
    if (!client) {
        return;
    }
    m_clients.remove(client);
    client->deleteLater();
    emit statusChanged(QStringLiteral("Remote control: client disconnected (%1 remaining)").arg(m_clients.size()));
}

void RemoteControlServer::sendJson(QWebSocket *client, const QJsonObject &object)
{
    client->sendTextMessage(QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
}

void RemoteControlServer::closeWithError(QWebSocket *client, const QString &reason)
{
    QJsonObject reply;
    reply[QStringLiteral("type")] = QStringLiteral("error");
    reply[QStringLiteral("message")] = reason;
    sendJson(client, reply);
    client->close();
}

void RemoteControlServer::broadcastState(const QString &band, const QString &mode, double frequencyHz,
                                          bool afcEnabled, bool catConnected, bool txActive, double snrDb,
                                          int qualityPercent, double rxLevelDb, double txLevelDb)
{
    QJsonObject obj;
    obj[QStringLiteral("type")] = QStringLiteral("state");
    obj[QStringLiteral("band")] = band;
    obj[QStringLiteral("mode")] = mode;
    obj[QStringLiteral("frequencyHz")] = frequencyHz;
    obj[QStringLiteral("afcEnabled")] = afcEnabled;
    obj[QStringLiteral("catConnected")] = catConnected;
    obj[QStringLiteral("txActive")] = txActive;
    obj[QStringLiteral("snrDb")] = snrDb;
    obj[QStringLiteral("qualityPercent")] = qualityPercent;
    obj[QStringLiteral("rxLevelDb")] = rxLevelDb;
    obj[QStringLiteral("txLevelDb")] = txLevelDb;

    for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); ++it) {
        if (it.value().authenticated) {
            sendJson(it.key(), obj);
        }
    }
}

void RemoteControlServer::broadcastRxText(const QString &text)
{
    m_lastRxText = text;
    QJsonObject obj;
    obj[QStringLiteral("type")] = QStringLiteral("rx_text");
    obj[QStringLiteral("text")] = text;
    for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); ++it) {
        if (it.value().authenticated) {
            sendJson(it.key(), obj);
        }
    }
}

void RemoteControlServer::broadcastLogMessage(const QString &message)
{
    QJsonObject obj;
    obj[QStringLiteral("type")] = QStringLiteral("log");
    obj[QStringLiteral("message")] = message;
    for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); ++it) {
        if (it.value().authenticated) {
            sendJson(it.key(), obj);
        }
    }
}
