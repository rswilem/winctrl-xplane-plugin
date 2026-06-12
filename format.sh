#!/bin/bash

# Format a single file with clang-format
format_file() {
    local file="$1"
    if [[ -f "$file" ]]; then
        clang-format -i --style=file "$file"
    else
        echo "File not found: $file"
        return 1
    fi
}

# Format all source files
format_all() {
    find src -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" \) \
        -not -path "*/build/*" \
        -not -path "*/.cmake/*" \
        -not -path "*/CMakeFiles/*" | while read -r file; do
        format_file "$file"
    done
}

# Main
if [[ $# -eq 0 ]]; then
    echo "Formatting all source files..."
    format_all
    echo "Done formatting all files."
else
    echo "Formatting file: $1"
    format_file "$1"
    echo "Done: $1"
fi
