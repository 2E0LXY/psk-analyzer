# assets/

Image assets for the installer and in-app splash screen. Referenced by
CMakeLists.txt (CPack/WiX settings) and src/main.cpp (QSplashScreen).
All entries here are optional — CMake and main.cpp both check whether the
file exists before using it, so the build and app work fine without any of
these present.

| File | Used for | Required format |
|---|---|---|
| `splash.png` | In-app splash screen shown while the program loads | PNG, any size (scaled to fit) |
| `wix_banner.bmp` | WiX/MSI installer banner (top of each page except welcome/finish) | BMP, 493x58 px |
| `wix_dialog.bmp` | WiX/MSI welcome/finish page background | BMP, 493x312 px |
| `pskedge.ico` | Windows executable/installer icon (shown in Add/Remove Programs) | ICO, multi-resolution (16/32/48/256px) recommended |

WiX requires the banner/dialog images in those exact pixel dimensions - a
differently-sized source image needs to be cropped/resized to match, not
just dropped in as-is.

## Why WiX, not NSIS

This project used NSIS initially. Its MUI2 dynamically-loaded bitmap
macros (`MUI_HEADERIMAGE_INIT`, `MUI_WELCOMEFINISHPAGE_INIT`) failed real
Windows CI builds with "no files found" for bitmap files confirmed to
exist at the exact referenced path - reproduced across multiple CI runs,
root cause not identified from this environment (each iteration cost a
multi-minute CI round trip against a problem that would take seconds to
debug with a local Windows+NSIS setup). Switched to WiX (MSI), which is
also the more standard Windows installer format. See git history around
the CMakeLists.txt WIX/NSIS transition for the full investigation if
anyone wants to revisit NSIS with more direct debugging access.
