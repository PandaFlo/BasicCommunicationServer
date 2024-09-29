package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"os/signal"
	"strings"
	"sync"
	"syscall"
)

type Client struct {
	conn net.Conn
	name string
	ch   chan string
}

var (
	clients    = make(map[*Client]bool)
	clientsMux sync.Mutex
)

func main() {
	listener, err := net.Listen("tcp", ":8080")
	if err != nil {
		fmt.Println("Error starting server:", err)
		return
	}
	defer listener.Close()

	fmt.Println("Server started on port 8080")

	// Handle Ctrl+C to gracefully shut down the server
	sigc := make(chan os.Signal, 1)
	signal.Notify(sigc, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-sigc
		fmt.Println("\nShutting down server...")
		listener.Close()
		os.Exit(0)
	}()

	// Goroutine to handle server's own messages
	go handleServerInput()

	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("Error accepting connection:", err)
			continue
		}
		go handleConnection(conn)
	}
}

func handleConnection(conn net.Conn) {
	// Prompt for screen name
	conn.Write([]byte("Enter your screen name: "))
	name, err := bufio.NewReader(conn).ReadString('\n')
	if err != nil {
		fmt.Println("Error reading name:", err)
		conn.Close()
		return
	}
	name = strings.TrimSpace(name)

	client := &Client{conn: conn, name: name, ch: make(chan string)}
	clientsMux.Lock()
	clients[client] = true
	clientsMux.Unlock()

	// Notify others of new client
	broadcast(fmt.Sprintf("%s has joined the chat\n", client.name), client)

	// Start a goroutine to send messages to the client
	go clientWriter(client)

	// Read messages from the client
	reader := bufio.NewReader(conn)
	for {
		msg, err := reader.ReadString('\n')
		if err != nil {
			break
		}
		msg = strings.TrimSpace(msg)
		if msg != "" {
			broadcast(fmt.Sprintf("%s: %s\n", client.name, msg), client)
		}
	}

	// Client disconnected
	conn.Close()
	clientsMux.Lock()
	delete(clients, client)
	clientsMux.Unlock()
	close(client.ch)
	broadcast(fmt.Sprintf("%s has left the chat\n", client.name), client)
}

func clientWriter(client *Client) {
	writer := bufio.NewWriter(client.conn)
	for msg := range client.ch {
		_, err := writer.WriteString(msg)
		if err != nil {
			break
		}
		writer.Flush()
	}
}

func broadcast(message string, sender *Client) {
	fmt.Print(message)
	clientsMux.Lock()
	defer clientsMux.Unlock()
	for client := range clients {
		if client != sender {
			select {
			case client.ch <- message:
			default:
				// If the client's channel is blocked, skip sending to avoid delays
			}
		}
	}
}

func handleServerInput() {
	reader := bufio.NewReader(os.Stdin)
	for {
		msg, err := reader.ReadString('\n')
		if err != nil {
			fmt.Println("Error reading server input:", err)
			break
		}
		msg = strings.TrimSpace(msg)
		if msg != "" {
			broadcast(fmt.Sprintf("server: %s\n", msg), nil)
		}
	}
}
