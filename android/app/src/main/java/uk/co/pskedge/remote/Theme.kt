package uk.co.pskedge.remote

import androidx.compose.material3.darkColorScheme
import androidx.compose.ui.graphics.Color

// Matches MainWindow.cpp's stylesheet palette (#0b1017 background, #6ee9ff
// accent) so the remote app reads as the same product, not a generic
// Material app.
val PSKedgeBackground = Color(0xFF0B1017)
val PSKedgeSurface = Color(0xFF141B24)
val PSKedgeAccent = Color(0xFF6EE9FF)
val PSKedgeText = Color(0xFFD8F7FF)

val PSKedgeDarkColorScheme = darkColorScheme(
    background = PSKedgeBackground,
    surface = PSKedgeSurface,
    primary = PSKedgeAccent,
    onBackground = PSKedgeText,
    onSurface = PSKedgeText,
    onPrimary = Color(0xFF00202A),
)
