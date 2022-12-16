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
    // char respHeader[MAX_OBJECT_SIZE];
    // int respHeaderLen;
    int LRU;
    struct _cache_obj *prev;
    struct _cache_obj *next;

} obj;

typedef struct _cache {
    obj *head;
    obj *tail;
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

    // proxy_cache.num = 0;
    // memset(&proxy_cache, 0, sizeof(cache));
    
    init_cache(&proxy_cache);
    Sem_init(&mutex, 0, 3);
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
    c->head = Malloc(sizeof(obj));
    c->head->flag = 0;
    c->head->prev = NULL;
    c->head->next = NULL;

    c->tail = Malloc(sizeof(obj));
    c->tail->flag = 0;
    c->tail->prev = NULL;
    c->tail->next = NULL;

    c->head->next = c->tail;
    c->tail->prev = c->head;

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
        version[MAXLINE];

    request *re = (request *)malloc(sizeof(request));
    memset(re, 0, sizeof(request));
    rio_t rio, rio_client;
    Rio_readinitb(&rio, fd);

    if (!Rio_readlineb(&rio, buf, MAXLINE)) return;
    replaceHTTPVersion(buf);
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    obj *pointer = search_cache(uri, fd);
    // if (pointer != NULL) {
    //     printf("cache hit\n");
    //     Rio_writen(fd, pointer->respBody, pointer->respBodyLen);
    //     return;
    // }
    // printf("cache miss\n");
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

    // int n, totalBytes = 0;
    // obj *o = Malloc(sizeof(obj));
    // o->flag = '0';
    // strcpy(o->uri, uri);
    // *o->respHeader = 0;
    // *o->respBody = 0;
    // while ((n = rio_readlineb(&rio_client, tinyResponse, MAXLINE)) != 0) {
    //     Rio_writen(fd, tinyResponse, n);

    //     if (strcmp(tinyResponse, "\r\n") == 0)  // prepare for body part
    //         break;

    //     strcat(o->respHeader, tinyResponse);
    //     totalBytes += n;
    // }
    // printf("head:%s\n", o->respHeader);
    // o->respHeaderLen = totalBytes;
    // totalBytes = 0;
    // while ((n = rio_readlineb(&rio_client, tinyResponse, MAXLINE)) != 0) {
    //     Rio_writen(fd, tinyResponse, n);
    //     totalBytes += n;
    //     strcat(o->respBody, tinyResponse);
    // }
    // o->respBodyLen = totalBytes;
    // if (totalBytes >= MAX_OBJECT_SIZE) {
    //     Free(o);
    //     return;
    // }
    // printf("body:%s\n", o->respBody);
    // P(&w);
    // insert_cache(o);
    // V(&w);



    int n;
    while ((n = Rio_readnb(&rio_client, tinyResponse, MAXLINE)) != 0) {
        Rio_writen(fd, tinyResponse, n);
        P(&w);
        obj *o = (obj * ) Malloc(sizeof(obj));
        o->flag = 1;
        strcpy(o->respBody, tinyResponse);
        o->respBodyLen = n;
        insert_cache(o);
        V(&w);
    }
}

obj *search_cache(char *uri, int fd) {
    printf("searching\n");
    P(&mutex);
    printf("searching\n");

    read_cnt++;
    if (read_cnt == 1) {
        P(&w);
    }
    V(&mutex);
    obj *p = proxy_cache.head->next;
    rio_t rio;
    Rio_readinitb(&rio, fd);
    while (p->flag != 0) {
        if (strcmp(uri, p->uri) ==  0) {
            break;
        }
        p = p->next;
    }

    P(&mutex);
    read_cnt--;
    if (read_cnt == 0) {
        V(&w);
    }
    V(&mutex);
    if (p == proxy_cache.head->next){
        return NULL;
    }
    return p;
}
void insert_cache(obj *o) {
    while (o->respBodyLen + cacheSize > MAX_CACHE_SIZE &&
           proxy_cache.head->next != proxy_cache.tail) {
        obj *last = proxy_cache.tail->prev;
        last->next->prev = last->prev;
        last->prev->next = last->next;

        last->next = NULL;
        last->prev = NULL;
        Free(last);
    }

    o->next = proxy_cache.head->next;
    o->prev = proxy_cache.head;
    proxy_cache.head->next->prev = o;
    proxy_cache.head->next = o;
    cacheSize += o->respBodyLen;
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