# ESP32-P4-NANO Audio Output Demo

This project demonstrates audio output functionality on the ESP32-P4-NANO development board. It generates a sine wave and plays it through the onboard audio codec and speaker.

## Features

- Configurable I2S audio output with support for different sample rates, bit depths, and channel configurations.
- Sine wave generation at a specified frequency and amplitude.
- Integration with the ES8311 audio codec for speaker output.
- FreeRTOS-based task management for audio generation and playback.

## How to Use

### Prerequisites

- ESP32-P4-NANO development board.
- ESP-IDF installed and configured on your system.
- A speaker connected to the board.

### Build and Flash

*The following steps are for the CLI, but can of course be done in the IDE as well.*

1. Clone this repository to your local machine.
2. Navigate to the project directory.
3. Run the following commands to build and flash the project (replace `YOUR_SERIAL_PORT` with the actual serial port of your ESP32-P4-NANO board):
   ```bash
   idf.py build
   idf.py -p YOUR_SERIAL_PORT flash
   idf.py -p YOUR_SERIAL_PORT monitor
   ```
4. Reset the board if necessary.

### Expected Output

Once the project is running, you should hear a 1 kHz sine wave played through the speaker. The ESP-IDF monitor will display logs related to the audio initialization and playback.

## Project Structure

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   ├── main.c          // Main application logic
│   ├── p4nano_audio.c  // Audio initialization and codec handling
│   ├── p4nano_audio.h  // Audio-related definitions and declarations
└── README.md            // Project documentation
```

## Configuration

The audio settings (e.g., sample rate, bit depth, and channels) can be modified in `main.c`:
```c
static const int SAMPLE_RATE = 48000; // Sample rate in Hz
static const int BIT_DEPTH = 16;      // Bit depth
static const int CHANNEL_COUNT = 1;   // Number of channels (1 for mono, 2 for stereo)
```

## License

This project is licensed under the Apache-2.0 License. See the `LICENSE` file for details.
