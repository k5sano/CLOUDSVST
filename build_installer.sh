#!/bin/bash

# CloudsCOSMOS Installer Build Script
# Creates a .pkg installer for VST3 and AU plugins

set -e

VERSION="1.0.0"
BUILD_DIR="$(pwd)/build"
INSTALLER_DIR="$(pwd)/installer"
PKG_DIR="$INSTALLER_DIR/pkg"

# Clean and create installer directory
rm -rf "$INSTALLER_DIR"
mkdir -p "$PKG_DIR"

echo "Building CloudsCOSMOS..."
cd "$BUILD_DIR"
cmake --build . --target CloudsCOSMOS_VST3 CloudsCOSMOS_AU CloudsCOSMOS_Standalone -j 8

echo "Creating installer package structure..."

# Create root directory structure for package
mkdir -p "$PKG_DIR/root/Library/Audio/Plug-Ins/VST3"
mkdir -p "$PKG_DIR/root/Library/Audio/Plug-Ins/Components"
mkdir -p "$PKG_DIR/root/Applications"

# Copy built plugins
cp -R "$BUILD_DIR/CloudsCOSMOS_artefacts/Debug/VST3/CloudsCOSMOS.vst3" \
      "$PKG_DIR/root/Library/Audio/Plug-Ins/VST3/"

cp -R "$BUILD_DIR/CloudsCOSMOS_artefacts/Debug/AU/CloudsCOSMOS.component" \
      "$PKG_DIR/root/Library/Audio/Plug-Ins/Components/"

# Copy standalone app
cp -R "$BUILD_DIR/CloudsCOSMOS_artefacts/Debug/Standalone/CloudsCOSMOS.app" \
      "$PKG_DIR/root/Applications/"

# Copy background images to Documents
mkdir -p "$PKG_DIR/root/Documents/CloudsCOSMOS"
cp /Users/sanokeigo/Pictures/PICS/CLOUDSCOSMOSPICS/*.png \
   "$PKG_DIR/root/Documents/CloudsCOSMOS/"

# Create README
cat > "$PKG_DIR/root/Documents/CloudsCOSMOS/README.txt" << 'EOF'
CloudsCOSMOS v1.0.0
Granular Texture Synthesis

Based on Mutable Instruments Clouds by Émilie Gillet (MIT License)

## Installation
- VST3: ~/Library/Audio/Plug-Ins/VST3/CloudsCOSMOS.vst3
- AU: ~/Library/Audio/Plug-Ins/Components/CloudsCOSMOS.component
- Standalone: /Applications/CloudsCOSMOS.app

## Background Images
The included background images can be loaded via the "BG Image" button.

## Controls
- Main knobs (Blue): Position, Size, Pitch, Density, Texture
- Blend knobs (Green): Dry/Wet, Stereo Spread, Feedback, Reverb
- Engine knobs (Pink): Eng.Trim, Eng.Gain, Limiter
- Freeze button (Orange, square): Freeze the buffer
- Trigger button: Trigger grain playback
- BG Image / Save / Load buttons: Load background image and save/load presets

## Visualization
- Lissajous figure shows stereo correlation (bottom-right)
- Signal meters A-F show levels at various points

## Presets
- Use "Save" to save current settings to a .preset file
- Use "Load" to load a saved preset
- Presets include background image path

© 2025 K5SANO
EOF

# Build component package
echo "Building component package..."
pkgbuild --root "$PKG_DIR/root" \
         --identifier com.K5SANO.CloudsCOSMOS \
         --version "$VERSION" \
         --install-location / \
         "$PKG_DIR/CloudsCOSMOS.pkg"

# Create distribution XML
cat > "$PKG_DIR/distribution.xml" << 'EOFX'
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>CloudsCOSMOS v1.0.0</title>
    <background file="background.png" alignment="center" scaling="tofit"/>
    <welcome file="welcome.html"/>
    <license file="license.txt" mime-type="text/plain"/>
    <options customize="allow" allow-external-scripts="false" rootVolumeOnly="true"/>
    <choices-outline>
        <line choice="default">CloudsCOSMOS Complete Installation</line>
    </choices-outline>
    <choice id="default" title="Install CloudsCOSMOS" enabled="true" selected="true" visible="true" start_selected="true" start_enabled="true">
        <pkg-ref id="com.K5SANO.CloudsCOSMOS"/>
    </choice>
    <pkg-ref id="com.K5SANO.CloudsCOSMOS" version="1.0.0" onConclusion="none">CloudsCOSMOS</pkg-ref>
</installer-gui-script>
EOFX

# Create welcome HTML
cat > "$PKG_DIR/welcome.html" << 'EOF'
<html>
<head>
<style>
body { font-family: -apple-system; margin: 20px; color: #333; }
h1 { color: #1a1a2e; }
h2 { color: #4a9eff; }
</style>
</head>
<body>
<h1>CloudsCOSMOS v1.0.0</h1>
<h2>Granular Texture Synthesis</h2>
<p>Welcome to CloudsCOSMOS! This plugin brings you the legendary Clouds granular processor by Mutable Instruments.</p>
<p><b>Features:</b></p>
<ul>
<li>4 playback modes: Granular, Stretch, Looping Delay, Spectral</li>
<li>Stereo input/output with real-time visualization</li>
<li>Lissajous figure for stereo correlation display</li>
<li>Adjustable output limiter</li>
<li>Background image support</li>
<li>Preset save/load system</li>
</ul>
<p><b>Installation:</b></p>
<ul>
<li>VST3 Plugin: ~/Library/Audio/Plug-Ins/VST3/</li>
<li>AU Plugin: ~/Library/Audio/Plug-Ins/Components/</li>
<li>Standalone App: /Applications/</li>
</ul>
<p><b>Credits:</b></p>
<p>Based on Mutable Instruments Clouds by Émilie Gillet (MIT License)</p>
<p>© 2025 K5SANO</p>
</body>
</html>
EOF

# Create license
cat > "$PKG_DIR/license.txt" << 'EOF'
MIT License

Copyright (c) 2025 K5SANO

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

Clouds DSP Original Copyright (c) 2013-2019 Emilie Gillet
https://github.com/pichenettes/eurorack/tree/master/clouds
EOF

# Copy background for installer
mkdir -p "$PKG_DIR/resources"
cp /Users/sanokeigo/Pictures/PICS/CLOUDSCOSMOSPICS/image_4ab84d95-26ba-4806-9306-3453bbea7a95.png \
   "$PKG_DIR/resources/background.png"

# Build final distribution package
echo "Building distribution package..."
productbuild --distribution "$PKG_DIR/distribution.xml" \
             --package-path "$PKG_DIR" \
             --resources "$PKG_DIR/resources" \
             "$INSTALLER_DIR/CloudsCOSMOS-v1.0.0-Mac.pkg"

echo "Installer created: $INSTALLER_DIR/CloudsCOSMOS-v1.0.0-Mac.pkg"
echo "Done!"
