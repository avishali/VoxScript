# VoxScript Debug Logging in Reaper

## How to See Debug Output in Reaper

### Option 1: Xcode Console (Recommended for macOS)
1. Open Xcode
2. Go to **Window → Devices and Simulators**
3. Select your Mac in the left sidebar
4. Click **Open Console** button at bottom
5. Filter for "VoxScript" in the search bar
6. Launch Reaper and load VoxScript

### Option 2: Built-in Console.app
1. Open **Applications → Utilities → Console.app**
2. In the search bar, type: `VoxScript`
3. Make sure "Start streaming" is enabled
4. Launch Reaper and load VoxScript

### Option 3: Terminal Log Streaming
```bash
# Open a terminal and run:
log stream --predicate 'processImagePath contains "REAPER"' --info --debug | grep -i voxscript

# Then launch Reaper in another window
```

## What You Should See

When you first load VoxScript as an ARA plugin in Reaper, you should see:

```
================================================
================================================
    VOXSCRIPT v0.1.0 - PHASE I LOADED
    ARA2 Text-Based Vocal Editing Plugin
================================================
ARA Mode: ACTIVE (ARA2)
================================================
```

When you drag an audio file into Reaper with VoxScript as ARA:

```
================================================
VOXSCRIPT: Creating Audio Source
Document: 0x...
HostRef: 0x...
================================================

================================================
VOXSCRIPT: Audio Source Created
Name: [your audio file name]
Sample Rate: 44100 Hz
Channels: 2
Samples: 123456
================================================

================================================
VOXSCRIPT: Audio Source Properties Updated
Name: [your audio file name]
Sample Rate: 44100 Hz
Channels: 2
Samples: 123456
Duration: 2.8 seconds
================================================

================================================
VOXSCRIPT: Creating Audio Modification
Source: 0x...
Clone: NO
================================================

================================================
VOXSCRIPT: Creating Playback Region
Modification: 0x...
================================================

================================================
VOXSCRIPT: Creating Playback Renderer
================================================
```

## Testing Steps in Reaper

1. **Launch Reaper**
2. **Create a new project**
3. **Insert VoxScript as ARA plugin:**
   - Right-click in empty space
   - Select **Insert new track**
   - Click **FX** button on the track
   - Find **VoxScript** under VST3
   - **IMPORTANT:** Make sure to enable it as ARA (there should be an option)
4. **Import audio file:**
   - Drag an audio file (preferably with vocals) onto the track
   - Reaper should show the VoxScript ARA interface
5. **Check console** - You should see all the debug messages

## Troubleshooting

### No Debug Output?
- Make sure you built the Debug version (not Release)
- Debug output only appears in Debug builds
- Check that Console.app is showing "All Messages", not just errors

### Plugin Not Loading?
```bash
# Check if VST3 was installed:
ls -la ~/Library/Audio/Plug-Ins/VST3/VoxScript.vst3

# If not there, rebuild:
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript
./build.sh
```

### ARA Not Activating?
- Some DAWs require explicit ARA activation
- In Reaper: Preferences → Plug-ins → VST → check "Enable VST3 ARA support"
- Restart Reaper after changing settings

## Expected Behavior

✅ **Success looks like:**
- Plugin loads without crashes
- Console shows "VOXSCRIPT v0.1.0 - PHASE I LOADED"
- Console shows "ARA Mode: ACTIVE (ARA2)"
- When audio is added, you see "Creating Audio Source" messages
- UI displays the dual-view layout (Script + Detail)

❌ **Failure looks like:**
- No console output at all
- Crash on plugin load
- "ARA Mode: Standalone" (means ARA didn't activate)
- Missing creation messages when audio is added

## Next Steps After Successful Test

Once you confirm these logs appear:
1. ✅ Mark Phase I as verified in PROJECT_STATUS.md
2. 🚀 Move to Phase II: Transcription Engine
3. 📝 Document any issues in CHANGELOG.md
