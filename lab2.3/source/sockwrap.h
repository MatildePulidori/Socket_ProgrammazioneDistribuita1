/*
 
 module: sockwrap.h
 
 purpose: definitions of functions in sockwrap.c
 
 reference: Stevens, Unix network programming (3ed)
 
 */


#ifndef _SOCKWRAP_H

#define _SOCKWRAP_H

#define SA struct sockaddr

#define INTERRUPTED_BY_SIGNAL (errno == EINTR)

ssize_t readline (int fd, void *vptr, size_t maxlen);

ssize_t Readline (int fd, void *ptr, size_t maxlen);

#endif
