# Components Directory

This directory should contain external components needed for the project.

## ESP WebRTC Solution

To add the Espressif ESP WebRTC Solution:

1. Clone the repository as a submodule or copy into this directory:
   ```bash
   git submodule add https://github.com/espressif/esp-webrtc-solution.git esp-webrtc-solution
   git submodule update --init --recursive
   ```

2. Or manually clone:
   ```bash
   cd components
   git clone --recursive https://github.com/espressif/esp-webrtc-solution.git
   ```

The ESP-IDF build system will automatically detect components in this directory.

