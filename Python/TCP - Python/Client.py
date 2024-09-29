import socket
import threading
import sys
from prompt_toolkit import PromptSession
from prompt_toolkit.patch_stdout import patch_stdout

HOST = '127.0.0.1'  # Server's hostname or IP address
PORT = 65432        # Port used by the server

def receive_messages(sock):
    """Receive messages from the server."""
    while True:
        try:
            message = sock.recv(1024)
            if not message:
                break
            print(f"\r{message.decode()}\n", end='')
        except:
            break

def main():
    """Main function to connect to the server and send messages."""
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect((HOST, PORT))
    except:
        print("Unable to connect to the server.")
        sys.exit()

    # Start a thread to receive messages from the server
    threading.Thread(target=receive_messages, args=(client_socket,), daemon=True).start()

    session = PromptSession()

    try:
        with patch_stdout():
            while True:
                message = session.prompt()
                client_socket.sendall(message.encode())
    except KeyboardInterrupt:
        pass
    finally:
        client_socket.close()
        sys.exit()

if __name__ == "__main__":
    main()
