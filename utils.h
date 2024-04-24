#ifndef UTILS_H
#define UTILS_H

#include <bits/stdc++.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <time.h>
#include <netinet/tcp.h>

#include <inttypes.h>

#define BUFF_LEN 1600
#define ID_LEN 10
#define CONTENT_LEN 1500
#define TCP_LEN 64
#define TOPIC_LEN 50

#define NO_CONTENT_SIZE (sizeof(tcp_client) - CONTENT_LEN)

typedef struct udp_message {
	char topic[TOPIC_LEN];
	uint8_t type;
	char content[CONTENT_LEN];
} udp_message;

typedef struct tcp_client {
	uint32_t ip;
	uint16_t port;
	uint8_t type;
	char topic[TOPIC_LEN];
	char content[CONTENT_LEN];
} tcp_client;

typedef struct tcp_server {
	// 0 - sub, 1 - unsub
	int type;
	int sf;
	char topic[TOPIC_LEN + 1];
} tcp_server;
#endif
