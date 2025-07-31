#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "csapp.h"

void doit(int fd);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);
void read_requesthdrs(rio_t* rp);
int parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize);
void serve_dynamic(int fd, char* filename,  char* cgiargs);
void get_filetype(char* filename, char* filetype);

// chapter11 homework
void echo(int fd);      // 11.6


int main(int argc, char** argv){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;     // large enough to hold any address
    char hostname[MAXLINE], port[MAXLINE];

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        Getnameinfo((struct sockaddr*)&clientaddr, clientlen, 
            hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        // do work
        doit(connfd);
        // echo(connfd);

        Close(connfd);
    }
}

void doit(int fd)
{
    rio_t rio;
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];   // request line
    
    Rio_readinitb(&rio, fd);

    // read request line
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("[[ Request headers ]]:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    // now support GET only
    if (strcasecmp(method, "GET") != 0) {
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny dose not implement this method");
        return; 
    }

    // ignore request header
    read_requesthdrs(&rio);

    // parse URI from GET request
    int is_static;      // serve-static or serve-dynamic
    char filename[MAXLINE], cgiargs[MAXLINE];
    struct stat statbuf;

    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &statbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", 
                "Tiny couldn't find this file");
        return;
    }

    if (is_static) {    // serve static content
        // not a regular file or user can't read
        if (!(S_ISREG(statbuf.st_mode)) || !(S_IRUSR & statbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read this file");
            return;
        }
        serve_static(fd, filename, statbuf.st_size);
    } else {            // serve dynamic content
        // not a regular file or user can't execute
        if (!(S_ISREG(statbuf.st_mode)) || !(S_IXUSR & statbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run this CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }

}

// wrap status code and message into response
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    // construct response body
    char *p = body;
    p += sprintf(p, "<html>\r\n");
    p += sprintf(p, "<head><title>Tiny Error</title></head>\r\n");
    p += sprintf(p, "<body>\r\n");
    p += sprintf(p, "<h1>%s: %s</h1>\r\n", errnum, shortmsg);
    p += sprintf(p, "<p>%s: %s</p>\r\n", longmsg, cause);
    p += sprintf(p, "<hr>\r\n");
    p += sprintf(p, "<small>The Tiny Web Server</small>\r\n");
    p += sprintf(p, "</body>\r\n</html>\r\n");
    // user snprintf() to avoid buffer overflow
    // size_t len = 0;
    // len += snprintf(body+len, sizeof(body)-len, "<html>...")

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);   // response line
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");            // response header
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));  // end with a empty line \r\n
    Rio_writen(fd, buf, strlen(buf));

    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t* rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n") != 0) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

// Assumption: 
// the document root / is ./public/
// the directory of executable files is ./cgi-bin/
int parse_uri(char* uri, char* filename, char* cgiargs)
{
    char* ptr;
    // strstr(a, b): find b in a, return ptr or NULL
    if (!strstr(uri, "cgi-bin")) {  // static content
        strcpy(cgiargs, "");
        strcpy(filename, "public");
        strcat(filename, uri);
        
        if (uri[strlen(uri)-1] == '/') {
            strcat(filename, "index.html");
        }
        return 1;
    } else {    // dynamic content
        // strchr(a, c): find c in a, return ptr
        ptr = strchr(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        } else {
            strcpy(cgiargs, "");    // empty argument
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char* filename, int filesize)
{
    char filetype[MAXLINE], buf[MAXBUF];
    get_filetype(filename, filetype);

    // send response header
    char* p = buf;
    p += sprintf(p, "HTTP/1.0 200 OK\r\n");
    p += sprintf(p, "Server: Tiny Web Server\r\n");
    p += sprintf(p, "Connection: close\r\n");
    p += sprintf(p, "Content-length: %d\r\n", filesize);
    p += sprintf(p, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf));

    printf("[[ Response headers ]]:\n");
    printf("%s", buf);

    // send file
    int srcfd = Open(filename, O_RDONLY, 0);
    char* srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);       // can close fd after mmap
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

// derive file type from filename
void get_filetype(char* filename, char* filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".flac"))
        strcpy(filetype, "audio/flac");
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char* filename, char* cgiargs)
{
    char buf[MAXLINE];
    char* emptylist[] = { NULL };
    // send partial header, the remaining part is sent by the cgi program
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) {  // child
        setenv("QUERY_STRING", cgiargs, 1); // argument pass by environment
        Dup2(fd, STDOUT_FILENO);    // redirect stdout to client socket
        Execve(filename, emptylist, environ);   // run cgi program
    }
    Wait(NULL); // parent waits for and reaps child
}

void echo(int fd)
{
    ssize_t n;
    rio_t rio;
    char buf[MAXLINE];
    
    Rio_readinitb(&rio, fd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        if (strcmp(buf, "\r\n") == 0)
            break;
        Rio_writen(fd, buf, n);
    }
}