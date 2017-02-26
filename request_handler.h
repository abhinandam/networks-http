//
// Created by evan on 2/25/17.
//

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

int close_socket(int sock);

int client_send(int sock, int client_sock, char * data);

int parseRequest(int sock, int client_sock, char * buf, int BUF_SIZE);


#endif //REQUEST_HANDLER_H
