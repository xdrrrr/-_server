#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>

int server_init(char *IP,unsigned short PORT,int backlog);
int server_accept(int server_fd);
char* receive_json(int client_socket);
int send_json(int client_socket, const char* json_str);
#endif
