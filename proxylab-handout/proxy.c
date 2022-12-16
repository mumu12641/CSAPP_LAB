#include <stdio.h>

#include "csapp.h"
#include "sbuf.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define NTHREADS 4
#define SBUFSIZE 16

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

typedef struct _request {
    char host[100];
    char port[100];
    char path[100];
} request;

typedef struct _cache_obj {
    int flag;
    char uri[MAXLINE];
    char respBody[MAX_OBJECT_SIZE];
    int respBodyLen;
    int LRU;

} obj;

typedef struct _cache {
    // obj *head;
    // obj *tail;
    obj **objs;
    int num;
} cache;

void init_cache(cache *c);
void replaceHTTPVersion(char *);
void doit(int fd);
void parse_uri(char *uri, request *re);
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

void *thread(void *vargp);
obj *search_cache(char *uri, int fd);
void insert_cache(obj *o);

sbuf_t sbuf; /* Shared buffer of connected descriptors */
static cache proxy_cache;
static int cacheSize = 0;
static int read_cnt = 0;
static sem_t mutex;
static sem_t w;

int main(int argc, char **argv) {
    // pb: 10210

    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    pthread_t tid;
    struct sockaddr_storage clientaddr;


    init_cache(&proxy_cache);
    Sem_init(&mutex, 0, 1);
    Sem_init(&w, 0, 1);

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);
    for (int i = 0; i < NTHREADS; i++) {
        Pthread_create(&tid, NULL, thread, NULL);
    }
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,
                    MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        sbuf_insert(&sbuf, connfd);
    }

    return 0;
}
void init_cache(cache *c) {
    c->objs = Malloc(sizeof(obj *) * 10);

    c->num = 0;
}
void *thread(void *vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        Close(connfd);
    }
}

void doit(int fd) {
    char buf[MAXLINE], buf_client[MAXLINE], method[MAXLINE], uri[MAXLINE],
        version[MAXLINE],tmp_uri[MAXLINE];

    request *re = (request *)malloc(sizeof(request));
    memset(re, 0, sizeof(request));
    rio_t rio, rio_client;
    Rio_readinitb(&rio, fd);

    if (!Rio_readlineb(&rio, buf, MAXLINE)) return;
    replaceHTTPVersion(buf);
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    strcpy(tmp_uri, uri);

    obj *pointer = search_cache(uri, fd);
    if (pointer != NULL) {
        printf("cache hit\n");
        Rio_writen(fd, pointer->respBody, pointer->respBodyLen);
        return;
    }
    printf("cache miss\n");
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }
    parse_uri(uri, re);

    strcpy(buf_client, "GET ");
    strcat(buf_client, re->path);
    strcat(buf_client, " HTTP/1.0\r\n");
    strcat(buf_client, "Host: ");
    strcat(buf_client, re->host);
    strcat(buf_client, "\r\n");
    strcat(buf_client, user_agent_hdr);
    strcat(buf_client, "Connection: close\r\n");
    strcat(buf_client, "Proxy-Connection: close\r\n");

    strcat(buf_client, "\r\n");

    int clientfd = Open_clientfd(re->host, re->port);
    Rio_readinitb(&rio_client, clientfd);

    Rio_writen(rio_client.rio_fd, buf_client, strlen(buf_client));

    char tinyResponse[MAXLINE];
    char *tinyResponseP = tinyResponse;

    int n;
    while ((n = Rio_readnb(&rio_client, tinyResponse, MAXLINE)) != 0) {
        Rio_writen(fd, tinyResponse, n);
        P(&w);
        obj *o = (obj *)Malloc(sizeof(obj));
        // o->flag = 1;
        strcpy(o->respBody, tinyResponse);
        strcpy(o->uri, tmp_uri);
        o->respBodyLen = n;
        o->LRU = 0;
        printf("insert cache %s ", o->uri);
        insert_cache(o);
        printf("size %d     cache num %d\n", o->respBodyLen,proxy_cache.num);
        V(&w);
    }
}

obj *search_cache(char *uri, int fd) {
    P(&mutex);
    read_cnt++;
    if (read_cnt == 1) {
        P(&w);
    }
    V(&mutex);
    obj *p = NULL;
    rio_t rio;
    Rio_readinitb(&rio, fd);
    for (int i = 0; i < proxy_cache.num; i++) {
        printf("%s  %s\n", proxy_cache.objs[i]->uri,uri);
        if (strcmp(uri, proxy_cache.objs[i]->uri) == 0) {
            p = proxy_cache.objs[i];
            p->LRU = 0;
        } else {
            (proxy_cache.objs[i]->LRU)++;
        }
    }
    P(&mutex);
    read_cnt--;
    if (read_cnt == 0) {
        V(&w);
    }
    V(&mutex);
    return p;
}
void insert_cache(obj *o) {
    if (o->respBodyLen + cacheSize > MAX_CACHE_SIZE) {
        int max = 0;
        int max_index = 0;
        for (int i = 0; i < proxy_cache.num; i++) {
            if (proxy_cache.objs[i]->LRU > max) {
                max = proxy_cache.objs[i]->LRU;
                max_index = i;
            }
        }
        Free(proxy_cache.objs[max_index]);
        proxy_cache.objs[max_index] = o;

    } else {
        proxy_cache.objs[proxy_cache.num] = o;
        proxy_cache.num++;
    }
}

void replaceHTTPVersion(char *buf) {
    char *pos = NULL;
    if ((pos = strstr(buf, "HTTP/1.1")) != NULL) {
        buf[pos - buf + strlen("HTTP/1.1") - 1] = '0';
    }
    return;
}

void parse_uri(char *uri, request *linep) {
    // "http://localhost:15213/home.html"
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
    } else {
        char *a = strstr(uri, "/");
        *a = '\0';
        strcpy(linep->port, "80");
        strcpy(linep->host, uri);
        *a = '/';
        strcpy(linep->path, a);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf,
            "<body bgcolor="
            "ffffff"
            ">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */