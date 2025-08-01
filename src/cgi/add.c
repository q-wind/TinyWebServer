#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAXLINE 2048

int get_param(const char* query, const char* key, int* val)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "%s=%%d", key);  // %% to escape %
    char* pos = strstr(query, key);
    if (pos) {
        if (sscanf(pos, pattern, val) == 1) {
            return 1;   // set val
        }
    }
    return 0;           // not found
}

/* 
 * add.c - a CGI program: adds two numbers
 * QUERY_STRING: first=1&second=2
 */
int main(void) {
    char *buf, *p, *method;
    char content[MAXLINE];
    int n1=0, n2=0, n1_set=0, n2_set=0;

    buf = getenv("QUERY_STRING");
    method = getenv("REQUEST_METHOD");
    if (buf) {
        n1_set = get_param(buf, "first", &n1);
        n2_set = get_param(buf, "second", &n2);
    }
    
    size_t len = 0;
    if (!n1_set || !n2_set) {
        // if not set, return 400 Bad Request
        printf("Status: HTTP/1.0 400 Bad Request\r\n");
        len += snprintf(content+len, MAXLINE-len, "<head><title>Tiny Error</title></head>\r\n");
        len += snprintf(content+len, MAXLINE-len, "<h1>400 Bad Request</h1>\r\n");
        len += snprintf(content+len, MAXLINE-len, 
                    "<p>Missing or invalid parameters. Usage: ?first=1&second=2</p>\r\n");

    } else {
        printf("Status: HTTP/1.0 200 OK\r\n");
        len += snprintf(content+len, MAXLINE-len, "<head><title>Tiny Web Server</title></head>\r\n");
        len += snprintf(content+len, MAXLINE-len, "<h1>Tiny CGI add</h1>\r\n");
        len += snprintf(content+len, MAXLINE-len, 
                            "<p>The answer is: %d + %d = %d\r\n<p>", n1, n2, n1 + n2);
    }
    len += snprintf(content+len, MAXLINE-len, "<hr>\r\n");
    len += snprintf(content+len, MAXLINE-len, "<small>The Tiny Web Server</small>\r\n");

    /* Generate the HTTP response */
    printf("Connection: close\r\n");
    printf("Content-length: %zu\r\n", len);
    printf("Content-type: text/html\r\n\r\n");
    if (strcasecmp(method, "HEAD") != 0) {
        printf("%s", content);
    }
    fflush(stdout);

    exit(0);
}