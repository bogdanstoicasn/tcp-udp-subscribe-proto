// user defined headers
#include "send_recv_file.h"
#include "utils.h"
//end of user defined headers

void workload(int sockfd);

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

    // connect to server
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[3]));
    rc = inet_aton(argv[2], &server_addr.sin_addr);
    DIE(rc < 0 , "inet_aton failed");

    rc = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    DIE(rc < 0, "connect failed");

    // send id_client to server
    char buffer[ID_LEN + 1];

    memset((void *) buffer, 0, ID_LEN + 1);
    strcpy(buffer, argv[1]);
    rc = send_all(sockfd, buffer, ID_LEN + 1);
    DIE(rc < 0, "send failed");

    workload(sockfd);

    close(sockfd);

    return 0;
}

void workload(int sockfd)
{
    ;
}
