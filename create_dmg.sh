#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Configuration ---
APP_NAME="KayteIDE"
APP_BUNDLE="${APP_NAME}.app"
# This is where your built .app bundle is located
BUILD_DIR="${1:-./cmake-build-release/bin}" # Default to release build dir, or use first arg
APP_PATH="${BUILD_DIR}/${APP_BUNDLE}"

# This is where the final .dmg will be created
DMG_OUTPUT_DIR="${BUILD_DIR}"
DMG_NAME="${APP_NAME}-Installer.dmg"
FINAL_DMG_PATH="${DMG_OUTPUT_DIR}/${DMG_NAME}"

# Temporary DMG file for building
TEMP_DMG_NAME="${APP_NAME}-temp.dmg"
TEMP_DMG_PATH="${DMG_OUTPUT_DIR}/${TEMP_DMG_NAME}"

# Volume name for the mounted DMG
VOLUME_NAME="${APP_NAME} Installer"

# Background image for the DMG (optional)
# Make sure you have a file like 'resources/dmg_background.png'
DMG_BACKGROUND_IMAGE_PATH="${2:-./resources/dmg_background.png}" # Default to resources, or use second arg

# Check if the app bundle exists
if [ ! -d "${APP_PATH}" ]; then
    echo "Error: Application bundle not found at ${APP_PATH}"
    exit 1
fi

echo "--- Starting DMG Creation for ${APP_NAME} ---"
echo "App Bundle: ${APP_PATH}"
echo "Output DMG: ${FINAL_DMG_PATH}"

# --- Cleanup previous temporary files ---
echo "Cleaning up old temporary DMG files..."
rm -f "${TEMP_DMG_PATH}" "${FINAL_DMG_PATH}"

# --- 1. Create a temporary read/write disk image ---
echo "Creating temporary disk image: ${TEMP_DMG_PATH}"
# Calculate size: App bundle size + some buffer (e.g., 50MB)
# Use 'du -sm' to get size in megabytes (integer)
APP_SIZE_MB=$(du -sm "${APP_PATH}" | awk '{print $1}')
REQUIRED_SIZE=$((APP_SIZE_MB + 50)) # Add 50MB buffer
if [ "${REQUIRED_SIZE}" -lt 100 ]; then # Minimum 100MB for small apps
    REQUIRED_SIZE=100
fi
hdiutil create -ov -fs HFS+ -volname "${VOLUME_NAME}" -size "${REQUIRED_SIZE}m" "${TEMP_DMG_PATH}"

# --- 2. Mount the temporary image ---
echo "Mounting temporary disk image..."
MOUNT_DIR="/Volumes/${VOLUME_NAME}"

# Capture the output of hdiutil attach and extract ONLY the main device name (e.g., /dev/diskX)
ATTACH_OUTPUT=$(hdiutil attach "${TEMP_DMG_PATH}" -readwrite -noverify -noBrowse -mountpoint "${MOUNT_DIR}")
DISK_ID=$(echo "$ATTACH_OUTPUT" | grep '^/dev/disk' | head -n 1 | awk '{print $1}')

# --- 3. Copy the application bundle into the mounted image ---
echo "Copying ${APP_BUNDLE} to ${MOUNT_DIR}/"
cp -R "${APP_PATH}" "${MOUNT_DIR}/"

# --- 4. Create a symlink to /Applications (optional but standard) ---
echo "Creating /Applications symlink..."
ln -s /Applications "${MOUNT_DIR}/Applications"

# --- 5. Customize the DMG appearance (optional) ---
# This part uses AppleScript to set background, icon positions, and window properties.
# Requires 'osascript' (built-in on macOS).
if [ -f "${DMG_BACKGROUND_IMAGE_PATH}" ]; then
    echo "Adding background image: ${DMG_BACKGROUND_IMAGE_PATH}"
    # Copy background image into the .background folder inside the DMG
    mkdir -p "${MOUNT_DIR}/.background"
    cp "${DMG_BACKGROUND_IMAGE_PATH}" "${MOUNT_DIR}/.background/background.png"

    # Define the full POSIX path to the background image *within the mounted DMG*
    DMG_BACKGROUND_FILE_POSIX_PATH="${MOUNT_DIR}/.background/background.png"

    echo "Setting DMG window properties and icon positions..."
    # Using the AppleScript block exactly as you provided it in your last message
    osascript <<EOF
    tell application "Finder"
        tell disk "${VOLUME_NAME}"
            open
            delay 1
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            set the bounds of container window to {100, 100, 600, 400}

            set opts to the icon view options of container window
            set arrangement of opts to not arranged
            set icon size of opts to 128

            try
                set background picture of opts to file ".background:background.png"
            end try

            set position of item "${APP_BUNDLE}" to {180, 180}
            set position of item "Applications" to {480, 180}
            update without registering applications
            delay 2
            close
        end tell
    end tell
EOF
else
    echo "Warning: DMG background image not found at ${DMG_BACKGROUND_IMAGE_PATH}. Skipping background setup."
fi


# --- 6. Unmount the temporary image ---
echo "Unmounting temporary disk image..."
if [ -n "$DISK_ID" ]; then
    echo "Found disk ID: ${DISK_ID}. Attempting to detach..."
    hdiutil detach "${DISK_ID}" -force || {
        echo "First detach attempt failed. Retrying in 5 seconds..."
        sleep 5
        hdiutil detach "${DISK_ID}" -force || {
            echo "Final unmount attempt failed. Exiting."
            exit 1
        }
    }
else
    echo "Error: Disk ID not captured during attach."
    hdiutil info # Debug output
    exit 1
fi
# --- 7. Convert to a compressed, read-only DMG ---
echo "Converting to final compressed DMG: ${FINAL_DMG_PATH}"
hdiutil convert "${TEMP_DMG_PATH}" -format UDZO -imagekey zlib-level=9 -o "${FINAL_DMG_PATH}" -ov

# --- 8. Clean up temporary DMG ---
echo "Cleaning up temporary DMG file..."
rm -f "${TEMP_DMG_PATH}"

echo "--- DMG Creation Complete! ---"
echo "Your installer is ready at: ${FINAL_DMG_PATH}"
