package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"
)

func main() {
	conn, err := net.Dial("tcp", "localhost:8080")
	if err != nil {
		fmt.Println("Error connecting to server:", err)
		return
	}
	defer conn.Close()

	// Handle Ctrl+C to gracefully shut down the client
	sigc := make(chan os.Signal, 1)
	signal.Notify(sigc, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-sigc
		fmt.Println("\nClosing connection...")
		conn.Close()
		os.Exit(0)
	}()

	// Read server's prompt and send screen name
	serverReader := bufio.NewReader(conn)
	prompt, _ := serverReader.ReadString(':')
	fmt.Print(prompt)
	stdinReader := bufio.NewReader(os.Stdin)
	name, _ := stdinReader.ReadString('\n')
	conn.Write([]byte(name))

	// Goroutine to listen for messages from the server
	go func() {
		for {
			msg, err := serverReader.ReadString('\n')
			if err != nil {
				fmt.Println("Disconnected from server")
				os.Exit(0)
			}
			fmt.Print(msg)
		}
	}()

	// Read user input and send to server
	for {
		msg, err := stdinReader.ReadString('\n')
		if err != nil {
			fmt.Println("Error reading input:", err)
			break
		}
		msg = strings.TrimSpace(msg)
		if msg != "" {
			_, err = conn.Write([]byte(msg + "\n"))
			if err != nil {
				fmt.Println("Error sending message:", err)
				break
			}
		}
	}
}
