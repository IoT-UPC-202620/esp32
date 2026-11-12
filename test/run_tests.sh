#!/usr/bin/env bash
set -e

mkdir -p test/build

SRCS="test/test_main.cpp"
for f in test/test_*.cpp; do
    if [ "$f" != "test/test_main.cpp" ] && [ -f "$f" ]; then
        SRCS="$SRCS $f"
    fi
done

if [ -f "src/gateway/CommandParser.cpp" ]; then
    SRCS="$SRCS src/gateway/CommandParser.cpp"
fi

if [ -f "src/devices/NodeIdentity.cpp" ]; then
    SRCS="$SRCS src/devices/NodeIdentity.cpp"
fi

if [ -f "src/devices/PresenceLight.cpp" ]; then
    SRCS="$SRCS src/devices/PresenceLight.cpp"
fi

if [ -f "src/devices/DoorPresentation.cpp" ]; then
    SRCS="$SRCS src/devices/DoorPresentation.cpp"
fi

g++ -std=c++17 -Wall -Wextra -Itest/vendor -Isrc $SRCS -o test/build/runner.exe

./test/build/runner.exe
