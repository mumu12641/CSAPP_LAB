#include <stdio.h>
#include <string.h>
#include <malloc.h>
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

void parse_uri(char *uri, request *re) {
    // http://www.cmu.edu:8080/hub/index.html
    // http://www.cmu.edu/hub/index.html

    char *str = strstr(uri, "//");
    memset(re, 0, sizeof(request));
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
        strcpy(re->host, "Host:");
        strcat(re->host, host);
        strcat(re->host, "\r\n");
        return;

    } else {
        printf("12\n");
        char *pathpose = strstr(uri, "/");
        if (pathpose != NULL) strcpy(re->path, pathpose);
        strcpy(re->port, "80");

        strcpy(re->host, "\r\n");
        return;
    }
}
int main() {
    char uri[100] = "./tiny/home.html";

    request *re = (request *)malloc(sizeof(request));
    // request re1;
    // strcpy(re1.connection, "1");
    // strcpy(re1.host, "1");
    // strcpy(re1.path, "1");
    // strcpy(re1.user_agent, "1");
    // strcpy(re1.proxy_connection, "1");
    // strcpy(re1.port, "1");
    // request *re = &re1;

    // memset(re, 0, sizeof(request));
    parse_uri(uri, re);
    // printf("%s\n", re->connection);
    // printf()
    char buf_client[100];
    strcpy(buf_client, "GET ");
    strcat(buf_client, re->path);
    strcat(buf_client, " HTTP/1.0\r\n");
    // strcat(buf_client, );
    strcat(buf_client, re->host);
    strcat(buf_client, re->user_agent);
    strcat(buf_client, re->connection);
    strcat(buf_client, re->proxy_connection);
    strcat(buf_client, "\r\n");
    printf("%s", buf_client);
    free(re);
    return 0;
}

// int main() {
//     // const char needle[10] = "NOOB";
//     // char *ret;

//     // ret = strstr(haystack, "/");

//     char *str = strstr(uri, "//");
//     str += 2;
//     printf("%s\n", str);
//     // printf("%s\n", port);
//     // int i = 0;

//     // port++;
//     // char port_number[10];
//     // for (; *port != '/'; port++, i++) {
//     //     port_number[i] = *port;
//     // }
//     // port_number[i] = '\0';
//     // printf("%s\n", test);

//     int j = 0;
//     char host[30];
//     for (; *str != '/' || *str != ':'; str++, j++) {
//         host[j] = *str;
//     }
//     host[j] = '\0';
//     printf("%s", host);

//     return 0;
// }