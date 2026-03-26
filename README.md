# Hand Gesture Controller

A hybrid Python + C++ application that uses your MacBook camera to detect hand gestures in real time and map them to system controls.

## Architecture
```
[Python Process]                        [C++ Process]
Camera feed                             Receives gesture string
MediaPipe detects hand        →         Parses the command
Recognises gesture            Socket    Acts on it:
Sends string + coordinates              - Volume up/down
e.g. "POINTING:0.52:0.34"              - Mouse movement
     "OPEN_HAND:0.50:0.50"             - Spotify controls
```

## Tech Stack

| Component | Technology |
|---|---|
| Hand detection | Python + MediaPipe |
| Camera feed | OpenCV (Python) |
| System controls | C++ + Core Graphics |
| IPC | TCP Socket (localhost:65432) |
| Build system | CMake |

## Gestures

| Gesture | Action |
|---|---|
| ☝️ Pointing | Move mouse cursor |
| 👍 Thumbs up | Volume up |
| ✊ Fist | Volume down |
| 🖐 Open hand | Spotify play/pause |
| ✌️ Peace | Spotify next track |

## Project Structure
```
hand-gesture-controller/
├── main.cpp                  # C++ socket server + system controls
├── gesture_sender.py         # Python hand detection + socket client
├── CMakeLists.txt            # C++ build config
├── hand_landmarker.task      # MediaPipe model (auto downloaded)
├── venv/                     # Python virtual environment
└── build/                    # C++ build output
```

## Prerequisites

- macOS
- Homebrew
- Python 3.8+
- CMake
- OpenCV

## Installation

### 1. Install dependencies
```bash
brew install cmake opencv
```

### 2. Set up Python environment
```bash
python3 -m venv venv
source venv/bin/activate
pip install mediapipe==0.10.30 opencv-python
```

### 3. Build the C++ server
```bash
mkdir build && cd build
cmake ..
make
```

## Running

You need two terminal windows.

**Terminal 1 — Start C++ server first:**
```bash
./build/hand_gesture
```

**Terminal 2 — Start Python client:**
```bash
source venv/bin/activate
python3 gesture_sender.py
```

## Permissions

On first run, macOS will request the following permissions:

- **Camera** — System Settings → Privacy & Security → Camera → enable Terminal
- **Accessibility** — System Settings → Privacy & Security → Accessibility → enable Terminal (required for mouse control)

## How It Works

1. Python opens the MacBook camera and processes each frame through MediaPipe
2. MediaPipe returns 21 hand landmarks per detected hand
3. Finger states (up/down) are calculated from landmark positions
4. A gesture is recognised from the combination of finger states
5. The gesture string and fingertip coordinates are sent over a TCP socket
6. The C++ server receives the message, parses it, and executes the mapped system action
# computer-vision-hand-gesture-controller
