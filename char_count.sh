#!/bin/bash

# Check if a string argument is provided
if [ -z "$1" ]; then
    echo "Usage: $0 \"your string here\""
    exit 1
fi

# Count characters in the input string
char_count=$(echo -n "$1" | wc -c)

echo "Character Count: $char_count"

