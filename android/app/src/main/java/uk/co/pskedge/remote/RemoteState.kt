package uk.co.pskedge.remote

/** Mirrors the "state" message in PROTOCOL.md exactly - field for field. */
data class RemoteState(
    val band: String = "",
    val mode: String = "BPSK31",
    val frequencyHz: Double = 0.0,
    val afcEnabled: Boolean = false,
    val catConnected: Boolean = false,
    val txActive: Boolean = false,
    val snrDb: Double = 0.0,
    val qualityPercent: Int = 0,
    val rxLevelDb: Double = -120.0,
    val txLevelDb: Double = -120.0,
)

enum class ConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    AUTHENTICATING,
    CONNECTED,
    AUTH_FAILED,
    ERROR,
}

/** The 10 bands the desktop app's band buttons offer - see MainWindow.cpp's
 * handleBandChanged() pskDialHz map, which this must stay in sync with. */
val PSK_BANDS = listOf("160m", "80m", "40m", "30m", "20m", "17m", "15m", "12m", "10m", "6m")

/** Desktop macro buttons - copied verbatim from MainWindow.cpp's
 * buildTxPanel() macro list, which this must stay in sync with. Expanded
 * server-side by MacroEngine using the desktop's own station settings -
 * the remote app does not need to know the operator's callsign to use
 * these. */
val MACROS = listOf(
    "CQ" to "CQ CQ CQ DE {MYCALL} {MYCALL} K",
    "Answer" to "{THEIRCALL} DE {MYCALL} ",
    "Report" to "{THEIRCALL} DE {MYCALL} UR {RST} SNR {SNR} AUDIO {AUDIO_FREQ}HZ IMD {IMD}",
    "Name/QTH" to "NAME {NAME} QTH {QTH} LOC {LOCATOR}",
    "Rig" to "RIG {RADIO_MODEL} PWR {PSK_POWER}W ANT {ANTENNA_NAME}",
    "73" to "{THEIRCALL} DE {MYCALL} 73 SK",
)
