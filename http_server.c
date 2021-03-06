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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "request_handler.c"

#define BUF_SIZE 8192

void *get_in_addr(struct sockaddr *sa) {
    return sa->sa_family == AF_INET
           ? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
           : (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(int argc, char* argv[])
{


    int HTTP_PORT = atoi(argv[1]);
    //int TLS_PORT = atoi(argv[2]);
    char * LOG_DIR = argv[3];

    log_init(LOG_DIR);

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("Running in: %s\n", cwd);


    // set logfile to line buffering
    setvbuf(logFile, NULL, _IOLBF, 0);


    int sock, client_sock;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

    fd_set master; // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    char remoteIP[INET6_ADDRSTRLEN];

    fprintf(stdout, "----- HTTP Server -----\n");

    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(logFile, "Failed creating socket.\n");

        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(HTTP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;


    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {


        fprintf(logFile, "setsockopt(SO_REUSEADDR) failed.\n");
    }

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(logFile, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(logFile, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }


    FD_ZERO(&master);
    // add the listener to the master set
    FD_SET(sock, &master);

    // keep track of the biggest file descriptor - currently sock
    fdmax = sock;


    /* finally, loop waiting for input and then write it back */
    while (1) {
        read_fds = master; // copy it
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            fprintf(logFile, "select");
            return EXIT_FAILURE;
        }

        int i = 0;
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == sock) {
                    // handle new connections
                    cli_size = sizeof(cli_addr);
                    client_sock = accept(sock,
                                         (struct sockaddr *) &cli_addr,
                                         &cli_size);

                    if (client_sock == -1) {
                        close(sock);
                        fprintf(logFile, "Error accepting connection.\n");
                        return EXIT_FAILURE;
                    } else {
                        FD_SET(client_sock, &master); // add to master set
                        if (client_sock > fdmax) {    // keep track of the max
                            fdmax = client_sock;
                        }
                        printf("selectserver: new connection from %s on "
                                       "socket %d\n",
                               inet_ntop(cli_addr.sin_family,
                                         get_in_addr((struct sockaddr *) &cli_addr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               client_sock);
                        fprintf(logFile, "selectserver: new connection from %s on "
                                        "socket %d\n",
                                inet_ntop(cli_addr.sin_family,
                                          get_in_addr((struct sockaddr *) &cli_addr),
                                          remoteIP, INET6_ADDRSTRLEN),
                                client_sock);
                    }
                } else {
		    fprintf(logFile,"%s", buf);
                    int status = parseRequest(sock, i, buf, BUF_SIZE);

                    if (status == -1)
                    {
                        close_socket(client_sock);
                        close_socket(sock);
                        fprintf(logFile, "Error reading from client socket.\n");
                        return EXIT_FAILURE;
                    }
                }

                /*if (close_socket(client_sock))
                {
                    close_socket(sock);
                    fprintf(stderr, "Error closing client socket.\n");
                    return EXIT_FAILURE;
                }*/
            }
        }
    }

    close_socket(sock);

    close_log();
    return EXIT_SUCCESS;
}
