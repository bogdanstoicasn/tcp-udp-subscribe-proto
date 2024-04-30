// user defined headers
#include "send_recv_file.h"
#include "utils.h"
//end of user defined headers

std::map<int, std::pair<in_addr, uint16_t>> ips_ports;

std::map<std::string, tcp_client *> ids;

void workload(int fdlisten, int fdudp);
void handle_tcp_request(int fd, int i, std::vector<struct pollfd> &multiplex, tcp_request req);

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "\nUsage: %s <PORT_SERVER>\n", argv[0]);
		return 1;
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int rc;

	uint16_t port = atoi(argv[1]);
	DIE(port < 0, "atoi failed");

	// create a TCP socket
	int fdlisten = socket(AF_INET, SOCK_STREAM, 0);
	DIE(fdlisten < 0, "sock failed");

	// create a UDP socket
	int fdudp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(fdudp < 0, "sock failed");

	struct sockaddr_in server;
	//socklen_t socket_len = sizeof(struct sockaddr_in);

	memset((char *)&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;

	// bind the TCP socket
	rc = bind(fdlisten, (struct sockaddr *)&server, sizeof(struct sockaddr));
	DIE(rc < 0, "bind failed");

	// bind the UDP socket
	rc = bind(fdudp, (struct sockaddr *)&server, sizeof(struct sockaddr));
	DIE(rc < 0, "bind failed");

	// listen for incoming connections
	rc = listen(fdlisten, 32);
	DIE(rc < 0, "listen failed");

	workload(fdlisten, fdudp);
	close(fdlisten);
	close(fdudp);
	return 0;
}

void workload(int fdlisten, int fdudp)
{
	//char buffer[BUFF_LEN];

	// error
	int rc;
	int enable = 1;

	std::vector<struct pollfd> multiplex;

	multiplex.push_back({fdlisten, POLLIN, 0});
	multiplex.push_back({fdudp, POLLIN, 0});
	multiplex.push_back({STDIN_FILENO, POLLIN, 0});
	for(;;) {
		rc = poll(&multiplex[0], multiplex.size(), -1);
		DIE(rc < 0, "poll failed");

		for (size_t i = 0; i < multiplex.size(); ++i) {
			if (!multiplex[i].revents)
				continue;

			if (multiplex[i].fd == fdlisten) {
				// new connection
				struct sockaddr_in client;
				socklen_t len = sizeof(client);
				int newsock = accept(fdlisten, (struct sockaddr *)&client, &len);
				DIE(newsock < 0, "accept failed");

				// make the socket non-blocking
				setsockopt(newsock, SOL_SOCKET, SO_REUSEADDR | TCP_NODELAY, &enable, sizeof(int));
				DIE(rc < 0, "setsockopt failed");

				// save the ip and port
				ips_ports[newsock] = std::make_pair(client.sin_addr, client.sin_port);
				multiplex.push_back({newsock, POLLIN, 0});
				continue;
			}
			if (multiplex[i].fd == STDIN_FILENO) {
				// read from stdin
				char buffer[BUFF_LEN];
				memset(buffer, 0, BUFF_LEN);
				rc = read(STDIN_FILENO, buffer, BUFF_LEN);
				DIE(rc < 0, "read failed");
				// parse the command
				char *command = strtok(buffer, " \n\t");
				if (strcmp(command, "exit") == 0) {
					// close all connections
					for (size_t j = 0; j < multiplex.size(); ++j) {
						if (multiplex[j].fd != fdlisten && multiplex[j].fd != fdudp) {
							close(multiplex[j].fd);
						}
					}
					return;
				}
			}

			tcp_request req;
			recv_all(multiplex[i].fd, (void *)&req, sizeof(tcp_request));

			handle_tcp_request(multiplex[i].fd, i, multiplex, req);
		}
	}
}

void handle_tcp_request(int fd, int i, std::vector<struct pollfd> &multiplex, tcp_request req)
{
	//int rc;
	if (req.type == CONNECT) {
		if (ids.find(req.id) != ids.end()) {
			// client already connected, block new conn
			if (ids[req.id]->connection) {
				printf("Client %s already connected.\n", req.id);
				close(multiplex[i].fd);
				multiplex.erase(multiplex.begin() + i);
				ips_ports.erase(multiplex[i].fd);
				return;
			}

			// client stored but offline
			// HERE check print
			printf("New client %s connected from %hu:%s.\n", req.id, ntohs(ips_ports[fd].second), inet_ntoa(ips_ports[fd].first));
			// send the message from waiting queue
			// TODO
			return;
		}

		printf("New client %s connected from %hu:%s.\n", req.id, ntohs(ips_ports[fd].second), inet_ntoa(ips_ports[fd].first));
		tcp_client *client = new tcp_client;
		client->fd = fd;
		client->connection = true;
		client->id = req.id;
		
		ids.insert({req.id, client});
		return;
	}

	if (req.type == EXIT) {
		printf("Client %s disconnected.\n", req.id);
		ids[req.id]->connection = false;
		close(multiplex[i].fd);
		multiplex.erase(multiplex.begin() + i);
		return;
	}
}
