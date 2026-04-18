#!/bin/bash

# This script scans the products directory for profiles, extracts device and aircraft information,
# and generates a compatibility matrix in the README.md file.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRODUCTS_DIR="$SCRIPT_DIR/src/include/products"
README="$SCRIPT_DIR/README.md"

echo "Updating compatibility matrix in README.md..."

# Device display names (pipe-separated: flag|key|display_name)
# flag: 0 = normal, 1 = always show checkmark, 2 = hide from matrix
DEVICE_NAMES="
1|ursa-minor-joystick|URSA MINOR Airline Joystick L+R
0|fmc|FMC (MCDU, PFP 3N/4/7)
0|pap3-mcp|3N PAP MCP
0|pdc|3N / 3M PDC
0|fcu-efis|FCU (+ optional EFIS L+R)
0|ecam|32 ECAM
0|agp|32 AGP Metal
2|tcas|32 TCAS
0|ursa-minor-throttle|URSA MINOR 32 Throttle Metal (+ optional 32 PAC Metal)
"

# Aircraft display names (pipe-separated: key|display_name)
AIRCRAFT_NAMES="
toliss|ToLiss A3xx
zibo|Zibo / LevelUp 737
xcrafts|X-Crafts E-jets
ff767|FlightFactor 767
ff777|FlightFactor 777
ff350|FlightFactor A350 V1
rotatemd11|Rotate MD-11
ixeg733|IXEG 737
ssg748|SSG 747
jf146|JustFlight 146
laminar-737|Laminar 737
laminar-a333|Laminar A330 (Aerogenesis)
c172|Laminar C172
"

# Function to get display name
get_display_name() {
    local key="$1"
    local mapping="$2"
    echo "$mapping" | grep "^0|$key|\|^1|$key|" | cut -d'|' -f3
}

# Function to get device flag (0 or 1)
get_device_flag() {
    local key="$1"
    local mapping="$2"
    echo "$mapping" | grep "^0|$key|\|^1|$key|" | cut -d'|' -f1
}

TEMP_DIR=$(mktemp -d)
PROFILE_DB="$TEMP_DIR/profiles.db"
AIRCRAFT_SET="$TEMP_DIR/aircraft.set"
DEVICES_FOUND="$TEMP_DIR/devices.set"
> "$PROFILE_DB"
> "$AIRCRAFT_SET"
> "$DEVICES_FOUND"

# Scan all profiles
for product_dir in "$PRODUCTS_DIR"/*; do
    [ -d "$product_dir" ] || continue
    device_folder=$(basename "$product_dir")
    profiles_dir="$product_dir/profiles"
    [ -d "$profiles_dir" ] || continue
    
    # Record device (use display name if available, otherwise use folder name)
    device_display=$(get_display_name "$device_folder" "$DEVICE_NAMES")
    device_flag=$(get_device_flag "$device_folder" "$DEVICE_NAMES")
    [ -z "$device_display" ] && device_display="$device_folder"
    [ -z "$device_flag" ] && device_flag="0"
    echo "$device_folder|$device_flag|$device_display" >> "$DEVICES_FOUND"
    
    echo "  ✓ Device: $device_display"
    
    # Scan profiles
    for profile_file in "$profiles_dir"/*-profile.cpp; do
        [ -f "$profile_file" ] || continue
        profile_name=$(basename "$profile_file" -profile.cpp)
        
        # Extract aircraft name from profile (format: {aircraft}-{device})
        aircraft_key="${profile_name%-$device_folder}"
        
        # Try to find aircraft in mapping
        aircraft_display=""
        found_match=0
        
        while IFS='|' read -r map_key display; do
            [ -z "$map_key" ] && continue
            if [ "$aircraft_key" = "$map_key" ]; then
                aircraft_display="$display"
                found_match=1
                break
            fi
        done <<< "$AIRCRAFT_NAMES"
        
        # If not found in mapping, use extracted aircraft name as display
        if [ $found_match -eq 0 ]; then
            aircraft_display="$aircraft_key"
        fi
        
        # Record profile
        echo "$device_folder:$aircraft_key:$aircraft_display" >> "$PROFILE_DB"
        # Record aircraft
        echo "$aircraft_display" >> "$AIRCRAFT_SET"
    done
done

# Get unique ordered aircraft list
ORDERED_AIRCRAFT="$TEMP_DIR/ordered_aircraft.list"
UNIQUE_AIRCRAFT="$TEMP_DIR/unique_aircraft.list"
> "$ORDERED_AIRCRAFT"
sort "$AIRCRAFT_SET" | uniq > "$UNIQUE_AIRCRAFT"

# First, add aircraft in AIRCRAFT_NAMES order if they have profiles
while IFS='|' read -r aircraft_key aircraft_display; do
    [ -z "$aircraft_key" ] && continue
    if grep -q "^[^:]*:[^:]*:$aircraft_display\$" "$PROFILE_DB"; then
        if ! grep -q "^$aircraft_display\$" "$ORDERED_AIRCRAFT"; then
            echo "$aircraft_display" >> "$ORDERED_AIRCRAFT"
        fi
    fi
done <<< "$AIRCRAFT_NAMES"

# Then, add any aircraft found but not in the mapping
while IFS= read -r aircraft; do
    [ -z "$aircraft" ] && continue
    if ! grep -q "^$aircraft\$" "$ORDERED_AIRCRAFT"; then
        echo "$aircraft" >> "$ORDERED_AIRCRAFT"
    fi
done < "$UNIQUE_AIRCRAFT"

AIRCRAFT_COUNT=$(grep -c . "$ORDERED_AIRCRAFT" || true)

# Build markdown table header with nbsp for spacing
MATRIX="| &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Device&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
SEPARATOR="|---"

while read -r aircraft; do
    [ -z "$aircraft" ] && continue
    MATRIX="$MATRIX | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; $aircraft"
    SEPARATOR="$SEPARATOR | ---"
done < "$ORDERED_AIRCRAFT"

MATRIX="$MATRIX |"$'\n'"$SEPARATOR |"

# Build rows for each device (in DEVICE_NAMES order first, then new ones)
# First, devices in DEVICE_NAMES order
while IFS='|' read -r device_flag device_key device_display; do
    [ -z "$device_key" ] && continue
    [ -d "$PRODUCTS_DIR/$device_key/profiles" ] || continue
    # Skip if flag is 2 (hide from matrix)
    [ "$device_flag" = "2" ] && continue
    
    MATRIX="$MATRIX"$'\n'"| **$device_display**"
    
    # Check compatibility for each aircraft
    while read -r aircraft; do
        [ -z "$aircraft" ] && continue
        if [ "$device_flag" = "1" ]; then
            # Always show checkmark if flag is 1
            MATRIX="$MATRIX | &check;"
        elif grep -q "^$device_key:[^:]*:$aircraft\$" "$PROFILE_DB"; then
            MATRIX="$MATRIX | &check;"
        else
            MATRIX="$MATRIX | &nbsp;"
        fi
    done < "$ORDERED_AIRCRAFT"
    MATRIX="$MATRIX |"
done <<< "$DEVICE_NAMES"

# Then, add any devices found but not in DEVICE_NAMES mapping
sort "$DEVICES_FOUND" | uniq > "$TEMP_DIR/unique_devices.list"

while IFS='|' read -r device_key device_flag device_display; do
    [ -z "$device_key" ] && continue
    # Skip if flag is 2 (hide from matrix)
    [ "$device_flag" = "2" ] && continue
    # Check if device is already in DEVICE_NAMES
    in_mapping=0
    while IFS='|' read -r mapped_flag mapped_key _; do
        [ -z "$mapped_key" ] && continue
        if [ "$mapped_key" = "$device_key" ]; then
            in_mapping=1
            break
        fi
    done <<< "$DEVICE_NAMES"
    
    # Only add if not already in mapping
    if [ $in_mapping -eq 0 ]; then
        MATRIX="$MATRIX"$'\n'"| **$device_display**"
        
        # Check compatibility for each aircraft
        while read -r aircraft; do
            [ -z "$aircraft" ] && continue
            if [ "$device_flag" = "1" ]; then
                # Always show checkmark if flag is 1
                MATRIX="$MATRIX | &check;"
            elif grep -q "^$device_key:[^:]*:$aircraft\$" "$PROFILE_DB"; then
                MATRIX="$MATRIX | &check;"
            else
                MATRIX="$MATRIX | &nbsp;"
            fi
        done < "$ORDERED_AIRCRAFT"
        MATRIX="$MATRIX |"
    fi
done < "$TEMP_DIR/unique_devices.list"

# Update README between markers
START_MARKER="### Compatibility Matrix"
END_MARKER="### Known Issues"

# Read the full file
BEFORE=""
AFTER=""
in_matrix=0
read_before=1

while IFS= read -r line; do
    if [ "$read_before" = "1" ]; then
        if echo "$line" | grep -q "^$START_MARKER"; then
            BEFORE="$BEFORE$line"$'\n'
            # Add intro text
            BEFORE="$BEFORE"$'\n'"The matrix below shows device and aircraft compatibility. Devices are listed vertically, aircraft horizontally."$'\n'
            read_before=0
            in_matrix=1
        else
            BEFORE="$BEFORE$line"$'\n'
        fi
    elif [ "$in_matrix" = "1" ]; then
        if echo "$line" | grep -q "^$END_MARKER"; then
            AFTER=$'\n'"$line"$'\n'
            in_matrix=0
        fi
    else
        AFTER="$AFTER$line"$'\n'
    fi
    
done < "$README"

# Write updated file
(echo -n "$BEFORE"; echo "$MATRIX"; echo -n "$AFTER") > "$README"

# Cleanup
rm -rf "$TEMP_DIR"
