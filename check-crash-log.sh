#!/bin/bash
# Check Reaper crash logs to see what caused the crash

echo "================================================"
echo "Checking Reaper Crash Logs"
echo "================================================"
echo ""

# Find most recent Reaper crash log
CRASH_LOG=$(ls -t ~/Library/Logs/DiagnosticReports/REAPER* 2>/dev/null | head -1)

if [ -z "$CRASH_LOG" ]; then
    echo "No Reaper crash logs found"
    echo ""
    echo "Try:"
    echo "1. Load VoxScript in Reaper"
    echo "2. Let it crash"
    echo "3. Run this script again"
    exit 0
fi

echo "Most recent crash: $CRASH_LOG"
echo ""
echo "Crash details:"
echo "--------------------------------------------"

# Show crash type and thread
grep -A 5 "Exception Type" "$CRASH_LOG"
echo ""

# Show crashed thread
grep -A 30 "Thread.*Crashed" "$CRASH_LOG" | head -35

echo ""
echo "--------------------------------------------"
echo ""
echo "Looking for VoxScript references..."
grep -i "voxscript" "$CRASH_LOG" || echo "No VoxScript mentions in crash log"

echo ""
echo "Full log at: $CRASH_LOG"
