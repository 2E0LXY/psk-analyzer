package uk.co.pskedge.remote

import androidx.lifecycle.ViewModel
import kotlinx.coroutines.flow.StateFlow

class RemoteViewModel : ViewModel() {
    private val client = RemoteClient()

    val status: StateFlow<ConnectionStatus> = client.status
    val state: StateFlow<RemoteState> = client.state
    val rxText: StateFlow<String> = client.rxText
    val logLines: StateFlow<List<String>> = client.logLines

    fun connect(host: String, port: Int, token: String) = client.connect(host, port, token)
    fun disconnect() = client.disconnect()
    fun setBand(band: String) = client.setBand(band)
    fun setAfc(enabled: Boolean) = client.setAfc(enabled)
    fun sendMacro(text: String) = client.sendMacro(text)
    fun sendTxText(text: String) = client.sendTxText(text)
    fun abortTx() = client.abortTx()

    override fun onCleared() {
        client.disconnect()
        super.onCleared()
    }
}
