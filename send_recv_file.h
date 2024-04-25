#ifndef SEND_RECV_FILE_H
#define SEND_RECV_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

int recv_all(int sockfd, void *buffer, size_t len);

int send_all(int sockfd, void *buffer, size_t len);

int whitespace_parse(char *buff, char **argv);

#endif
