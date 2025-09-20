# TRAT

**A cross-platform Remote Administration Tool (RAT) programmed in C++.**

---

TRAT (Telegram-RAT) is an educational project designed to learn C++, networking, and operating system concepts. While not a unique concept, it serves as a practical tool for exploring system design and low-level programming.

## Dependencies
- C++20
- CURL (client-side networking)
- OpenSSL (Linux) / Windows Secure Channel (Windows)
- simdjson (JSON parsing)
- tiny-process-library (process management)
- CMake (build system)
- zlib and zstd (compression - currently unused)


## Design
Headers and source files are separated. The entry point is located at `rat_trat/src/main.cpp`.

**Note:** The original Telegram bot tokens are invalid due to account suspension. Future versions will implement a custom server and communication protocol.

The `main.cpp` initializes a `::rat::handler::Handler` instance, which contains the core logic for:
- Process management
- Command handling
- File uploads/downloads

### Project Structure
The project is organized into modules:
- **rat_encryption**: Simple XOR obfuscation (not cryptographic encryption)
- **rat_compression**: Basic compression API (currently unused)
- **rat_networking**: Light abstraction layer over libcurl
- **rat_tbot**: Minimal Telegram Bot API implementation (getUpdate, getFile, sendMessage, setOffset)
- **rat_handler**: Core logic module (tightly coupled with Telegram API in current implementation)
- **rat_trat**: Main executable

## Future Development
Plans include transitioning from Telegram Bot API to a custom protocol server while maintaining the current client architecture.

## Building
### Prerequisites
Install all required dependencies.

### Windows Notes
Dependencies were compiled statically and stored in a `STATIC/` directory. CMake configurations reference this path with aliases, to ensure that building remains consistent through platforms.
For windows the msys2/mingw64 environnment is a must have.
https://www.msys2.org/
### Build Commands
To build the client.
```bash
mkdir -p client/build
cd client/build/
cmake -S .. -B . --preset=release
cmake --build . -j$(nproc)  # Linux
cmake --build . -j%NUMBER_OF_PROCESSORS%  # Windows
