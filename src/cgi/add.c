#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "csapp.h"
#include "util.h"

/* 
 * add.c - a CGI program: adds two numbers
 * QUERY_STRING: first=1&second=2
 */
int main(void) {
    char *buf, *p;
    char content[MAXLINE];
    int n1=0, n2=0;

    /* Extract the two arguments */
    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        if (p != NULL) {
            *p = '\0';
            sscanf(buf, "first=%d", &n1);
            sscanf(p+1, "second=%d", &n2);

            /* Make the response body */
            size_t len = 0;
            len += snprintf(content+len, MAXLINE-len, "<h1>Tiny CGI add</h1>\r\n");
            len += snprintf(content+len, MAXLINE-len, 
                                "<p>The answer is: %d + %d = %d\r\n<p>", n1, n2, n1 + n2);
            len += snprintf(content+len, MAXLINE-len, "<hr>\r\n");
            len += snprintf(content+len, MAXLINE-len, "<small>The Tiny Web Server</small>\r\n");
            content[len] = '\0';
        
            /* Generate the HTTP response */
            printf("Connection: close\r\n");
            printf("Content-length: %d\r\n", (int)strlen(content));
            printf("Content-type: text/html\r\n\r\n");
            printf("%s", content);
            fflush(stdout);
            exit(0);
        }
    }

    clienterror(STDOUT_FILENO, buf, "400", "Bad Request", 
                        "Tiny couldn't parse this request");
    exit(0);
}