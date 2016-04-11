#include "comp429lib.h"

/* initialize an internal IO buffer with a descriptor fd. */
void init_read_buffer(io_data *data, int fd);

/* read data from the descriptor fd to corresponding buffer. */
ssize_t io_read(io_data *data, char *usrbuf, size_t n);

/* read one line to usrbuf */
ssize_t read_one_line(io_data *data, void *usrbuf, size_t maxlen);

/* write at most n bytes from usrbuf to descriptor fd*/ 
ssize_t io_write(int fd, char* usrbuf, size_t n);

/* Server read data from pingpong client */
void read_from_client(struct node *client, struct node* head);

/* Server write data to pingpong client. */
void write_to_client(struct node *client, struct node* head);

/* */
void www_serve(int fd, char* myrootdir);
void parse_uri(char *uri, char* filename, char* myrootdir);
void error_handler(int fd, char* errnum, char* shortmsg, char* longmsg);


/* remove the data structure associated with a connected socket
   used when tearing down the connection */
void dump(struct node *head, int socket);

/* create the data structure associated with a connected socket */
void add(struct node *head, int socket, struct sockaddr_in addr);


int main(int argc, char** argv)
{
    int listenfd,connfd,max;
    int optval = 1;
    struct sockaddr_in sin,addr;
    unsigned short server_port;
    fd_set read_set, write_set;
    struct timeval time_out;
    int select_retval;
    int byteCnt;
    struct node head;
    struct node *current, *next;
    int BACKLOG = 5, mode;
    char myrootdir[MAXLINE];

    head.socket = -1;
    head.next = 0;
    server_port = atoi(argv[1]);
    if (server_port<18000 || server_port>18200) {
        perror("Invalid port! port shour be from 18000 to 18200"); abort();
    } 
    socklen_t addr_len = sizeof(struct sockaddr_in);
    
    if (argv[2]==NULL) mode = DEFAULT;
    else if (strcmp(argv[2],"www")==0) {
        if (argv[3] == NULL) {
            perror("Please specify root directory!"); abort();
        }
        memcpy(myrootdir,argv[3], strlen(argv[3]));
        mode = WWW;
    }
    else {
        perror("Wrong mode!");abort();
    }
    
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0){
        perror("opening TCP socket"); abort();
    }
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))<0){
        perror("setting TCP socket option"); abort();
    }
    
    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(server_port);

    if (bind(listenfd, (struct sockaddr *)&sin, addr_len)<0){
        perror("binding socket to address"); abort();
    }

    if (listen(listenfd, BACKLOG)<0){
        perror("listen on socket failed"); abort();
    }

    while(1){
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);

        FD_SET(listenfd, &read_set);
        max = listenfd;

        for (current=head.next; current!=NULL; current = current->next){
            FD_SET(current->socket,&read_set);
            if (current->pending_data>0 && current->buf!=NULL) {
                FD_SET(current->socket, &write_set);
            }
            if (current->socket > max) max = current->socket;
        }
        time_out.tv_usec = 1000000;
        time_out.tv_sec = 0;

        select_retval = select(max+1,&read_set,&write_set,NULL,&time_out);
        if (select_retval<0){
            perror("select failed"); abort();
        }
        if (select_retval == 0) continue;
        if (select_retval>0){
            if (FD_ISSET(listenfd, &read_set)) {
                connfd = accept(listenfd, (struct sockaddr*) &addr, &addr_len);
                if (connfd<0) {
                    perror("error accepting connection");
                    abort();
                }
                if (fcntl(connfd, F_SETFL, O_NONBLOCK)<0){
                    perror("making socket non-blocking");
                    abort();
                }
                printf("\nAccepted connection. Client IP address is: %s\n", inet_ntoa(addr.sin_addr));
                add(&head,connfd,addr);
            }
            for (current = head.next; current; current = next){
                next = current->next;
                if (FD_ISSET(current->socket, &write_set) && mode == DEFAULT){
                    write_to_client(current, &head);
                }
                if (FD_ISSET(current->socket, &read_set) && mode==DEFAULT){
                    read_from_client(current,&head);
                }
                else if (FD_ISSET(current->socket, &read_set) && mode==WWW){                                     
                    www_serve(current->socket, myrootdir);
                    close(current->socket);
                    dump(&head, current->socket);
                }
            }
        }
    }
}

void write_to_client(struct node* client, struct node* head){
    int sent_byteCnt;
    sent_byteCnt = send(client->socket, client->buf_ptr, client->pending_data, MSG_DONTWAIT);
    client->pending_data -= sent_byteCnt;
    client->buf_ptr += sent_byteCnt;
    printf("\n%d bytes are sent back to client this time\n", sent_byteCnt);
    if (client->pending_data<=0){
        memset(client->buf, 0, sizeof(client->buf));
        client->buf_ptr = NULL;
    } 
}
void read_from_client(struct node* client, struct node* head){
    int rec_byteCnt;
    if (client->buf_ptr==NULL) {
        client->buf_ptr = client->buf;
        client->received_data = 0;
        client->pending_data = 0;
    }
    rec_byteCnt = recv(client->socket,client->buf_ptr,MAXSIZE,0);
    if (rec_byteCnt<=0){
        printf("\nClient closed connection. Client IP address is: %s\n\n",
        inet_ntoa(client->client_addr.sin_addr));
        close(client->socket);
        dump(head, client->socket);
        return;
    }
    client->buf_ptr += rec_byteCnt;
    client->received_data += rec_byteCnt;
    printf("\n%d bytes are received this time. In total %d bytes are received now", rec_byteCnt,client->received_data);
    if (client->received_data >= ntohs(*(unsigned short*)client->buf)){
        printf("\nWhole message is received: in total %d bytes\n", client->received_data);
        client->pending_data = client->received_data;
        client->buf_ptr = client->buf;
    } 
}

void www_serve(int fd, char* myrootdir){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], filetype[MAXLINE];
    FILE* req_file;
    io_data io_buf;
    init_read_buffer(&io_buf,fd);
    read_one_line(&io_buf, buf, MAXLINE);
    struct stat file_status;
    sscanf(buf,"%s %s %s", method, uri, version);
    /*error handling. This program only handle GET method*/
    if (strcasecmp(method, "GET")){
        error_handler(fd, "501", "Not Implemented", "www_server only implements GET method");
        return;
    }
    /*reads the request headers*/
    read_one_line(&io_buf,buf,MAXLINE);
    while (strcmp(buf,"\r\n")){
        read_one_line(&io_buf,buf,MAXLINE);
    }
    /*parse URI from GET request*/
    parse_uri(uri, filename, myrootdir);
    if (strstr(uri,"../")!=NULL){
        error_handler(fd, "400", "Bad Request", "The uri should not contain ../");
        return;
    }
    if (stat(filename,&file_status)<0){
        sprintf(buf, "www_server couldn't find the file %s",filename);
        error_handler(fd, "404", "Not found", buf);
        return;
    }
    
    /* Get the type of requested file, here we only handle text/html and treat all other files as text/plain*/
    if (strstr(filename, ".html")) strcpy(filetype, "text/html");
    else if (strstr(filename, ".png")) strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg")) strcpy(filetype, "image/jpeg");
    else strcpy(filetype, "text/plain");
    /* Build and send response headers to client */
    memset(buf,0,sizeof(buf));
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sServer: www_server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, (int)file_status.st_size);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    send(fd, buf, strlen(buf),MSG_DONTWAIT);
    
    /* Send response body to client */
    memset(buf,0, sizeof(buf));
    req_file = fopen(filename, "r");
    while (!feof(req_file)){
        fread(buf, sizeof(char), MAXLINE, req_file);
        if (io_write(fd,buf,strlen(buf))<0) {
            error_handler(fd, "500", "Internal Server Error", "Server could not send data to client");
            return;
        }       
    }
    fclose(req_file);    
}

void error_handler(int fd, char* errnum, char* shortmsg, char* longmsg){
    char header_buf[MAXLINE], error_body[MAXBUF];
    
    /*Build error response body*/
    sprintf(error_body, "%s %s\r\n%s\r\n\r\n", errnum, shortmsg, longmsg);
    
    /*Send error response headers to client*/
    sprintf(header_buf, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    sprintf(header_buf, "%sContent-type: text/plain\r\n", header_buf);
    sprintf(header_buf, "%sContent-length: %d\r\n\r\n", header_buf, (int)strlen(error_body));
    send(fd, header_buf, strlen(header_buf), MSG_DONTWAIT);
    send(fd, error_body, strlen(error_body), MSG_DONTWAIT);
}
void parse_uri(char* uri, char* filename, char* myrootdir){
    char *ptr;  
    strcpy(filename, myrootdir);
    if (uri[0] != '/') strcat(filename,"/");
    strcat(filename, uri);
    if (uri[strlen(uri)-1]=='/') strcat(filename,"home.html");
    return;
}


void dump(struct node *head, int socket){
    struct node *current, *temp;
    current = head;
    while (current->next){
        if (current->next->socket==socket){
            temp = current->next;
            current->next = temp->next;
            free(temp);
            return;
        }
        current = current->next;
    }
}

void add(struct node *head, int socket, struct sockaddr_in addr){
    struct node *new_node;
    new_node = (struct node*)malloc(sizeof(struct node));
    new_node->socket = socket;
    new_node->client_addr = addr;
    new_node->next = head->next;
    head->next = new_node;
}


void init_read_buffer(io_data *data, int fd){ 
    data->descriptor = fd;
    data->unread_st = 0;
    data->unread_st_buf = data->read_buf;
}

ssize_t io_read(io_data *data, char *usrbuf, size_t n){
    int cnt;
    while (data->unread_st<=0){
        data->unread_st = read(data->descriptor, data->read_buf, sizeof(data->read_buf));
        if (data->unread_st<0) return -1;
        else if (data->unread_st==0) return 0;
        else data->unread_st_buf = data->read_buf;
    }
    cnt = n;
    if (data->unread_st<n) cnt = data->unread_st;
    memcpy(usrbuf, data->unread_st_buf, cnt);
    data->unread_st_buf += cnt;
    data->unread_st -= cnt;
    return cnt;
}

ssize_t read_one_line(io_data *data, void *usrbuf, size_t maxlen){
    int i, read_cnt;
    char c, *bufp = usrbuf;
    for (i=1; i<maxlen; i++){
        if ((read_cnt=io_read(data,&c,1))==1){
            *bufp++ = c;
            /* End of the line. */
            if (c=='\n') break;
        }
        else if (read_cnt==0) {
            /* End of the file. */
            if (i==1) return 0;
            else break;
        }
        /* Get error or interrupted. */
        else return -1;
    }
    *bufp = '\0';
    return i;
}

ssize_t io_write(int fd, char* usrbuf, size_t n){
    int left_cnt = n;
    int byte_cnt = 0;
    char *buf_ptr = usrbuf; 
    while (left_cnt>0){
        byte_cnt = write(fd, usrbuf, left_cnt);
        if (byte_cnt<=0) return -1;
        left_cnt -= byte_cnt;
        buf_ptr += byte_cnt;
    }
    return n;
}
