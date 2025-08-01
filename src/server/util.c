#include "util.h"
#include "csapp.h"

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