// user defined headers
#include "send_recv_file.h"
#include "utils.h"
//end of user defined headers

// handle the client functionality
void workload(int sockfd,char *clientid);

// exit function, closes the connection
int exit_function(int sockfd, char *id, char *comp, int argc);

// subscribe function, sends the subscribe message to the server
int subscribe_function(int sockfd, char *id, char *comp, char *topic, int argc);

// unsubscribe function, sends the unsubscribe message to the server
int unsubscribe_function(int sockfd, char *id, char *comp, char *topic, int argc);

// handle the subscription message shown to stdout
void handle_subscription(char *buff);

// close all connections(sockets)
void close_connections(struct pollfd *multiplex, int size);

int main(int argc, char *argv[])
{
	if (argc != 4) {
        fprintf(stderr, "\nUsage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n",
                argv[0]);
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	DIE(strlen(argv[1]) > 10, "ID to long\n");

    int rc;

    // create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "sock failed");

    // make address reusable
    int enable = 1;
    rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	DIE(rc < 0, "fail(SO_REUSEADDR)");

    // nagles algorithm
    int flag = 1;
    rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    DIE(rc < 0, "fail(TCP_NODELAY)");

    // connect to server
    struct sockaddr_in server;
    memset((char *) &server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[3]));
    rc = inet_pton(AF_INET, argv[2], &server.sin_addr);
    DIE(rc < 0 , "inet_aton failed");

    rc = connect(sockfd, (struct sockaddr *) &server, sizeof(server));
    DIE(rc < 0, "connect failed");

    // send client id packet to server
    tcp_request req;
    memset((void *) &req, 0, sizeof(tcp_request));

    strcpy(req.id, argv[1]);

    req.type = CONNECT;

    rc = send_all(sockfd, (void *) &req, sizeof(tcp_request));
    DIE(rc < 0, "send failed to send connect message\n");
    
    workload(sockfd, argv[1]);

    close(sockfd);

    return 0;
}

void workload(int sockfd, char *clientid)
{
    // for error checking/ general use
    int rc, argc;
    std::vector<struct pollfd> multiplex;

    multiplex.push_back({sockfd, POLLIN, 0});
    multiplex.push_back({STDIN_FILENO, POLLIN, 0});


    char buff[BUFF_LEN], *argv[BUFF_LEN];
    memset(buff, 0, BUFF_LEN);

    for(;;) {
        rc = poll(multiplex.data(), 2, -1);
        if (rc < 0) {
            perror("poll failed");
            return;
        }

        // server respons
        if (multiplex[0].revents & POLLIN) {
            int size = 0;
            rc = recv_all(sockfd, &size, sizeof(int));
            if (!rc) {
                close_connections(multiplex.data(), multiplex.size());
                perror("recv failed");
                return;
            }

            char *payload = new char[size];

            rc = recv_all(sockfd, payload, size);
            if (!rc) {
                close_connections(multiplex.data(), multiplex.size());
                perror("recv failed");
                return;
            }

            handle_subscription(payload);

            delete[] payload;

            continue;
        }

        if (!(multiplex[1].revents & POLLIN))
            continue;

        memset(buff, 0, BUFF_LEN);
        memset(argv, 0, sizeof(argv));

        // stdin input by subscriber
        fgets(buff, BUFF_LEN - 1, stdin);
        argc = whitespace_parse(buff, argv);

        // exit
        if (!exit_function(sockfd, clientid, argv[0], argc)) {
            close_connections(multiplex.data(), multiplex.size());
            return;
        }

        // subscribe
        if (!subscribe_function(sockfd, clientid, argv[0], argv[1], argc))
            continue;

        // unsubscribe
        if (!unsubscribe_function(sockfd, clientid, argv[0], argv[1], argc))
            continue;

    }
}

// sends exit message to server
int exit_function(int sockfd, char *id, char *comp, int argc)
{
    if (strcmp(comp, "exit") == 0 && argc == 1) {
        tcp_request req;
        memset((void *) &req, 0, sizeof(tcp_request));
        strcpy(req.id, id);

        req.type = EXIT;

        int rc = send_all(sockfd, (void *) &req, sizeof(tcp_request));
        DIE(rc < 0, "send failed to send exit message\n");

        return 0;
    }
    return 1;
}

// sends subscribe message to server
int subscribe_function(int sockfd, char *id, char *comp, char *topic, int argc)
{
    if (strcmp(comp, "subscribe") == 0 && argc == 2) {
        tcp_request req;
        memset((void *) &req, 0, sizeof(tcp_request));
        strcpy(req.id, id);

        req.type = SUBSCRIBE;
        strcpy(req.s, topic);

        int rc = send_all(sockfd, (void *) &req, sizeof(tcp_request));
        DIE(rc < 0, "send failed to send subscribe message\n");

        printf("Subscribed to topic %s\n", topic);
        return 0;
    }

    return 1;
}

// sends unsubscribe message to server
int unsubscribe_function(int sockfd, char *id, char *comp, char *topic, int argc)
{
    if (strcmp(comp, "unsubscribe") == 0 && argc == 2) {
        tcp_request req;
        memset((void *) &req, 0, sizeof(tcp_request));
        strcpy(req.id, id);

        req.type = UNSUBSCRIBE;
        strcpy(req.s, topic);

        int rc = send_all(sockfd, (void *) &req, sizeof(tcp_request));
        DIE(rc < 0, "send failed to send unsubscribe message\n");

        printf("Unsubscribed from topic %s\n", topic);
        return 0;
    }
    return 1;
}

// close all connections(sockets)
void close_connections(struct pollfd *multiplex, int size)
{
    for (int i = 0; i < size; i++)
        close(multiplex[i].fd);
}

// handle the subscription message shown to stdout
void handle_subscription(char *buff)
{
    char topic[TOPIC_LEN + 1] = {0};

    memcpy(topic, buff, TOPIC_LEN);
    buff += TOPIC_LEN;

    u_int8_t type = *buff;
    buff += sizeof(u_int8_t);

    // int
    if (type == 0) {
        int8_t sign = *buff;
        buff += sizeof(int8_t);

        int32_t network;
        std::memcpy(&network, buff, sizeof(int32_t));
        int64_t value = ntohl(network);

        if (sign)
            value = -value;

        printf("%s - INT - %ld\n", topic, value);
        return;
    }

    // short real
    if (type == 1) {
        uint16_t network;
        std::memcpy(&network, buff, sizeof(uint16_t));
        float value = ntohs(network) / 100.0;

        printf("%s - SHORT_REAL - %.2f\n", topic, value);
        return;
    }

    // float
    if (type == 2) {
        int8_t sign = *buff;
        buff += sizeof(int8_t);

        u_int32_t network;
        std::memcpy(&network, buff, sizeof(u_int32_t));
        float value = ntohl(network);

        if (sign)
            value = -value;

        buff += sizeof(u_int32_t);
        int8_t power = *buff;

        printf("%s - FLOAT - %f\n", topic, value / (float) pow(10, power));
        return;
    }

    // string
    if (type == 3)
        printf("%s - STRING - %s\n", topic, buff);
}