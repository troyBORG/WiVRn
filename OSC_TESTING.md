# Testing OSC Output Feature

## Building

### Build Server Only
```bash
cd /mnt/12tb/git/linuxVR/WiVRn
cmake -B build-server . -GNinja -DWIVRN_BUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-server
```

### Build Server + Dashboard (for GUI configuration)
```bash
cmake -B build-dashboard . -GNinja -DWIVRN_BUILD_CLIENT=OFF -DWIVRN_BUILD_SERVER=ON -DWIVRN_BUILD_DASHBOARD=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-dashboard
```

The server binary will be at: `build-server/wivrn-server` or `build-dashboard/server/wivrn-server`
The dashboard binary will be at: `build-dashboard/dashboard/wivrn-dashboard`

## Testing OSC Output

### Method 1: Using a Simple OSC Listener (Python)

Install python-osc:
```bash
pip install python-osc
```

Create a simple listener script `test_osc_listener.py`:
```python
#!/usr/bin/env python3
from pythonosc import osc_server
from pythonosc.dispatcher import Dispatcher

def print_handler(address, *args):
    print(f"{address}: {args}")

dispatcher = Dispatcher()
dispatcher.map("/tracking/trackers/*", print_handler)

server = osc_server.ThreadingOSCUDPServer(("127.0.0.1", 9000), dispatcher)
print("Listening for OSC messages on 127.0.0.1:9000")
print("Press Ctrl+C to stop")
server.serve_forever()
```

Run it:
```bash
python3 test_osc_listener.py
```

### Method 2: Using oscdump (from liblo-tools)

If you have `liblo-tools` installed:
```bash
oscdump 9000
```

### Method 3: Using netcat (basic UDP listener)

```bash
nc -u -l 9000
```

This will show raw UDP packets (not parsed OSC).

## Enabling OSC Output

### Via Configuration File

Edit `~/.config/wivrn/config.json` (or create it if it doesn't exist):
```json
{
  "osc-enabled": true,
  "osc-host": "127.0.0.1",
  "osc-port": 9000
}
```

### Via Dashboard

1. Run the dashboard: `./build-dashboard/dashboard/wivrn-dashboard`
2. Go to Settings → Advanced options
3. Check "Enable OSC output (SteamLink emulation)"
4. Set OSC Host (default: 127.0.0.1)
5. Set OSC Port (default: 9000)
6. Click OK to save

## Testing Steps

1. Start your OSC listener (using one of the methods above)
2. Start WiVRn server: `./build-server/wivrn-server` (or via dashboard)
3. Connect your headset to WiVRn
4. Start a VR application or just move the headset/controllers
5. You should see OSC messages appearing in your listener:
   - `/tracking/trackers/head/position` (x, y, z)
   - `/tracking/trackers/head/rotation` (x, y, z, w)
   - `/tracking/trackers/left_controller/position` (x, y, z)
   - `/tracking/trackers/left_controller/rotation` (x, y, z, w)
   - `/tracking/trackers/right_controller/position` (x, y, z)
   - `/tracking/trackers/right_controller/rotation` (x, y, z, w)
   - And velocity/angular velocity messages when available

## Expected OSC Messages

The OSC output sends messages in SteamLink format:

- **Position**: `/tracking/trackers/{device_name}/position` with 3 floats (x, y, z)
- **Rotation**: `/tracking/trackers/{device_name}/rotation` with 4 floats (x, y, z, w quaternion)
- **Velocity**: `/tracking/trackers/{device_name}/velocity` with 3 floats (x, y, z)
- **Angular Velocity**: `/tracking/trackers/{device_name}/angularvelocity` with 3 floats (x, y, z)

Device names:
- `head` - Headset tracking
- `left_controller` - Left controller (grip/aim/palm poses)
- `right_controller` - Right controller (grip/aim/palm poses)
- `left_hand` - Left hand tracking
- `right_hand` - Right hand tracking
- `eye_gaze` - Eye gaze tracking (if available)

## Troubleshooting

- **No messages received**: 
  - Check that OSC is enabled in config or dashboard
  - Verify the host/port are correct
  - Check firewall settings (UDP port 9000)
  - Make sure the headset is connected and tracking data is being received

- **Connection errors in logs**:
  - Check server logs for OSC-related errors
  - Verify the host address is valid (127.0.0.1 for localhost)

- **Messages not in expected format**:
  - The OSC format follows SteamLink specification
  - Check that your listener supports the OSC protocol properly

