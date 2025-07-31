#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
 * add.c - a simple CGI program that adds two numbers
 * passed in the QUERY_STRING environment variable.
 */
int main(void) {
    char *buf, *p;
    char arg1[1024], arg2[1024], content[1024];
    int n1=0, n2=0;

    /* Extract the two arguments */
    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        if (p == NULL) {
            // If only one argument, handle it gracefully or set a default for the second
            strcpy(arg1, buf);
            n1 = atoi(arg1);
            n2 = 0; // Or handle as an error
        } else {
            *p = '\0';
            strcpy(arg1, buf);
            strcpy(arg2, p+1);
            n1 = atoi(arg1);
            n2 = atoi(arg2);
        }
    }

    /* Make the response body */
    sprintf(content, "Welcome to add.com: THE Internet addition portal.\r\n<p>");
    
    char temp[256];
    sprintf(temp, "The answer is: %d + %d = %d\r\n<p>", n1, n2, n1 + n2);
    strcat(content, temp);

    strcat(content, "Thanks for visiting!\r\n");
  
    /* Generate the HTTP response */
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}