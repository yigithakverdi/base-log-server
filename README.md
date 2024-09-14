# Log Server Application

A multi-threaded C++ log server capable of handling multiple client connections concurrently. The server listens for connections on a specified port, creates a thread for each client to manage communication, and logs client messages with metadata. It includes features like log file rotation and resource exhaustion handling.

## Table of Contents

- [Log Server Application](#log-server-application)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Requirements](#requirements)
  - [Installation](#installation)
  - [Usage](#usage)
    - [Server](#server)
      - [Command-Line Arguments](#command-line-arguments)
      - [Examples](#examples)
    - [Client](#client)
      - [Using Netcat](#using-netcat)
      - [Using a Custom Client](#using-a-custom-client)
  - [Configuration](#configuration)
  - [Testing](#testing)
  - [Project Structure](#project-structure)

---

## Features

- **Concurrent Client Handling**: Supports multiple clients simultaneously using threads.
- **Thread and Resource Management**:
  - Uses mutexes and semaphores to prevent resource exhaustion.
  - Limits the number of concurrent client connections.
- **Individual Client Logging**:
  - Creates separate log files for each connected client.
  - Clients write logs to independent log files concurrently.
- **Metadata Logging**: Logs include client IP, timestamp, and message size.
- **Log File Rotation**: Automatically rotates log files when they exceed a specified size.
- **Configurable Parameters**: Allows customization via command-line arguments.

---

## Requirements

- **Operating System**: Linux or macOS (POSIX-compliant systems)
- **Compiler**: `g++` with C++11 support
- **Libraries**:
  - POSIX Threads (`-pthread`)
  - Networking libraries (`<netinet/in.h>`, `<arpa/inet.h>`)
  - POSIX Semaphores (`<semaphore.h>`)

---

## Installation

1. **Clone the Repository**

   ```bash
   git clone https://github.com/yourusername/log-server.git
   cd log-server
   ```

2. **Compile the Server**

   ```bash
   g++ -std=c++11 -pthread server.cpp -o server
   ```

3. **(Optional) Compile the Client**

   If you have a client program, compile it similarly:

   ```bash
   g++ -std=c++11 client.cpp -o client
   ```

---

## Usage

### Server

Start the server with default settings:

```bash
./server
```

#### Command-Line Arguments

- `--port PORT`: Specify the port number (default: `12345`).
- `--rotation SIZE`: Set the log rotation size in bytes (default: `5242880` bytes or 5 MB).

#### Examples

- Run the server on port `8080`:

  ```bash
  ./server --port 8080
  ```

- Run the server with a log rotation size of 1 MB:

  ```bash
  ./server --rotation 1048576
  ```

### Client

You can use `netcat` to simulate a client or use a custom client program.

#### Using Netcat

1. **Connect to the Server**

   ```bash
   nc localhost 12345
   ```

2. **Send Messages**

   Type messages and press Enter to send them to the server.

#### Using a Custom Client

If you have a `client.cpp`, compile and run it:

```bash
./client
```

---

## Configuration

The server can be configured using command-line arguments:

- **Port Number**: Use `--port` to set the listening port.
- **Log Rotation Size**: Use `--rotation` to set the maximum log file size before rotation.

Example:

```bash
./server --port 8080 --rotation 1048576
```

---

## Testing

To test the server's functionality and resource handling:

1. **Start the Server**

   ```bash
   ./server
   ```

2. **Simulate Multiple Clients**

   Open multiple terminals and connect to the server using `netcat` or your client program.

   ```bash
   nc localhost 12345
   ```

3. **Exceed Maximum Connections**

   Attempt to connect more clients than the `MAX_THREADS` limit (default is 10). Excess clients will wait until a slot is available.

4. **Send Messages**

   From each client, send messages to the server.

5. **Verify Logs**

   - Each client will have its own log file named `client_<IP>_<PORT>.log`.
   - Check that messages are logged correctly with metadata.

---

## Project Structure

- `server.cpp`: The main server application source code.
- `client.cpp`: (Optional) A simple client application source code.
- `README.md`: Project documentation.
- `LICENSE`: Project license file.

---
