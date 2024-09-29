import socket
import threading

HOST = '127.0.0.1'  # Localhost
PORT = 65432        # Port to listen on

# Lists to keep track of clients and their screen names
clients = []
screen_names = {}

def broadcast(message, sender_socket=None):
    """Send a message to all clients except the sender."""
    for client in clients:
        if client != sender_socket:
            try:
                client.sendall(message)
            except:
                # Remove the client if sending fails
                client.close()
                if client in clients:
                    clients.remove(client)

def handle_client(client_socket):
    """Handle communication with a single client."""
    try:
        # Prompt for screen name
        client_socket.sendall(b'Enter your screen name: ')
        screen_name = client_socket.recv(1024).decode().strip()
        screen_names[client_socket] = screen_name

        # Notify others of the new client
        join_message = f"{screen_name} has joined the chat!\n".encode()
        broadcast(join_message, client_socket)
        print(join_message.decode().strip())

        # Receive messages from the client
        while True:
            message = client_socket.recv(1024)
            if not message:
                break
            full_message = f"{screen_name}: {message.decode()}".encode()
            broadcast(full_message, client_socket)
            print(full_message.decode().strip())
    except:
        pass
    finally:
        # Handle client disconnection
        client_socket.close()
        clients.remove(client_socket)
        leave_message = f"{screen_names[client_socket]} has left the chat.\n".encode()
        broadcast(leave_message)
        print(leave_message.decode().strip())
        del screen_names[client_socket]

def accept_connections(server_socket):
    """Accept new client connections."""
    while True:
        try:
            client_socket, _ = server_socket.accept()
            clients.append(client_socket)
            threading.Thread(target=handle_client, args=(client_socket,), daemon=True).start()
        except KeyboardInterrupt:
            break
    server_socket.close()

def server_send_messages():
    """Allow the server to send messages to all clients."""
    try:
        while True:
            server_message = input()
            full_message = f"server: {server_message}\n".encode()
            broadcast(full_message)
    except KeyboardInterrupt:
        pass

def main():
    """Main function to start the server."""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((HOST, PORT))
    server_socket.listen()
    print(f"Server is listening on {HOST}:{PORT}")

    # Start threads for accepting connections and sending messages
    accept_thread = threading.Thread(target=accept_connections, args=(server_socket,), daemon=True)
    accept_thread.start()

    server_send_messages()
    print("Server is shutting down.")
    for client in clients:
        client.close()
    server_socket.close()

if __name__ == "__main__":
    main()
