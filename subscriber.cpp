// user defined headers
#include "send_recv_file.h"
#include "utils.h"
//end of user defined headers

void workload(int sockfd,char *clientid);

int exit_function(int sockfd, char *id, char *comp, int argc);

int subscribe_function(int sockfd, char *id, char *comp, char *topic, int argc);

int unsubscribe_function(int sockfd, char *id, char *comp, char *topic, int argc);

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
        memset(buff, 0, BUFF_LEN);
        memset(argv, 0, sizeof(argv));


        rc = poll(multiplex.data(), 2, -1);
        if (rc < 0) {
            perror("poll failed");
            return;
        }

        // server respons
        if (multiplex[0].revents & POLLIN) {
            rc = recv_all(sockfd, buff, sizeof(tcp_request));
            if (rc <= 0) {
                close_connections(multiplex.data(), multiplex.size());
                perror("recv failed");
                return;
            }

            // TODO: here we must parse the message received from the server
            
            continue;
        }

        if (!(multiplex[1].revents & POLLIN)) {
            continue;
        }

        // stdin input by subscriber
        fgets(buff, BUFF_LEN, stdin);
        argc = whitespace_parse(buff, argv);

        // exit
        if (!exit_function(sockfd, clientid, argv[0], argc)) {
            close_connections(multiplex.data(), multiplex.size());
            return;
        }

        // subscribe
        if (!subscribe_function(sockfd, clientid, argv[0], argv[1], argc)) {
            continue;
        }

        // unsubscribe
        if (!unsubscribe_function(sockfd, clientid, argv[0], argv[1], argc)) {
            continue;
        }

    }
}

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

void close_connections(struct pollfd *multiplex, int size)
{
    for (int i = 0; i < size; i++) {
        close(multiplex[i].fd);
    }
}