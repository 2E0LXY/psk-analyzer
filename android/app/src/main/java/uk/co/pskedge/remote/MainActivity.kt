package uk.co.pskedge.remote

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.lifecycle.viewmodel.compose.viewModel

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme(colorScheme = PSKedgeDarkColorScheme) {
                Surface(modifier = Modifier.fillMaxSize()) {
                    PskEdgeRemoteApp()
                }
            }
        }
    }
}

@Composable
fun PskEdgeRemoteApp(viewModel: RemoteViewModel = viewModel()) {
    val status by viewModel.status.collectAsState()

    if (status == ConnectionStatus.CONNECTED) {
        ControlScreen(viewModel)
    } else {
        ConnectScreen(viewModel, status)
    }
}
