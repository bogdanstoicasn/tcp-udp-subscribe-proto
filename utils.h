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

enum request_type {
    EXIT,
    SUBSCRIBE,
    UNSUBSCRIBE,
    CONNECT
};

typedef struct udp_message {
	int len;
	char *payload;
} udp_message;


typedef struct tcp_client {
	int fd;
	bool connection;
	std::string id;
	std::vector<std::string> topics;
} tcp_client;

typedef struct tcp_request {
	char id[ID_LEN + 1];
	char s[CONTENT_LEN + 1];
	request_type type;
} tcp_request;

#endif
