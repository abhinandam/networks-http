/******************************************************************************
* http_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for a HTTP server.  The   *
*              server runs on a hard-coded port and supports GET, HEAD, and   *
*              POST requests sent to it by connected clients.  It supports    *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Evan Kesten <ebk46@cornell.edu>                                    *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "responses.h"

#define HTTP_PORT 9998
#define BUF_SIZE 4096

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int client_send(int sock, int client_sock, char * data) {
    int data_len = strlen(data);
    if (send(client_sock, data, data_len, 0) != data_len) {
        close_socket(client_sock);
        close_socket(sock);
        fprintf(stderr, "Error sending to client.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("Running in: %s\n", cwd);

    int sock, client_sock;
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

    fprintf(stdout, "----- HTTP Server -----\n");

    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(HTTP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    /* finally, loop waiting for input and then write it back */
    while (1)
    {
        cli_size = sizeof(cli_addr);
        if ((client_sock = accept(sock, (struct sockaddr *) &cli_addr,
                                  &cli_size)) == -1)
        {
            close(sock);
            fprintf(stderr, "Error accepting connection.\n");
            return EXIT_FAILURE;
        }

        readret = 0;
        while((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1)
        {
            char * method, * relPath; //, * client_http_version;
            int lineCnt = 0;
            char * endBuf;
            // TODO: be able to handle just \r instead of \r\n
            char * line = strtok_r(buf, "\r\n", &endBuf);
            while (line != NULL && (strcmp(line, "\r\n") != 0 || strcmp(line, "\n") != 0)) {
                char * endTok;
                char * tok = strtok_r(line, " ", &endTok);
                int tokCnt = 0;
                while (tok != NULL) {
                    printf("%s ", tok);
                    if (lineCnt == 0) {
                        switch (tokCnt) {
                            case 0:
                                // TODO: To uppercase to avoid caps issue
                                method = tok;
                                break;
                            case 1:
                                relPath = tok;
                                break;
                            case 2:
                                // client_http_version = tok;
                                break;
                            default:
                                // Extra stuff - do nothing with it
                                break;
                        }
                    }
                    tok = strtok_r(NULL, " ", &endTok);
                    tokCnt++;
                }
                printf("\n");
                // TODO: be able to handle just \n instead of \r\n
                line = strtok_r(NULL, "\r\n", &endBuf);
                lineCnt++;
            }
            char * content = endBuf;
            char resource_path[1024];
            strcpy(resource_path, cwd);
            strcat(resource_path, relPath);

            // Checking for directory traversal attack
            char * resTok = strtok(relPath, "/");
            int pathCnt = 0;
            while (resTok != NULL && pathCnt >= 0) {
                if (strcmp(resTok, "..") == 0) {
                    pathCnt--;
                } else {
                    pathCnt++;
                }
                resTok = strtok(NULL, "/");
            }

            // TODO: Need to re-examine the paths for everything
            if (pathCnt < 0) {
                client_send(sock, client_sock, error_403);
            } else if (access(resource_path, F_OK) == -1) {
                client_send(sock, client_sock, error_404);
            } else {
                if (strcmp(method, "HEAD") == 0) {
                    client_send(sock, client_sock, success_200);
                } else if (strcmp(method, "GET") == 0) {
                    FILE *fp = fopen(resource_path, "r");
                    fseek(fp, 0, SEEK_END);
                    long fsize = ftell(fp);
                    fseek(fp, 0, SEEK_SET);

                    char *file_buf = malloc(fsize + 1);
                    fread(file_buf, fsize, 1, fp);
                    fclose(fp);

                    char response[fsize + strlen(success_200) + 2];
                    strcpy(response, success_200);
                    strcat(response, "\r\n");
                    strcat(response, file_buf);
                    free(file_buf);

                    client_send(sock, client_sock, response);
                } else if (strcmp(method, "POST") == 0) {
                    // TODO: DO "POST" ACTION
                    /*while (contentLen != NULL && strlen(content) < contentLen) {
                        if (readret = recv(client_sock, buf, BUF_SIZE, 0) >= 1) {
                            strcat(content, buf);
                        }
                    }*/
                    printf("%s", content);
                    client_send(sock, client_sock, success_200);
                } else {
                    client_send(sock, client_sock, error_501);
                }
            }

            memset(buf, '\0', BUF_SIZE);

        }

        if (readret == -1)
        {
            close_socket(client_sock);
            close_socket(sock);
            fprintf(stderr, "Error reading from client socket.\n");
            return EXIT_FAILURE;
        }

        if (close_socket(client_sock))
        {
            close_socket(sock);
            fprintf(stderr, "Error closing client socket.\n");
            return EXIT_FAILURE;
        }
    }

    close_socket(sock);

    return EXIT_SUCCESS;
}
