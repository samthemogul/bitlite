# Bitlite

Bitlite is a free chat application that leverages Wi-Fi technology and the BitTorrent protocol to send messages between devices. It is designed to provide a decentralized and efficient way to communicate without relying on traditional internet services.

## Features

- **Wi-Fi Scanning and Connection**: Automatically scans for available Wi-Fi networks and allows users to connect to a selected network.
- **Decentralized Messaging**: Uses a BitTorrent-like protocol to send messages directly between devices without a central server.
- **WebSocket Communication**: Enables real-time message exchange between devices using WebSocket technology.

## Upcoming Integration

1. Expand support to include Windows, Debian, Fedora, Android, and iOS devices.
2. Implement Wi-Fi Direct with a multi-hop mechanism to enable devices to chat over longer distances without requiring a traditional Access Point or hotspot.
3. Extend the BitTorrent-like protocol to support file sharing between devices.

## Project Structure

```
Bitlite/
├── build/                # Compiled binaries and build files
├── include/             # Header files for C++ modules
├── src/                 # Source code for C++ modules
├── tests/               # Test cases (if any)
├── torrents/            # Sample torrent files
├── server.js            # Node.js WebSocket client and process manager
├── server.py            # Python WebSocket server for bidirectional chat
├── CMakeLists.txt       # CMake configuration file
└── package.json         # Node.js dependencies
```

## Prerequisites

### For C++ Components
- A C++ compiler (e.g., GCC, Clang)
- CMake (version 3.10 or higher)
- Boost libraries

### For Node.js Components
- Node.js (version 16 or higher)
- `ws` package (installed via `npm install`)

### For Python Components
- Python 3.11 or higher
- `websockets` package (installed via `pip install websockets`)

## Installation

1. Build the C++ components:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

1. Install Node.js dependencies:
   ```bash
   npm install
   ```

3. Install Python dependencies:
   ```bash
   pip install websockets
   ```

## Usage

### Starting the System

1. Start the Python WebSocket server on your phone(Using Termux or an equivalent):
   ```bash
   python3 server.py
   ```

2. Run the Node.js WebSocket client on your laptop(Linux-Debian OS for now):
   ```bash
   node server.js
   ```

3. Follow the prompts to select a Wi-Fi network and connect.

4. Use the terminal to send and receive messages in real-time.

### Example Workflow

- The Node.js client spawns the C++ binary to scan and connect to Wi-Fi networks.
- Once connected, the client retrieves the phone's IP and port and establishes a WebSocket connection.
- Messages can be exchanged in real-time between the laptop and the phone.

## Contributing

This project would be open-sourced soon, watch out.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Acknowledgments


- [WebSocket](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API) for real-time communication
- [Node.js](https://nodejs.org/) for process management and WebSocket client
- [Python](https://www.python.org/) for the WebSocket server
- [Netlink](https://wireless.docs.kernel.org/en/latest/en/developers/documentation/nl80211.html) for Wi-Fi network communication in Linux
