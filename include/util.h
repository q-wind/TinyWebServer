#ifndef __UTIL_H__
#define __UTIL_H__

#include <stddef.h>

void Wrap_Rio_Writen(int fd, char* buf, size_t n);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);

#endif