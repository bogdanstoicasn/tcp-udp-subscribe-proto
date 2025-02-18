#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#include "send_recv_file.h"

// store bytes received in buffer, return the number of bytes received
int recv_all(int sockfd, void *buffer, size_t len)
{
	int rc;
	size_t bytes_received = 0;
	size_t bytes_remaining = len;
	char *buff = (char *)buffer;
	while (bytes_remaining) {
		rc = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
		if (rc <= 0)
			return rc;

		bytes_received += rc;
		bytes_remaining -= rc;
	}
	return bytes_received;
}

// send all data and return the number of bytes sent
int send_all(int sockfd, void *buffer, size_t len)
{
	int rc;
	size_t bytes_sent = 0;
	size_t bytes_remaining = len;
	char *buff = (char *) buffer;

	while (bytes_remaining) {
		rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
		if (rc <= 0)
			return rc;

		bytes_sent += rc;
		bytes_remaining -= rc;
	}
	return bytes_sent;
}

// function that parses \n and \t
int whitespace_parse(char *buff, char **argv)
{
	int argc = 0;
	char *p = strtok(buff, " \n\t");
	while (p) {
		argv[argc++] = p;
		p = strtok(NULL, " \n\t");
	}
	return argc;
}
