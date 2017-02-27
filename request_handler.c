//
// Created by evan on 2/25/17.
//

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include "request_handler.h"
#include "responses.h"


void convertToUpperCase(char *sPtr) {
    while(*sPtr != '\0') {
        *sPtr = toupper((unsigned char) *sPtr);
        sPtr++;
    }
}

int close_socket(int sock) {
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int client_send(int sock, int client_sock, struct response * resp) {
    char * responseStr;
    char contentLengthStr[20] = "";
    size_t respLength = strlen(resp->contentTemplate) + 2;
    if (resp->content != NULL) {
        respLength += resp->contentLength;
    }
    //if (resp->contentLength != 0) {
        sprintf(contentLengthStr, "%d", (int)resp->contentLength);
        respLength += strlen(contentLengthLabel) + strlen(contentLengthStr) + 2;
    //}
    if (resp->contentType != NULL) {
        respLength += strlen(contentTypeLabel) + strlen(resp->contentType) + 2;
    }

    responseStr = malloc(respLength);
    strcpy(responseStr, resp->contentTemplate);
    //if (resp->contentLength != 0) {
        strcat(responseStr, contentLengthLabel);
        strcat(responseStr, contentLengthStr);
        strcat(responseStr, "\r\n");
    //}
    if (resp->contentType != NULL) {
        strcat(responseStr, contentTypeLabel);
        strcat(responseStr, resp->contentType);
        strcat(responseStr, "\r\n");
    }
    if (resp->content != NULL) {
        strcat(responseStr, "\r\n");
        strcat(responseStr, resp->content);
    }
    strcat(responseStr, "\r\n");
    //printf("%s", responseStr);
    resp->contentTemplate = success_200;
    if (send(client_sock, responseStr, respLength, 0) != respLength) {
        close_socket(client_sock);
        //close_socket(sock);
        fprintf(stderr, "Error sending to client.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int parseRequest(int sock, int client_sock, char * buf, size_t BUF_SIZE) {
    struct request * req;
    req = malloc(sizeof(struct request));
    req->expContentLength = 0;
    req->contentLength = 0;
    req->expCont = 0;
    req->content = NULL;
    req->relPath = NULL;
    req->method = NULL;

    struct response * resp;
    resp = malloc(sizeof(struct response));
    resp->contentLength = 0;
    resp->content = NULL;
    resp->contentTemplate = success_200;
    resp->contentType = NULL;

    ssize_t readret = 0;
    int endHeader = 0;
    int rnrnCatch = 0;
    int endRequest = 0;
    int lineCnt = 0;
    int contLengthExists = 0;

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
                    req->expCont = 0;
                } else {
                    size_t contentLength = strlen(buf);
                    contentLength += req->contentLength;
                    char * newContent;
                    newContent = malloc((size_t)contentLength);
                    if (req->content != NULL) {
                        strcpy(newContent, req->content);
                        free(req->content);
                    }
                    strcpy(newContent, buf);
                    req->content = newContent;
                    req->contentLength = contentLength;
                    if (req->contentLength >= req->expContentLength) {
                        endRequest = 1;
                        req->expCont = 0;
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
                                    convertToUpperCase(tok);
                                    req->method = malloc(strlen(tok) + 1);
                                    strcpy(req->method, tok);
                                    if (strcmp(req->method, "HEAD") != 0 &&
                                        strcmp(req->method, "GET") != 0 &&
                                        strcmp(req->method, "POST") != 0) {
                                        endHeader = 1;
                                        endRequest = 1;
                                    }
                                    break;
                                case 1:
                                    req->relPath = malloc(strlen(tok) + 1);
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
                            contLengthExists = 1;
                        }

                        if (strcmp(tok, "100-continue") == 0) {
                        //if (req->method != NULL && strcmp(req->method, "POST") == 0) {
                            req->expCont = 1;
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
                    if (req->expCont == 1) {
                        resp->contentTemplate = continue_100;
                        client_send(sock, client_sock, resp);
                    }
                    endHeader = 1;
                    if (strlen(endBuf) > 3) {
                        endBuf += 3;
                        size_t contentLength = strlen(endBuf);
                        req->content = malloc((size_t)contentLength + 1);
                        strcpy(req->content, endBuf);
                        req->contentLength += contentLength;
                    }
                }
                memset(buf, '\0', BUF_SIZE);
            }
        } else {
            if (req->expCont == 0) { // && req->expContentLength >= req->contentLength
                endRequest = 1;
            }
        }
    }

    // Finished collecting header and content, now figure out the response

    char resource_path[1024];

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    strcpy(resource_path, cwd);

    int pathCnt = 0;

    if(req->relPath != NULL && strlen(req->relPath) > 0) {
        strcat(resource_path, req->relPath);

        // Checking for directory traversal attack
        char *resTok = strtok(req->relPath, "/");
        while (resTok != NULL && pathCnt >= 0) {
            if (strcmp(resTok, "..") == 0) {
                pathCnt--;
            } else {
                pathCnt++;
            }
            resTok = strtok(NULL, "/");
        }
    } else {
        strcat(resource_path, "/");
    }

    // TODO: Need to re-examine the paths for everything
    if (pathCnt < 0) {
        resp->contentTemplate = error_403;
    } else if (access(resource_path, F_OK) == -1) {
        resp->contentTemplate = error_404;
    } else {
        if (strcmp(req->method, "HEAD") == 0 || strcmp(req->method, "GET") == 0) {
            FILE *fp = fopen(resource_path, "r");
            fseek(fp, 0, SEEK_END);
            long fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            char *file_buf = malloc((size_t)fsize + 1);
            fread(file_buf, (size_t)fsize, 1, fp);
            fclose(fp);

            resp->contentLength = (size_t)fsize;

            if (strcmp(req->method, "GET") == 0) {
                resp->content = malloc((size_t)fsize);
                strcpy(resp->content, file_buf);
            }
            free(file_buf);
        } else if (strcmp(req->method, "POST") == 0) {
            // TODO: DO "POST" ACTION
            /*while (contentLen != NULL && strlen(content) < contentLen) {
                if (readret = recv(client_sock, buf, BUF_SIZE, 0) >= 1) {
                    strcat(content, buf);
                }
            }*/
            printf("\nSTART-CONTENT\n");
            printf("%s", req->content);
            printf("\nEND-CONTENT\n");

            /*FILE *fp;
            int x = 10;

            fp=fopen("content.txt", "w");
            if(fp == NULL)
                exit(-1);
            fprintf(fp, "%s", req->content);
            fclose(fp);*/

            if (contLengthExists == 0) {
                resp->contentTemplate = error_411;
            }

            free(req->content);
        } else {
            resp->contentTemplate = error_501;
        }

        client_send(sock, client_sock, resp);

    }

    free(req);
    free(resp);

    if (readret == -1) {
        return -1;
    } else {
        return 1;
    }
}



