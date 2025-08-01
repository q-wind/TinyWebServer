#include "util.h"
#include "csapp.h"

void Wrap_Rio_Writen(int fd, char* buf, size_t n)
{
    if (rio_writen(fd, buf, n) == -1) {
        if (errno == EPIPE) {
            // SIGPIPE, client closed connection
            fprintf(stderr, "EPIPE: client closed connection\n");
            return;
        } else {
            unix_error("Wrap_Rio_Writen error");
        }
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
    size_t buf_len = 0;
    buf_len += snprintf(buf+buf_len, MAXLINE-buf_len, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    buf_len += snprintf(buf+buf_len, MAXLINE-buf_len, "Content-type: text/html\r\n");
    buf_len += snprintf(buf+buf_len, MAXLINE-buf_len, "Content-length: %zu\r\n\r\n", body_len);
    Wrap_Rio_Writen(fd, buf, buf_len);

    // Send response body
    Wrap_Rio_Writen(fd, body, body_len);
}