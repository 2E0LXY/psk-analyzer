package uk.co.pskedge.remote

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp

@Composable
fun ConnectScreen(viewModel: RemoteViewModel, status: ConnectionStatus) {
    var host by remember { mutableStateOf("") }
    var port by remember { mutableStateOf("8765") }
    var token by remember { mutableStateOf("") }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Text("PSKedge Remote", style = androidx.compose.material3.MaterialTheme.typography.headlineMedium)

        androidx.compose.foundation.layout.Spacer(modifier = Modifier.padding(12.dp))

        OutlinedTextField(
            value = host,
            onValueChange = { host = it },
            label = { Text("PC address (e.g. 192.168.1.20)") },
            modifier = Modifier.fillMaxWidth(),
        )
        OutlinedTextField(
            value = port,
            onValueChange = { port = it.filter(Char::isDigit) },
            label = { Text("Port") },
            keyboardOptions = androidx.compose.foundation.text.KeyboardOptions(keyboardType = KeyboardType.Number),
            modifier = Modifier.fillMaxWidth(),
        )
        OutlinedTextField(
            value = token,
            onValueChange = { token = it },
            label = { Text("Auth token (set in PSKedge Settings > Remote)") },
            visualTransformation = PasswordVisualTransformation(),
            modifier = Modifier.fillMaxWidth(),
        )

        androidx.compose.foundation.layout.Spacer(modifier = Modifier.padding(8.dp))

        Button(
            onClick = {
                val portNumber = port.toIntOrNull() ?: 8765
                viewModel.connect(host.trim(), portNumber, token)
            },
            enabled = host.isNotBlank() && token.isNotBlank() &&
                status != ConnectionStatus.CONNECTING && status != ConnectionStatus.AUTHENTICATING,
        ) {
            Text(
                when (status) {
                    ConnectionStatus.CONNECTING -> "Connecting..."
                    ConnectionStatus.AUTHENTICATING -> "Authenticating..."
                    else -> "Connect"
                }
            )
        }

        when (status) {
            ConnectionStatus.AUTH_FAILED -> Text("Wrong token", color = androidx.compose.ui.graphics.Color.Red)
            ConnectionStatus.ERROR -> Text(
                "Connection failed - check address, port, and that the PC's Remote setting is enabled",
                color = androidx.compose.ui.graphics.Color.Red,
            )
            else -> {}
        }
    }
}
