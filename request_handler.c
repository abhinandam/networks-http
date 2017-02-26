//
// Created by evan on 2/25/17.
//

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "responses.h"

int close_socket(int sock) {
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
        //close_socket(sock);
        fprintf(stderr, "Error sending to client.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int parseRequest(int sock, int client_sock, char * buf, int BUF_SIZE) {
    struct request * req;
    req = malloc(sizeof(struct request));
    req->expContentLength = 0;
    req->contentLength = 0;
    req->content = NULL;
    req->relPath = NULL;
    req->method = NULL;

    int readret = 0;
    int endHeader = 0;
    int rnrnCatch = 0;
    int endRequest = 0;
    int expect_continue = 0;
    int lineCnt = 0;

    // Keep reading from the socket until the request header and content are collected completely
    while (endRequest == 0) {
        int count;
        ioctl(client_sock, FIONREAD, &count);
        if (count <= 0 && endHeader == 0) {
            return 1;
        }
        if (count >= 1 && (readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1) {
            char *endBuf;
            // If the header is complete, collect content (across multiple buffers if need be)
            if (endHeader == 1) {
                if (req->contentLength >= req->expContentLength) {
                    endRequest = 1;
                    expect_continue = 0;
                } else {
                    int contentLength = strlen(buf);
                    contentLength += req->contentLength;
                    char *newContent = malloc(contentLength);
                    if (req->content != NULL) {
                        strcpy(newContent, req->content);
                        free(req->content);
                    }
                    strcpy(newContent, buf);
                    req->content = newContent;
                    req->contentLength = contentLength;
                    if (req->contentLength >= req->expContentLength) {
                        endRequest = 1;
                        expect_continue = 0;
                    }
                }
            // If the header is not complete, parse it before moving onto the data
            } else {
                char *line = strtok_r(buf, "\r\n", &endBuf);
                while (line != NULL) {
                    char *endTok;
                    char *tok = strtok_r(line, " ", &endTok);
                    int tokCnt = 0;
                    while (tok != NULL) {
                        printf("%s ", tok);
                        if (lineCnt == 0) {
                            switch (tokCnt) {
                                case 0:
                                    // TODO: To uppercase to avoid caps issue
                                    req->method = malloc(strlen(tok));
                                    strcpy(req->method, tok);
                                    break;
                                case 1:
                                    req->relPath = malloc(strlen(tok));
                                    strcpy(req->relPath, tok);
                                    break;
                                case 2:
                                    // client_http_version = tok;
                                    break;
                                default:
                                    // Extra stuff - do nothing with it
                                    break;
                            }
                        }

                        if (strcmp(tok, "Content-Length:") == 0) {
                            req->expContentLength = atoi(endTok);
                        }

                        if (strcmp(tok, "100-continue") == 0) {
                            expect_continue = 1;
                        }

                        tok = strtok_r(NULL, " ", &endTok);
                        tokCnt++;
                    }
                    printf("\n");

                    char * ret = strstr(endBuf, "\n\r\n");
                    if (ret == endBuf) {
                        rnrnCatch = 1;
                        break;
                    }
                    line = strtok_r(NULL, "\r\n", &endBuf);
                    lineCnt++;
                }
                // If the next line is blank (only \r\n), consider the header complete
                // and collect the content
                //if (strcmp(line, "\r\n") == 0) {
                if (rnrnCatch == 1 || strcmp(buf, "\r\n") == 0) {
                    if (expect_continue == 1) {
                        client_send(sock, client_sock, continue_100);
                    }
                    endHeader = 1;
                    if (strlen(endBuf) > 3) {
                        endBuf += 3;
                        int contentLength = strlen(endBuf);
                        req->content = malloc(contentLength);
                        strcpy(req->content, endBuf);
                        req->contentLength += contentLength;
                    }
                }
                memset(buf, '\0', BUF_SIZE);
            }
        } else {
            if (expect_continue == 0) {
                endRequest = 1;
            }
        }
    }

    // Finished collecting header and content, now figure out the response

    char resource_path[1024];

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    strcpy(resource_path, cwd);
    strcat(resource_path, req->relPath);

    // Checking for directory traversal attack
    char *resTok = strtok(req->relPath, "/");
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
        if (strcmp(req->method, "HEAD") == 0) {
            client_send(sock, client_sock, success_200);
        } else if (strcmp(req->method, "GET") == 0) {
            FILE *fp = fopen(resource_path, "r");
            fseek(fp, 0, SEEK_END);
            long fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            char *file_buf = malloc(fsize + 1);
            fread(file_buf, fsize, 1, fp);
            fclose(fp);

            char response[fsize + strlen(success_200) + 2];
            strcpy(response, success_200);
            strcat(response, file_buf);
            strcat(response, "\r\n");
            free(file_buf);

            client_send(sock, client_sock, response);
        } else if (strcmp(req->method, "POST") == 0) {
            // TODO: DO "POST" ACTION
            /*while (contentLen != NULL && strlen(content) < contentLen) {
                if (readret = recv(client_sock, buf, BUF_SIZE, 0) >= 1) {
                    strcat(content, buf);
                }
            }*/
            printf("%s", req->content);

            /*FILE *fp;
            int x = 10;

            fp=fopen("content.txt", "w");
            if(fp == NULL)
                exit(-1);
            fprintf(fp, "%s", req->content);
            fclose(fp);*/

            free(req->content);
            client_send(sock, client_sock, success_200);
        } else {
            client_send(sock, client_sock, error_501);
        }
    }

    free(req);

    if (readret == -1) {
        return -1;
    } else {
        return 1;
    }
}



