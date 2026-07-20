# LowSightFace

> An assistive solution for individuals with low vision or face recognition difficulties.

## Overview

**BntechEyeFriend** (LowSightFace) is an assistive tool designed for **people with low vision** and **prosopagnosia (face blindness)**. It continuously captures video through a camera, detects and recognizes faces in real time against a local face library, and instantly announces recognized names via voice — allowing users to identify people around them without needing to see clearly.

## Core Features

- **Real-time face detection** — Automatically detects faces from live camera feed
- **Face recognition & matching** — Compares detected faces against a local face library
- **Text-to-Speech (TTS) announcement** — Speaks recognized names aloud; no need to look at the screen
- **Face library management** — Add, remove, and manage enrolled persons
- **Multi-camera support** — Switch between front/rear cameras and external devices
- **Face zoom** — Click a face bounding box to enlarge it to fullscreen for detail inspection

## Privacy & Security

All face data and recognition inference run **entirely locally**. No images or face features are uploaded to any server. The system uses ONNX Runtime / MediaPipe for on-device inference with no third-party cloud APIs.

## Platform Support

| Platform | Status |
|----------|--------|
| HTML5 (Web) | 🚧 In development |
| Qt Desktop (Windows/Linux) | ✅ Available |
| Qt WASM (Browser) | ✅ Available |
| Android / iOS | 📋 Planned |
| HarmonyOS | 📋 Planned |
| ESP32-CAM Hardware | 🚧 In development |

## Quick Start (HTML5)

```bash
cd html5_client
python -m http.server 8080 --bind 127.0.0.1
# Open http://localhost:8080
```

## Tech Stack

- **Face Detection**: ONNX Runtime (desktop) / Google MediaPipe + BlazeFace (HTML5)
- **Face Recognition**: MobileFaceNet via ONNX Runtime
- **TTS**: Web Speech API / system-native TTS engine


## Usage

1. Launch the app and grant camera permission
2. Enroll face photos and names in the face library
3. Point the camera at people — the system auto-detects and matches
4. Recognized names are announced via TTS
5. Click face boxes to zoom in for detail

## Use Cases

- Social scenarios for visually impaired individuals
- Face blindness (prosopagnosia) assistance
- Memory aid for elderly users
- Multi-person identification in meetings and gatherings

## License

This project uses a **dual-license model**:

- **Non-commercial use** → GNU General Public License v3.0 (GPLv3): free for personal use, academic research, non-profit organizations, and fully open-source derivative projects.
- **Commercial use** → A commercial license must be obtained from the project author for enterprise tools, hardware/software products for sale, closed-source redistribution, paid SaaS services, and OEM/ODM mass production.

> 📧 For commercial licensing inquiries, please contact the project author.

---

*Helping everyone "see" the people around them.*
