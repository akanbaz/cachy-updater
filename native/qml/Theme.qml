pragma Singleton

import QtQuick

// CachyOS design tokens. Flat surfaces, cyan accent, deep blue, neutral greys.
// Spacing follows the KDE Plasma 8 / 16 / 24 grid.
QtObject {
    // Neutral greys
    readonly property color bg: "#161719"
    readonly property color deepBg: "#121315"
    readonly property color surface: "#1D1E21"
    readonly property color surfaceHover: "#26282C"
    readonly property color rowHover: "#232529"
    readonly property color border: "#2A2C30"
    readonly property color borderStrong: "#34373C"

    // CachyOS blues + cyan accent
    readonly property color kernel: "#0F4C75"
    readonly property color kernelDeep: "#0A3D62"
    readonly property color kernelBorder: "#1B6699"
    readonly property color cyan: "#00B7C2"
    readonly property color cyanHover: "#1AC7D1"
    readonly property color cyanPressed: "#009AA4"
    readonly property color cyanInk: "#062A2E"
    readonly property color cyanBg: "#0D2A2E"

    // Text
    readonly property color text: "#E7E9EB"
    readonly property color textDim: "#B4B9BE"
    readonly property color textMuted: "#868D94"
    readonly property color textFaint: "#5F656B"

    // Warm tones — warnings/errors only
    readonly property color warnBg: "#2E2513"
    readonly property color warnBorder: "#B98A32"
    readonly property color warnText: "#E4AE49"
    readonly property color errorBg: "#331A1A"
    readonly property color errorBorder: "#A84545"
    readonly property color errorText: "#E58A8A"
    readonly property color ok: "#2FBE8F"

    // Spacing grid
    readonly property int spacingSmall: 8
    readonly property int spacing: 16
    readonly property int spacingLarge: 24

    readonly property int radius: 6
    readonly property int radiusSmall: 4

    readonly property string monoFamily: "JetBrains Mono, Hack, Noto Sans Mono, monospace"
    readonly property string sansFamily: "Noto Sans, sans-serif"
}
