#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Request {
    char host[100];
    char port[100];
    char path[100];
} request;

void parse_uri(char *uri, request *linep);
int main() {
    char uri[100] = "http://www.baidu.com/tiny/home.html";
    request *re = (request *)malloc(sizeof(request));

    char *pathpose = strstr(uri, "/");
    // if (pathpose != NULL) strcpy(re->host, pathpose);
    // strcpy(re->port, "80");
    parse_uri(uri, re);
    printf("%s\n", re->host);
    printf("%s\n", re->path);
    printf("%s\n\n", re->port);
    free(re);
    return 0;
}
void parse_uri(char *uri, request *linep) {
    if (strstr(uri, "http://") != uri) {
        fprintf(stderr, "Error: invalid uri!\n");
        // exit(0);
        return;
    }
    uri += strlen("http://");
    char *c = strstr(uri, ":");
    if (c != NULL) {
        *c = '\0';
        strcpy(linep->host, uri);
        uri = c + 1;
        c = strstr(uri, "/");
        *c = '\0';
        strcpy(linep->port, uri);
        *c = '/';
        strcpy(linep->path, c);
    }else{
        char *a = strstr(uri, "/");
        *a = '\0';
        strcpy(linep->port, "80");
        strcpy(linep->host, uri);
        *a = '/';
        strcpy(linep->path, a);
    }
}