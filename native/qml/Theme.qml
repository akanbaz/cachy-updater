pragma Singleton

import QtQuick

// CachyOS design tokens. Adapts to system light/dark via Qt.styleHints.
// Spacing follows the KDE Plasma 8 / 16 / 24 grid.
QtObject {
    readonly property bool isDark: Qt.styleHints.colorScheme === Qt.ColorScheme.Dark

    // Surfaces
    readonly property color bg: isDark ? "#161719" : "#F4F5F6"
    readonly property color deepBg: isDark ? "#121315" : "#EAECEF"
    readonly property color surface: isDark ? "#1D1E21" : "#FFFFFF"
    readonly property color surfaceHover: isDark ? "#26282C" : "#E8EAED"
    readonly property color rowHover: isDark ? "#232529" : "#DFE3E8"
    readonly property color border: isDark ? "#2A2C30" : "#D0D4D9"
    readonly property color borderStrong: isDark ? "#34373C" : "#B8BEC6"

    // CachyOS blues + cyan accent
    readonly property color kernel: isDark ? "#0F4C75" : "#1A6FA0"
    readonly property color kernelDeep: isDark ? "#0A3D62" : "#E6F2F8"
    readonly property color kernelBorder: isDark ? "#1B6699" : "#7EB6D6"
    readonly property color cyan: "#00B7C2"
    readonly property color cyanHover: "#1AC7D1"
    readonly property color cyanPressed: "#009AA4"
    readonly property color cyanInk: isDark ? "#062A2E" : "#043033"
    readonly property color cyanBg: isDark ? "#0D2A2E" : "#D7F3F5"

    // Text
    readonly property color text: isDark ? "#E7E9EB" : "#1A1C1E"
    readonly property color textDim: isDark ? "#B4B9BE" : "#4A5158"
    readonly property color textMuted: isDark ? "#868D94" : "#6B737A"
    readonly property color textFaint: isDark ? "#5F656B" : "#8A9299"

    // Warm tones — warnings/errors only
    readonly property color warnBg: isDark ? "#2E2513" : "#FBF3E0"
    readonly property color warnBorder: isDark ? "#B98A32" : "#C4922A"
    readonly property color warnText: isDark ? "#E4AE49" : "#8A6500"
    readonly property color errorBg: isDark ? "#331A1A" : "#FBEAEA"
    readonly property color errorBorder: isDark ? "#A84545" : "#C45A5A"
    readonly property color errorText: isDark ? "#E58A8A" : "#A83232"
    readonly property color ok: "#2FBE8F"

    // Spacing grid (Plasma-aligned)
    readonly property int spacingSmall: 8
    readonly property int spacing: 16
    readonly property int spacingLarge: 24

    readonly property int radius: 6
    readonly property int radiusSmall: 4

    // Prefer Kirigami-friendly pixel sizes; ListView/icons use these.
    readonly property int iconSmall: 16
    readonly property int iconMedium: 22
    readonly property int iconLarge: 32

    readonly property string monoFamily: "JetBrains Mono, Hack, Noto Sans Mono, monospace"
    readonly property string sansFamily: "Noto Sans, sans-serif"
}
