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
void sigchild_handler(int sig);     // 11.8


int main(int argc, char** argv){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;     // large enough to hold any address
    char hostname[MAXLINE], port[MAXLINE];

    Signal(SIGCHLD, sigchild_handler);

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

    if (strstr(uri, "..")) {
        clienterror(fd, uri, "403", "Forbidden", 
                    "Path traversal is not allowed, including \"..\" in");
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
    size_t body_len = 0;

    // construct response body safely
    body_len += snprintf(body + body_len, MAXBUF - body_len, "<html>\r\n");
    body_len += snprintf(body + body_len, MAXBUF - body_len, "<head><title>Tiny Error</title></head>\r\n");
    body_len += snprintf(body + body_len, MAXBUF - body_len, "<body>\r\n");
    body_len += snprintf(body + body_len, MAXBUF - body_len, "<h1>%s: %s</h1>\r\n", errnum, shortmsg);
    body_len += snprintf(body + body_len, MAXBUF - body_len, "<p>%s: %s</p>\r\n", longmsg, cause);
    body_len += snprintf(body + body_len, MAXBUF - body_len, "<hr>\r\n");
    body_len += snprintf(body + body_len, MAXBUF - body_len, "<small>The Tiny Web Server</small>\r\n");
    body_len += snprintf(body + body_len, MAXBUF - body_len, "</body>\r\n</html>\r\n");

    // Send response headers safely
    snprintf(buf, MAXLINE, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    snprintf(buf, MAXLINE, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    snprintf(buf, MAXLINE, "Content-length: %zu\r\n\r\n", body_len);
    Rio_writen(fd, buf, strlen(buf));

    // Send response body
    Rio_writen(fd, body, body_len);
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
        if (uri[strlen(uri)-1] == '/') {
            snprintf(filename, MAXLINE, "public%sindex.html", uri);
        } else {
            snprintf(filename, MAXLINE, "public%s", uri);
        }
        return 1;
    } else {    // dynamic content
        // strchr(a, c): find c in a, return ptr
        ptr = strchr(uri, '?');
        if (ptr) {
            snprintf(cgiargs, MAXLINE, "%s", ptr+1);
            *ptr = '\0';
        } else {
            strcpy(cgiargs, "");    // No overflow risk
        }
        snprintf(filename, MAXLINE, ".%s", uri);
        return 0;
    }
}

void serve_static(int fd, char* filename, int filesize)
{
    char filetype[MAXLINE], buf[MAXBUF];
    size_t header_len = 0;
    get_filetype(filename, filetype);

    // send response header safely
    header_len += snprintf(buf + header_len, MAXBUF - header_len, "HTTP/1.0 200 OK\r\n");
    header_len += snprintf(buf + header_len, MAXBUF - header_len, "Server: Tiny Web Server\r\n");
    header_len += snprintf(buf + header_len, MAXBUF - header_len, "Connection: close\r\n");
    header_len += snprintf(buf + header_len, MAXBUF - header_len, "Content-length: %d\r\n", filesize);
    header_len += snprintf(buf + header_len, MAXBUF - header_len, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, header_len);

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
    // get last . position
    char* ext = strrchr(filename, '.');
    if (!ext) {
        strcpy(filetype, "text/plain");
        return;
    }
    
    if (strcmp(ext, ".html") == 0)
        strcpy(filetype, "text/html");
    else if (strcmp(ext, ".png") == 0)
        strcpy(filetype, "image/png");
    else if (strcmp(ext, ".gif") == 0)
        strcpy(filetype, "image/gif");
    else if (strcmp(ext, ".jpg") == 0)
        strcpy(filetype, "image/jpeg");
    else if (strcmp(ext, ".flac") == 0)
        strcpy(filetype, "audio/flac");
    else if (strcmp(ext, ".mp4") == 0)
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char* filename, char* cgiargs)
{
    char buf[MAXLINE];
    char* emptylist[] = { NULL };
    // send partial header, the remaining part is sent by the cgi program
    snprintf(buf, MAXLINE, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    snprintf(buf, MAXLINE, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) {  // child
        setenv("QUERY_STRING", cgiargs, 1); // argument pass by environment
        Dup2(fd, STDOUT_FILENO);    // redirect stdout to client socket
        Execve(filename, emptylist, environ);   // run cgi program
    }
    // parent reaps child in sigchld_handle
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

void sigchild_handler(int sig)
{
    int olderrno = errno;
    int status;
    pid_t pid;
    // -1: wait for any child, WNOHANG: if no child ready return 0
    // do not use Waitpid() in csapp.c, since no children return -1, Waitpid() considers it as an error
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Reap child
    }
    errno = olderrno;
}