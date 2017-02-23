//
// Created by evan on 2/19/17.
//

#ifndef RESPONSES_H
#define RESPONSES_H

char success_200[] = "HTTP/1.1 200 OK\r\n"
                     "Connection: Closed\r\n";

char error_400[] = "HTTP/1.1 400 Bad Request\r\n"
                   "Connection: Closed\r\n";

char error_401[] = "HTTP/1.1 401 Unauthorized\r\n"
                   "Connection: Closed\r\n"
                   "Content-Type: text/html\r\n";

char error_403[] = "HTTP/1.1 403 Forbidden\r\n"
                   "Connection: Closed\r\n"
                   "Content-Type: text/html\r\n";

char error_404[] = "HTTP/1.1 404 Not Found\r\n"
                   "Connection: Closed\r\n"
                   "Content-Type: text/html\r\n";


char error_501[] = "HTTP/1.1 501 Method Unimplemented\r\n"
                   "Connection: Closed\r\n"
                   "Content-Type: text/html\r\n";

#endif //RESPONSES_H