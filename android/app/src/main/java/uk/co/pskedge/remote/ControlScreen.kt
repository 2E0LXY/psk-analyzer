package uk.co.pskedge.remote

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import kotlin.math.roundToInt

@Composable
fun ControlScreen(viewModel: RemoteViewModel) {
    val state by viewModel.state.collectAsState()
    val rxText by viewModel.rxText.collectAsState()
    val logLines by viewModel.logLines.collectAsState()
    var composerText by remember { mutableStateOf("") }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
    ) {
        Text(
            "%.6f MHz".format(state.frequencyHz / 1_000_000.0),
            style = MaterialTheme.typography.headlineLarge,
            color = PSKedgeAccent,
        )
        Text(
            "${state.band}  -  ${state.mode}  -  ${if (state.catConnected) "CAT connected" else "CAT disconnected"}",
            style = MaterialTheme.typography.bodyMedium,
        )

        Spacer(modifier = Modifier.height(12.dp))

        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("AFC")
            Switch(checked = state.afcEnabled, onCheckedChange = { viewModel.setAfc(it) })
        }

        Spacer(modifier = Modifier.height(8.dp))
        Text("Band", style = MaterialTheme.typography.labelLarge)
        LazyVerticalGrid(
            columns = GridCells.Fixed(5),
            modifier = Modifier.height(96.dp),
        ) {
            items(PSK_BANDS) { band ->
                val selected = band == state.band
                if (selected) {
                    Button(onClick = { viewModel.setBand(band) }, modifier = Modifier.padding(2.dp)) {
                        Text(band)
                    }
                } else {
                    OutlinedButton(onClick = { viewModel.setBand(band) }, modifier = Modifier.padding(2.dp)) {
                        Text(band)
                    }
                }
            }
        }

        Spacer(modifier = Modifier.height(12.dp))

        // Poor-to-good colour scale, same convention as the desktop's
        // LevelMeter widget (LevelMeter.cpp): red at 0, green at 1.0.
        MeterRow("SNR", fractionForRange(state.snrDb, -10.0, 30.0), "%.1f dB".format(state.snrDb))
        MeterRow("Decode quality", state.qualityPercent / 100.0, "${state.qualityPercent}%")
        MeterRow("RX audio", fractionForRange(state.rxLevelDb, -60.0, 0.0), "%.0f dBFS".format(state.rxLevelDb))
        MeterRow(
            "TX audio",
            if (state.txActive) fractionForRange(state.txLevelDb, -30.0, 0.0) else 0f,
            if (state.txActive) "%.0f dBFS".format(state.txLevelDb) else "--",
        )

        Spacer(modifier = Modifier.height(12.dp))
        Text("Live RX", style = MaterialTheme.typography.labelLarge)
        Card(modifier = Modifier.fillMaxWidth().height(100.dp)) {
            Text(
                rxText.takeLast(2000),
                modifier = Modifier.padding(8.dp).verticalScroll(rememberScrollState()),
                style = MaterialTheme.typography.bodySmall,
            )
        }

        Spacer(modifier = Modifier.height(12.dp))
        Text("Macros - tap to send immediately", style = MaterialTheme.typography.labelLarge)
        LazyVerticalGrid(columns = GridCells.Fixed(3), modifier = Modifier.height(96.dp)) {
            items(MACROS) { (label, template) ->
                OutlinedButton(onClick = { viewModel.sendMacro(template) }, modifier = Modifier.padding(2.dp)) {
                    Text(label)
                }
            }
        }

        Spacer(modifier = Modifier.height(8.dp))
        OutlinedTextField(
            value = composerText,
            onValueChange = { composerText = it },
            label = { Text("Compose and send") },
            modifier = Modifier.fillMaxWidth(),
            keyboardOptions = KeyboardOptions.Default,
        )
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Button(
                onClick = {
                    viewModel.sendTxText(composerText)
                    composerText = ""
                },
                enabled = composerText.isNotBlank(),
            ) { Text("Send") }
            OutlinedButton(
                onClick = { viewModel.abortTx() },
                enabled = state.txActive,
            ) { Text("Abort") }
        }

        Spacer(modifier = Modifier.height(12.dp))
        Text("Log", style = MaterialTheme.typography.labelLarge)
        Card(modifier = Modifier.fillMaxWidth().height(80.dp)) {
            Column(modifier = Modifier.padding(8.dp).verticalScroll(rememberScrollState())) {
                logLines.takeLast(50).forEach { line ->
                    Text(line, style = MaterialTheme.typography.bodySmall)
                }
            }
        }
    }
}

@Composable
private fun MeterRow(label: String, fraction: Float, valueText: String) {
    Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth().padding(vertical = 2.dp)) {
        Text(label, modifier = Modifier.padding(end = 8.dp))
        LinearProgressIndicator(
            progress = { fraction.coerceIn(0f, 1f) },
            modifier = Modifier.weight(1f),
            color = colorForFraction(fraction),
        )
        Text(valueText, modifier = Modifier.padding(start = 8.dp))
    }
}

/** Same red -> amber -> green scale as the desktop's LevelMeter::colorForFraction. */
private fun colorForFraction(fraction: Float): Color {
    val f = fraction.coerceIn(0f, 1f)
    return if (f < 0.5f) {
        val t = f / 0.5f
        Color(220, (60 + t * (200 - 60)).roundToInt(), 60)
    } else {
        val t = (f - 0.5f) / 0.5f
        Color((220 - t * (220 - 90)).roundToInt(), 200, (60 + t * (150 - 60)).roundToInt())
    }
}

private fun fractionForRange(value: Double, min: Double, max: Double): Float {
    if (max <= min) return 0f
    return ((value - min) / (max - min)).coerceIn(0.0, 1.0).toFloat()
}
