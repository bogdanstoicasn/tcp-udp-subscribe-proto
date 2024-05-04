// user defined headers
#include "send_recv_file.h"
#include "utils.h"
//end of user defined headers

std::map<int, std::pair<in_addr, uint16_t>> ips_ports;

std::map<std::string, tcp_client *> ids;

// TODO:add a varible or smth that holds the topics and the clients that are subscribed to them


void workload(int fdlisten, int fdudp);

void handle_request(int fd, int i, std::vector<struct pollfd> &multiplex, tcp_request *request);

int regex_search(tcp_request *request);

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

	// make reusablae port
	int enable = 1;
	rc = setsockopt(fdlisten, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt failed");

	// create a UDP socket
	int fdudp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(fdudp < 0, "sock failed");

	// make reusablae port
	rc = setsockopt(fdudp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "setsockopt failed");

	// disable nagle's algorithm
	int flag = 1;
	rc = setsockopt(fdlisten, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

	struct sockaddr_in server;

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
	// free ids
	for (auto it = ids.begin(); it != ids.end(); ++it) {
		delete it->second;
	}
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
			if (!(multiplex[i].revents & POLLIN)) {continue;}
			if (multiplex[i].fd == fdudp) {
				memset(buffer, 0, BUFF_LEN);
				struct sockaddr_in youtube;

				recvfrom(multiplex[i].fd, buffer, BUFF_LEN, 0, (struct sockaddr *)&youtube, (socklen_t *)sizeof(youtube));
				continue;
			}
			if (multiplex[i].fd == fdlisten) {
				struct sockaddr_in client;
				socklen_t len = sizeof(client);
				int fd = accept(fdlisten, (struct sockaddr *)&client, &len);
				DIE(fd < 0, "accept failed");
				// make reusablae port
				int enable = 1;
				rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
				DIE(rc < 0, "setsockopt failed");
				multiplex.push_back({fd, POLLIN, 0});
				ips_ports[fd] = std::make_pair(client.sin_addr, client.sin_port);
				continue;
			}
			if (multiplex[i].fd == STDIN_FILENO) {
				memset(buffer, 0, BUFF_LEN);
				fgets(buffer, BUFF_LEN - 1, stdin);
				buffer[strlen(buffer) - 1] = '\0';
				if (strcmp(buffer, "exit") == 0) {
					for (size_t j = 0; j < multiplex.size(); ++j) {
						if (multiplex[j].fd != fdlisten && multiplex[j].fd != fdudp) {
							close(multiplex[j].fd);
						}
					}
					return;
				}
				continue;
			}
			// receive request
			tcp_request request;
			memset(&request, 0, sizeof(tcp_request));
			recv_all(multiplex[i].fd, &request, sizeof(tcp_request));
			handle_request(multiplex[i].fd, i, multiplex, &request);
		}
	}
}

void handle_request(int fd, int i, std::vector<struct pollfd> &multiplex, tcp_request *request)
{
	int rc;
	if (request->type == EXIT) {
		printf("Client %s disconnected.\n", request->id);
		ids[request->id]->connection = false;
		close(fd);
		multiplex.erase(multiplex.begin() + i);
		return;
	}

	if (request->type == CONNECT) {
		if (ids.find(request->id) != ids.end()) {
			// client already conn, we reject the new one
			if (ids[request->id]->connection) {
				printf("Client %s already connected.\n", request->id);
				close(fd);
				multiplex.erase(multiplex.begin() + i);
				ips_ports.erase(fd);
				return;
			}
			// client in database
			printf("New client %s connected from %hu:%s.\n", request->id, ntohs(ips_ports[fd].second), inet_ntoa(ips_ports[fd].first));
			auto client = ids[request->id];
			client->connection = true;
			//TODO send pending messages
			return;
		}
		printf("New client %s connected from %hu:%s.\n", request->id, ntohs(ips_ports[fd].second), inet_ntoa(ips_ports[fd].first));
		tcp_client *client = new tcp_client;
		client->connection = true;
		client->fd = fd;
		ids[request->id] = client;
		ids.insert(std::make_pair(request->id, client));
		return;
	}
	if (request->type == SUBSCRIBE) {
		tcp_client *client = ids[request->id];
		rc = regex_search(request);
		(void)client;
		// topic with no regex
		if (rc == 0) {
			// if (!client->topics.count(request->s))
			// 	topics[request->s].push_back(client);

			// client->topics[request->s] = true;
		}

		// topic with regex
		// TODO
		return;
	}
	if (request->type == UNSUBSCRIBE) {
		tcp_client *client = ids[request->id];

		rc = regex_search(request);
		
		(void)client;
		// topic with no regex
		if (rc == 0) {
			// auto &topic = topics[request->s];
			// topic.erase(std::find(topic.begin(), topic.end(), client));

			// client->topics.erase(request->s);
		}

		// topic with regex
		// TODO
	}
}

int regex_search(tcp_request *request)
{
	// we search for + or * in the string
	// if find 1 or more regex we return 1
	// else we return 0

	char *p = request->s;
	while (*p) {
		if (*p == '+' || *p == '*') {
			return 1;
		}
		++p;
	}

	return 0;
}