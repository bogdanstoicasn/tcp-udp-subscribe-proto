#include <regex>

// user defined headers
#include "send_recv_file.h"
#include "utils.h"
//end of user defined headers

std::map<int, std::pair<in_addr, uint16_t>> map_ipport;

std::map<std::string, tcp_client *> ids;

// handle the server functionality
void workload(int fdlisten, int fdudp);

// handle the request from the client(packet received)
void handle_request(int fd, int i, std::vector<struct pollfd> &multiplex, tcp_request *request);

// search for + or * in the string
int regex_search(tcp_request *request);

// free the ids map
void free_ids();

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
	free_ids();

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
			if (!(multiplex[i].revents & POLLIN)) 
				continue;
			if (multiplex[i].fd == fdudp) {
				char buff[2048];
				memset(buff, 0, 2048);

				struct sockaddr_in youtube;
				socklen_t len = sizeof(youtube);
				rc = recvfrom(fdudp, buff, 2048, 0, (struct sockaddr *)&youtube, &len);
				DIE(rc < 0, "recvfrom failed");

				udp_message *msg = NULL;
				msg = new udp_message;
				DIE(msg == NULL, "new failed");

				msg->len = rc;
				msg->payload = new char[rc];
				memcpy(msg->payload, buff, rc);

				char tpp[51];
				memset(tpp, 0, 51);
				memcpy(tpp, buff, 50);
				// send to all clients that are subscribed to the topic
				// we do a for through all clients and check if the topic is in the topics field
				// if it is we send the message to the client
				for (auto it = ids.begin(); it != ids.end(); ++it) {
					tcp_client *client = it->second;
					if (std::find(client->topics.begin(), client->topics.end(), tpp) != client->topics.end()) {
						// if connected
						if (client->connection) {
							// send the message: first size then the message
							send_all(client->fd, &msg->len, sizeof(int));
							send_all(client->fd, msg->payload, msg->len);
							continue;
						}
						// if not connected we add the message to the messages field
						rc = 0;
						client->messages.push_back(msg);
					}
					
				}
				
				// free the message
				if (!rc) {
					delete[] msg->payload;
					delete msg;
				}
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

				// store the client in the multiplex and in the ips_ports map
				multiplex.push_back({fd, POLLIN, 0});
				map_ipport[fd] = {client.sin_addr, client.sin_port};

				continue;
			}
			if (multiplex[i].fd == STDIN_FILENO) {
				memset(buffer, 0, BUFF_LEN);
				fgets(buffer, BUFF_LEN - 1, stdin);
				buffer[strlen(buffer) - 1] = '\0';

				if (strcmp(buffer, "exit") == 0) {
					for (size_t j = 0; j < multiplex.size(); ++j)
						if (multiplex[j].fd != fdlisten && multiplex[j].fd != fdudp)
							close(multiplex[j].fd);
					return;
				}

				// debug functionality
				if (strcmp(buffer, "printf") == 0)
					for (auto it = ids.begin(); it != ids.end(); ++it)
						printf("Client %s connected: %d\n", it->first.c_str(),
							   it->second->connection);
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

		/// search for the client in the ids map and set to false the connection field
		tcp_client *client = ids.find(request->id)->second;
		client->connection = false;

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
				map_ipport.erase(fd);

				return;
			}
			// client in database
			printf("New client %s connected from %hu:%s.\n", request->id,
				   ntohs(map_ipport[fd].second), inet_ntoa(map_ipport[fd].first));
			auto client = ids[request->id];
			client->connection = true;
			//TODO: send the msgs from the messages field
			return;
		}
		printf("New client %s connected from %hu:%s.\n", request->id,
			   ntohs(map_ipport[fd].second), inet_ntoa(map_ipport[fd].first));

		tcp_client *client = new tcp_client;
		client->connection = true;
		client->fd = fd;

		// add the id to the id filed
		client->id.assign(request->id);
		ids.insert({request->id, client});
		return;
	}

	if (request->type == SUBSCRIBE) {
		tcp_client *client = ids[request->id];

		// the regex search happens when we send msg
		client->topics.push_back(request->s);
		return;
	}
	if (request->type == UNSUBSCRIBE) {
		tcp_client *client = ids[request->id];

		rc = regex_search(request);

		// topic with no regex
		if (rc == 0) {
			// remove topic from client in topics field
			auto it = std::find(client->topics.begin(), client->topics.end(), request->s);
			if (it != client->topics.end())
				client->topics.erase(it);
			return;
		}
		// topic with regex
	}
}

int regex_search(tcp_request *request)
{
	// if find 1 or more regex we return 1
	// else we return 0
	char *p = request->s;
	while (*p) {
		if (*p == '+' || *p == '*')
			return 1;
		++p;
	}

	return 0;
}

void free_ids()
{
	for (auto it = ids.begin(); it != ids.end(); ++it) {
		delete it->second;
		// free the messages field
		// HERE
		for (auto it2 = it->second->messages.begin(); it2 != it->second->messages.end(); ++it2) {
			delete[] (*it2)->payload;
			delete *it2;
		}
	}
}