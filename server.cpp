// user defined headers
#include "send_recv_file.h"
#include "utils.h"
//end of user defined headers

void workload(int fdlisten, int fdudp);

int main(int argc, char *argv[])
{
	if (argc != 2) {
        fprintf(stderr, "\nUsage: %s <PORT_SERVER>\n", argv[0]);
        return 1;
    }

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int rc;
	int enable = 1;
	int port = atoi(argv[1]);
	
	int fdlisten = socket(AF_INET, SOCK_STREAM, 0);
	DIE(fdlisten < 0, "socket");

	// reusable addrs
	rc = setsockopt(fdlisten, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "fail(SO_REUSEADDR) line 35 - server.cpp ");

	// disable Nagle algo
	
	rc = setsockopt(fdlisten, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
	DIE(rc < 0, "fail(TCP_NODELAY) line 41 - server.cpp");

	struct sockaddr_in server;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&server, 0, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	rc = bind(fdlisten, (struct sockaddr *)&server, socket_len);
	DIE(rc < 0, "fail bind line 53 - server.cpp");

	int fdudp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(fdudp < 0, "fail udp line 56 - server.cpp");

	rc = setsockopt(fdudp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "fail(SO_REUSEADDR) line 59 - server.cpp");

	struct sockaddr_in udp;
	memset(&udp, 0, sizeof(udp));
	udp.sin_family = AF_INET;
	udp.sin_addr.s_addr = htonl(INADDR_ANY);

	rc = bind(fdudp, (struct sockaddr *)&udp, socket_len);
	DIE(rc < 0, "fail bind line 67 - server.cpp");
	
	workload(fdlisten, fdudp);
	close(fdlisten);
	close(fdudp);
	return 0;
}

void workload(int fdlisten, int fdudp)
{
	
}
