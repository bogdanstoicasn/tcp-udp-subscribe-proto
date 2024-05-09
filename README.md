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
- [Disclaimer](#disclaimer)
- [Resources](#resources)

## Client

The client is a simple program that connects to the server and sends messages.
The client is using tcp to connect and send messages.
We use multiplexing to get messages from STDIN and from the server.
We disable Nagle's algorithm to send messages faster and
we make the address reusable.
The client sends the commands through packets to the server.
The received data from the server is handled and made human-readable.

Sockets:

- *tcp socket* - used to connect to the server and send/receive messages

- *stdin* - used to get input from the user

## Server

The server is a simple program that listens for connections from tcp and udp sockets.

The udp sockets send just the subscription messages.
The server gets them and checks if the client is online and
subscribed to the topic. If the client is online and subscribed to the topic,
send the message to the client.
The server can also get input from STDIN (exit command).

Sockets:

- *stdin* - used to get input from the admin(just close the serverz)

- *tcp socket* - used for the clients to connect to the server

- *udp socket* - used to receive subscription messages

- *client sockets* - used to send/receive messages to the clients

## Data structures

We store the ips and ports in:

- *std::map<int, std::pair<in_addr, uint16_t>> map_ipport;*

We store the clients in:

- *std::map<std::string, tcp_client *> ids;*

**tcp_client** contains fields for:

- connection status(true or false)

- file descriptor(tcp socket)

- id(name of the client)

- subscriptions(topics)

**udp_message** contains fields for:

- length(length of the message)

- message(unfiltered message)

Info: first we send the lentgh of the message and then the message.

## Disclaimer

The regex functionality is not implemented.
If the client is offline, the server doesn't save the messages in a buffer,
so the messages are lost.

## Resources

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/multi/index.html)

- [C++ Reference](https://en.cppreference.com/w/)

