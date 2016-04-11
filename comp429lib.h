#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include<sys/stat.h>
#include<netdb.h>
#include<sys/time.h>


#define WWW 1
#define DEFAULT 0
#define MAXLINE 8192
#define MAXBUF 8192
#define MAXSIZE 65535

/*a io_data structure to maintain an internal buffer for robust IO operation*/
typedef struct {
    int descriptor;          /* descriptor for this internal buf */
    int unread_st;           /* unread bytes in internal buf */
    char *unread_st_buf;     /* next unread byte in internal buf */
    char read_buf[MAXLINE];  /* internal buffer */
} io_data;

/* A linked list node data structure to maintain application
   information related to a connected socket */
struct node{
	int socket;
	int received_data;
	int pending_data;
	char buf[MAXSIZE];
	char* buf_ptr;
	struct sockaddr_in client_addr;
	struct node *next;
};







