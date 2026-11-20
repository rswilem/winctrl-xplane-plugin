#!/bin/bash

# This script scans the products directory for profiles, extracts device and aircraft information,
# and generates a compatibility matrix in the README.md file.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRODUCTS_DIR="$SCRIPT_DIR/src/include/products"
README="$SCRIPT_DIR/README.md"

echo "Updating compatibility matrix in README.md..."

# Device display names (pipe-separated: flag|key|display_name), in column order.
# The joystick and throttles go last; devices missing from this list are placed before them.
# flag: 0 = normal, 1 = always show checkmark, 2 = hide from matrix
DEVICE_NAMES="
0|fmc|FMC (MCDU, PFP 3N/4/7)
0|pap3-mcp|3N PAP MCP
0|pdc|3N / 3M PDC
0|fcu-efis|FCU (+ optional EFIS L+R)
0|ecam|32 ECAM
0|agp|32 AGP Metal
0|tcas|32 TCAS
0|rmp|32 RMP
0|nws|32 NWS
1|joystick|Joystick (URSA MINOR Airline L+R, ORION Joystick Base II)
0|ursa-minor-throttle|URSA MINOR 32 Throttle Metal (+ optional 32 PAC Metal)
1|orion-throttle|ORION Throttle Base II
"

# Aircraft display names (pipe-separated: key|display_name)
AIRCRAFT_NAMES="
toliss|ToLiss A3xx
zibo|Zibo / LevelUp 737
xcrafts-ejets|X-Crafts E-Jets
fps-ejets|FPS E-Jets
xcrafts-erj|X-Crafts ERJ
ff767|FlightFactor 767
ff777|FlightFactor 777
ff350|FlightFactor A350 V1
ffa320|FlightFactor A320 ultimate (XP11)
rotatemd11|Rotate MD-11
ixeg733|IXEG 737
fps748|FPS 747-800i V2 (SSG) and V3
jf146|JustFlight 146
cis-seneca|CIS Seneca II
laminar-737|Laminar 737
laminar-a333|Laminar A330 (+ Aerogenesis)
laminar-citx|Laminar Citation X
c172-laminar|Laminar C172
b58|Laminar Baron B58
c172-afl|Airfoillabs C172
sparky744|Sparky 744
jar330|JarDesign A330
stratosphere77w|Stratosphere Studios 777-300ER
kingair350|Airfoillabs King Air 350
a220|Deanarica A220-300
bae146|JustFlight 146
pa28|JustFlight PA-28
q4xp|FlyJSim Q4XP
dwsim-atr72|DeltaWing ATR 72-600
fjs732|FlyJSim 732 TwinJet
c525|TorqueSim Citation CJ525
cl650|Hot Start CL650
felis-742|Felis 747-200 Classic
magknight787|Magknight 787
aerobask-phenom|Aerobask Phenom 300
tu-154-ce|Tu-154B2 CE
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

# Order the device columns: DEVICE_NAMES order, with any devices missing from it placed
# before the joystick, which starts the peripherals
ORDERED_DEVICES="$TEMP_DIR/ordered_devices.list"
UNMAPPED_DEVICES="$TEMP_DIR/unmapped_devices.list"
> "$ORDERED_DEVICES"
> "$UNMAPPED_DEVICES"

sort "$DEVICES_FOUND" | uniq | while IFS='|' read -r device_key device_flag device_display; do
    [ -z "$device_key" ] && continue
    [ "$device_flag" = "2" ] && continue
    if ! echo "$DEVICE_NAMES" | grep -q "^[0-9]|$device_key|"; then
        echo "$device_key|$device_flag|$device_display" >> "$UNMAPPED_DEVICES"
    fi
done

while IFS='|' read -r device_flag device_key device_display; do
    [ -z "$device_key" ] && continue
    [ "$device_key" = "joystick" ] && cat "$UNMAPPED_DEVICES" >> "$ORDERED_DEVICES"
    [ -d "$PRODUCTS_DIR/$device_key/profiles" ] || continue
    # Skip if flag is 2 (hide from matrix)
    [ "$device_flag" = "2" ] && continue
    echo "$device_key|$device_flag|$device_display" >> "$ORDERED_DEVICES"
done <<< "$DEVICE_NAMES"

# Build markdown table header: aircraft vertically, devices horizontally. The aircraft column is
# padded with nbsp so its names stay on one line.
MATRIX="| Aircraft&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
SEPARATOR="|---"

while IFS='|' read -r device_key device_flag device_display; do
    MATRIX="$MATRIX | $device_display"
    SEPARATOR="$SEPARATOR | :---:"
done < "$ORDERED_DEVICES"

MATRIX="$MATRIX |"$'\n'"$SEPARATOR |"

# Build a row for each aircraft
while read -r aircraft; do
    [ -z "$aircraft" ] && continue
    MATRIX="$MATRIX"$'\n'"| **$aircraft**"

    while IFS='|' read -r device_key device_flag device_display; do
        if [ "$device_flag" = "1" ]; then
            # Always show checkmark if flag is 1
            MATRIX="$MATRIX | &check;"
        elif grep -q "^$device_key:[^:]*:$aircraft\$" "$PROFILE_DB"; then
            MATRIX="$MATRIX | &check;"
        else
            MATRIX="$MATRIX | &nbsp;"
        fi
    done < "$ORDERED_DEVICES"
    MATRIX="$MATRIX |"
done < "$ORDERED_AIRCRAFT"

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
            BEFORE="$BEFORE"$'\n'"The matrix below shows device and aircraft compatibility. Aircraft are listed vertically, devices horizontally."$'\n'
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
