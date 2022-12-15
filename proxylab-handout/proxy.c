#include <stdio.h>

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

typedef struct Request {
    char host[100];
    char user_agent[100];
    char connection[100];
    char proxy_connection[100];
    char port[100];
    char path[100];
} request;

void doit(int fd);
// int parse_uri(char *uri,  char *filename, char *cgiargs);
void parse_uri(char *uri, request *re);
void read_requesthdrs(rio_t *rp);

int main(int argc, char **argv) {
    // pb: 10210

    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,
                    MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }

    // printf("%s", user_agent_hdr);
    return 0;
}

void doit(int fd) {
    char buf[MAXLINE], buf_client[MAXLINE], method[MAXLINE], uri[MAXLINE],
        version[MAXLINE];

    // char set_
    request *re = (request *)malloc(sizeof(request));
    memset(re, 0, sizeof(request));

    rio_t rio, rio_client;
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) return;
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
        // clienterror(fd, method, "501", "Not Implemented",
        //             "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);
    parse_uri(uri, re);

    int serverfd = Open_clientfd(re->host, re->port);
    Rio_readinitb(&rio_client, serverfd);
    // strcpy(buf_client, re->connection);
    // char re_cmd[MAXLINE];
    strcpy(buf_client, "GET ./tiny/home.html HTTP/1.0\r\n");
    // strcat(buf_client, re->path);
    // strcat(buf_client, " HTTP/1.0\r\n");
    // // strcat(buf_client, );
    
    // strcat(buf_client, re->user_agent);
    // strcat(buf_client, re->connection);
    // strcat(buf_client, re->proxy_connection);
    // strcat(buf_client, re->host);
    strcat(buf_client, "\r\n");
    Rio_writen(serverfd, buf_client, strlen(buf_client));

    size_t n;

    while ((n == Rio_readlineb(&rio_client, buf, MAXLINE)) != 0) {
        printf("proxy received %d bytes,then send\n", (int)n);
        Rio_writen(fd, buf, n);
    }
    Close(serverfd);
    free(re);
    // Rio_writen(fd, buf_new, strlen(buf_new));
}

void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n")) {  // line:netp:readhdrs:checkterm
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

void parse_uri(char *uri, request *re) {
    // http://www.cmu.edu:8080/hub/index.html
    // http://www.cmu.edu/hub/index.html

    char *str = strstr(uri, "//");
    
    strcpy(re->user_agent, user_agent_hdr);
    strcpy(re->connection, "Connection: close\r\n");
    strcpy(re->proxy_connection, "Proxy-Connection: close\r\n");

    if (str != NULL) {
        str += 2;
        char *port = strstr(str, ":");
        if (port != NULL) {
            port++;
            int i = 0;
            char port_number[10];
            for (; *port != '/'; port++, i++) {
                port_number[i] = *port;
            }
            port_number[i] = '\0';
            strcpy(re->port, port_number);
        } else {
            strcpy(re->port, "80");
        }
        char *tmp;
        tmp = strstr(str, "/");
        strcpy(re->path, tmp);
        int j = 0;
        char host[30];
        for (; *str != '/' && *str != ':'; str++, j++) {
            host[j] = *str;
        }
        host[j] = '\0';
        
        strcpy(re->host, "Host: ");
        strcat(re->host, host);
        strcat(re->host, "\r\n");
        return;

    } else {
        char *pathpose = strstr(uri, "/");
        if (pathpose != NULL) strcpy(re->host, pathpose);
        strcpy(re->port, "80");

        strcpy(re->host, "\r\n");
        return;
    }
}
// int parse_uri(char *uri, char *filename, char *cgiargs) {
//     char *ptr;

//     if (!strstr(uri, "cgi-bin")) {
//         /* Static content */                // line:netp:parseuri:isstatic
//         strcpy(cgiargs, "");                // line:netp:parseuri:clearcgi
//         strcpy(filename, ".");              //
//         line:netp:parseuri:beginconvert1 strcat(filename, uri); //
//         line:netp:parseuri:endconvert1 if (uri[strlen(uri) - 1] == '/')    //
//         line:netp:parseuri:slashcheck
//             strcat(filename, "home.html");  //
//             line:netp:parseuri:appenddefault
//         return 1;
//     } else { /* Dynamic content */  // line:netp:parseuri:isdynamic
//         ptr = index(uri, '?');      // line:netp:parseuri:beginextract
//         if (ptr) {
//             strcpy(cgiargs, ptr + 1);
//             *ptr = '\0';
//         } else
//             strcpy(cgiargs, "");  // line:netp:parseuri:endextract
//         strcpy(filename, ".");    // line:netp:parseuri:beginconvert2
//         strcat(filename, uri);    // line:netp:parseuri:endconvert2
//         return 0;
//     }
// }