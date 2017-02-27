//
// Created by evan on 2/25/17.
//

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

struct request {
    int expContentLength;
    size_t contentLength;
    int expCont;
    char * method;
    char * relPath;
    //char * httpVer;
    char * content;
};

struct response {
    size_t contentLength;
    char * contentTemplate;
    char * contentType;
    char * content;
};

int close_socket(int sock);

int client_send(int sock, int client_sock, struct response * resp);

int parseRequest(int sock, int client_sock, char * buf, size_t BUF_SIZE);

#endif //REQUEST_HANDLER_H
