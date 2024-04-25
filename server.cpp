// user defined headers
#include "send_recv_file.h"
#include "utils.h"
//end of user defined headers

std::map<int, std::pair<in_addr, uint16_t>> ips_ports;

std::map<std::string, tcp_client *> ids;

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
	char buffer[BUFF_LEN];

	// error
	int rc;

	std::vector<struct pollfd> multiplex;

	multiplex.push_back({fdlisten, POLLIN, 0});
	multiplex.push_back({fdudp, POLLIN, 0});
	multiplex.push_back({STDIN_FILENO, POLLIN, 0});
	for(;;) {
		rc = poll(&multiplex[0], multiplex.size(), -1);
		DIE(rc < 0, "poll failed");

		for (size_t i = 0; i < multiplex.size(); ++i) {
			if (!(multiplex[i].revents & POLLIN)) {
				continue;
			}

			if (multiplex[i].fd == STDIN_FILENO) {
				char *argv[BUFF_LEN];
				memset(buffer, 0, BUFF_LEN);
				fgets(buffer, BUFF_LEN, stdin);
				int argc = whitespace_parse(buffer, argv);

				if (argc == 1 && !strcmp(argv[0], "exit")) {
					for (auto &client : multiplex) {
						close(client.fd);
					}
					return;
				}
				continue;
			}

			if (multiplex[i].fd == fdlisten) {
				// connection request from inactive client
				struct sockaddr_in client;
				socklen_t socket_len = sizeof(struct sockaddr_in);
				int newsockfd = accept(fdlisten, (struct sockaddr *)&client, &socket_len);
				DIE(newsockfd < 0, "accept failed");

				// disable Nagle algo
				int enable = 1;
				rc = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));//here
				DIE(rc < 0, "fail(TCP_NODELAY) line 116 - server.cpp");

				// save the ip and port of the client for connection later on
				ips_ports[newsockfd] = {client.sin_addr, client.sin_port};

				// add the new client to the multiplex
				multiplex.push_back({newsockfd, POLLIN, 0});

				continue;
			}

			if (multiplex[i].fd == fdudp) {

				struct sockaddr_in client;
				socklen_t socket_len = sizeof(struct sockaddr_in);

				memset(buffer, 0, BUFF_LEN);

				int rc = recvfrom(fdudp, buffer, BUFF_LEN, 0, (struct sockaddr *)&client, &socket_len);
				DIE(rc < 0, "recvfrom failed");
				//HERE
			}

			// client request HERE
			memset(buffer, 0, BUFF_LEN);

			tcp_request req;
			recv_all(multiplex[i].fd, &req, sizeof(req));

			// TODO: handle the cases: CONNECT, SUBSCRIBE, UNSUBSCRIBE, EXIT

		}


	}
}
