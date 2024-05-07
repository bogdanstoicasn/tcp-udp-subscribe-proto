#### Copyright 2024 Stoica Mihai-Bogdan 325CA (bogdanstoicasn@yahoo.com)

# Project UDP-TCP Subscribe Protocol

## Overview

This project aims to implement a simple subscribe protocol between a server and
some clients. The clients are unique(unique ID), decided at the first
connection. In addition to this, the clients can be offline. The client can:

- `subscribe to a topic`

- `unsubscribe from a topic`

- `exit the server`

- `connect to the server`

The server facilitates the communication between the users and the subscriptions

## Table of contents

- [Overview](#overview)
- [Table of contents](#table-of-contents)
- [Client](#client)
- [Server](#server)
- [Data structures](#data-structures)
- [Resources](#resources)

## Client

The client is a simple program that connects to the server and sends messages.
The client is using tcp to connect and send messages.
We use multiplexing to get messages from STDIN and from the server.
We disable Nagle's algorithm to send messages faster and
we make the address reusable.

## Server

The server is a simple program that listens for connections from tcp and udp sockets.

The udp sockets send just the subscription messages.
The server gets them and checks if the client is online and
subscribed to the topic. If the client is online and subscribed to the topic, send the message to the client.
If the client is offline, save the message in a list and send it when the client connects.
The server can also get input from STDIN (exit command).

## Data structures

We store the ips and ports in:

- *std::map<int, std::pair<in_addr, uint16_t>> ips_ports;*

We store the clients in:

- *std::map<std::string, tcp_client *> ids;*

**tcp_client** contains fields for:

- connection status(true or false)

- file descriptor(tcp socket)

- id(name of the client)

- subscriptions(topics)

- messages(handles offline messages)

**udp_message** contains fields for:

- counter(how many subscribers)

- length(length of the message)

- message(unfiltered message)

## Resources

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/multi/index.html)

- [C++ Reference](https://en.cppreference.com/w/)

