package uk.co.pskedge.remote

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import org.json.JSONObject
import java.util.concurrent.TimeUnit

/**
 * WebSocket client implementing PROTOCOL.md exactly - see that file for
 * the wire format this speaks. One instance per connection; call
 * [connect] to (re)start, [disconnect] to tear down.
 */
class RemoteClient {
    private val client = OkHttpClient.Builder()
        .pingInterval(20, TimeUnit.SECONDS)
        .build()
    private var socket: WebSocket? = null

    private val _status = MutableStateFlow(ConnectionStatus.DISCONNECTED)
    val status: StateFlow<ConnectionStatus> = _status.asStateFlow()

    private val _state = MutableStateFlow(RemoteState())
    val state: StateFlow<RemoteState> = _state.asStateFlow()

    private val _rxText = MutableStateFlow("")
    val rxText: StateFlow<String> = _rxText.asStateFlow()

    private val _logLines = MutableStateFlow<List<String>>(emptyList())
    val logLines: StateFlow<List<String>> = _logLines.asStateFlow()

    private var pendingToken: String = ""

    fun connect(host: String, port: Int, token: String) {
        disconnect()
        pendingToken = token
        _status.value = ConnectionStatus.CONNECTING
        val request = Request.Builder().url("ws://$host:$port").build()
        socket = client.newWebSocket(request, listener)
    }

    fun disconnect() {
        socket?.close(1000, "client disconnect")
        socket = null
        _status.value = ConnectionStatus.DISCONNECTED
    }

    fun setBand(band: String) = sendCommand(JSONObject().put("type", "set_band").put("band", band))
    fun setAfc(enabled: Boolean) = sendCommand(JSONObject().put("type", "set_afc").put("enabled", enabled))
    fun sendMacro(text: String) = sendCommand(JSONObject().put("type", "send_macro").put("text", text))
    fun sendTxText(text: String) = sendCommand(JSONObject().put("type", "send_tx_text").put("text", text))
    fun abortTx() = sendCommand(JSONObject().put("type", "abort_tx"))
    fun requestState() = sendCommand(JSONObject().put("type", "request_state"))

    private fun sendCommand(json: JSONObject) {
        // Commands sent before authentication complete are silently
        // dropped rather than queued - the server would close the
        // connection anyway (see RemoteControlServer::handleTextMessage,
        // "first message must be auth"), and queuing risks firing a
        // stale command (e.g. an old TX text) the instant auth succeeds,
        // which is a worse failure mode for something that keys a
        // transmitter than just not sending it.
        if (_status.value != ConnectionStatus.CONNECTED) {
            return
        }
        socket?.send(json.toString())
    }

    private val listener = object : WebSocketListener() {
        override fun onOpen(webSocket: WebSocket, response: Response) {
            _status.value = ConnectionStatus.AUTHENTICATING
            val auth = JSONObject().put("type", "auth").put("token", pendingToken)
            webSocket.send(auth.toString())
        }

        override fun onMessage(webSocket: WebSocket, text: String) {
            val obj = try {
                JSONObject(text)
            } catch (e: Exception) {
                return
            }
            when (obj.optString("type")) {
                "auth_ok" -> _status.value = ConnectionStatus.CONNECTED
                "auth_failed" -> _status.value = ConnectionStatus.AUTH_FAILED
                "state" -> _state.value = RemoteState(
                    band = obj.optString("band"),
                    mode = obj.optString("mode", "BPSK31"),
                    frequencyHz = obj.optDouble("frequencyHz", 0.0),
                    afcEnabled = obj.optBoolean("afcEnabled"),
                    catConnected = obj.optBoolean("catConnected"),
                    txActive = obj.optBoolean("txActive"),
                    snrDb = obj.optDouble("snrDb", 0.0),
                    qualityPercent = obj.optInt("qualityPercent", 0),
                    rxLevelDb = obj.optDouble("rxLevelDb", -120.0),
                    txLevelDb = obj.optDouble("txLevelDb", -120.0),
                )
                "rx_text" -> _rxText.value = obj.optString("text")
                "log" -> _logLines.value = (_logLines.value + obj.optString("message")).takeLast(200)
            }
        }

        override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
            _status.value = ConnectionStatus.ERROR
        }

        override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
            if (_status.value != ConnectionStatus.ERROR) {
                _status.value = ConnectionStatus.DISCONNECTED
            }
        }
    }
}
