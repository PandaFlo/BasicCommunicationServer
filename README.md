# BasicCommunicationServer

### Overview
This repository contains implementations of a basic chat server written in **Go**, **C**, and **Python**. The chat server allows multiple clients to connect, send messages to each other, and gracefully handle client disconnections. Each language version of the server demonstrates a slightly different approach to achieving the same functionality, serving as a comparison of network programming across different languages.

### Features
- **Multi-client support**: Multiple clients can connect to the server simultaneously.
- **Message broadcasting**: When a client sends a message, it's broadcasted to all other connected clients.
- **Graceful shutdown**: The server and clients can handle shutdowns (e.g., via Ctrl+C) without abruptly terminating connections.
- **Real-time communication**: Clients can send and receive messages in real time.

### Components

#### 1. Go Version (Go-based TCP Chat Server)
- **Server**:
  - The server listens on port `8080`.
  - New clients are prompted to enter a screen name upon connecting.
  - Messages from clients are broadcast to all other connected clients.
  - Handles clients joining and leaving the chat.
  - Graceful server shutdown on receiving system signals (e.g., SIGTERM).
  
- **Client**:
  - Connects to the server at `localhost:8080`.
  - Sends user messages to the server.
  - Displays messages broadcast by other users.

#### 2. C Version
- Implements similar functionality using C sockets.
- May require compiling with a C compiler (`gcc` or similar).
- Typically runs in Unix-based environments where socket programming is available.
  
#### 3. Python Version
- A Python implementation of the chat server using the `socket` library.
- Easier to set up with Python and demonstrates a high-level approach to socket programming.

### Setup Instructions

#### Go Version

1. **Server**:
   - Navigate to the Go directory.
   - Run the server:
     ```bash
     go run server.go
     ```
   
2. **Client**:
   - In a separate terminal, run the client:
     ```bash
     go run client.go
     ```
   - Follow the prompt to enter your screen name and start chatting!

#### C Version

1. **Server**:
   - Navigate to the C directory.
   - Compile the server code:
     ```bash
     gcc -o chat_server server.c
     ```
   - Run the server:
     ```bash
     ./chat_server
     ```

2. **Client**:
   - Compile the client code:
     ```bash
     gcc -o chat_client client.c
     ```
   - Run the client:
     ```bash
     ./chat_client
     ```

#### Python Version

1. **Server**:
   - Navigate to the Python directory.
   - Run the server:
     ```bash
     python server.py
     ```

2. **Client**:
   - Run the client:
     ```bash
     python client.py
     ```

### Graceful Shutdown
Both the server and clients in all versions handle Ctrl+C gracefully:
- The server will broadcast a shutdown message and close all connections.
- Clients will disconnect and exit when terminated.

### Future Enhancements
- Add authentication and user management features.
- Implement private messaging between clients.
- Add logging to track messages and errors on the server side.
- Improve client-side error handling and reconnection logic.

### Contributing
Feel free to submit pull requests to improve the functionality or add new features. Make sure to provide test cases when necessary and update the documentation.

