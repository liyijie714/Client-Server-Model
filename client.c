#include "comp429lib.h"
int main(int argc, char **argv){

	/* socket descriptor */
	int clientfd;

	/* The host name and the message client sending to server.*/
	char *hostname,*send_msg, *rec_msg;

	/* A structure used to specify a time interval.*/
	struct timeval sendTime,recTime;

	/* Port number, times of messages being sent to server, the number of bytes received */
	struct hostent *host_entry;

	/* address structure for identifying the server */
	struct sockaddr_in serveraddr;

	int port,count, byteCnt, sent_bytes, rec_bytes;

	/* Size of message assigned by users */
	unsigned short size;

	/* Pint-pong client program should take 4 mandatory command line parameters. */
	if (argc != 5){
		fprintf(stderr, "usage: %s <hostname> <port>(18000<=port<=18200) <size>(10<=size<=65535), <count>(1<=count<=10000)\n", argv[0]);
		exit(0);
	}

	/* The host where the server is running.*/
	hostname = argv[1];

	/* The port on which the server is running. On CLEAR, the usable range is 18000 <= port <= 18200 */
	port = atoi(argv[2]);
	if (port<18000 || port>18200){
		fprintf(stderr,"invalid port (18000<=port<=18200)");
		exit(0);
	}

	/* The size in bytes of each message to send (10 <= size <= 65,535) */
	size = (unsigned short) atoi(argv[3]);
	if (size<10 || size>65535) {
		fprintf(stderr, "size out of range(10<=size<=65535)");
		exit(0);
	}

	/* The number of message exchanges to perform (1 <= count <= 10,000) */
	count = atoi(argv[4]);
	if (count<1||count>10000) {
		fprintf(stderr, "invalid count (1<=count<=10000)");
		exit(0);
	}
	
	/* building connection with server*/
	clientfd = socket(AF_INET, SOCK_STREAM,0);
	if (clientfd<0) {
		perror("failed creating socket!");
		abort();
	}

	host_entry = gethostbyname(hostname);
	if (host_entry==NULL) {
		perror("failed getting host address!");
		abort();
	}
	memset((char*)&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	memcpy((char*)&serveraddr.sin_addr.s_addr, (char*)host_entry->h_addr_list[0], host_entry->h_length);
	serveraddr.sin_port = htons((unsigned short)port);
	if (connect(clientfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))<0) {
		perror("connected failed!");
		abort();
	}
	
	send_msg = (char*) malloc(size+1);
	rec_msg = (char*) malloc(size+1);
	memset(send_msg, 0,size+1);
	*(unsigned short*)send_msg = htons(size); /*put the size of message in the send_msg buffer*/
	int i=0;
	for (i=0; i<size-10; i++)
	{
		send_msg[i+10] = (char)('a'+i%26);
	}
	send_msg[i+10] = '\0'; 
	double average_sec = 0.0, average_usec = 0.0;
	for(i=0; i<count; i++)
	{
		if (gettimeofday(&sendTime,NULL)==-1)
		{
			fprintf(stderr, "failed when getting time");
			exit(0);
		}
		int localtv_sec = (int)sendTime.tv_sec;
		int localtv_usec = (int)sendTime.tv_usec;
		*(int*)(send_msg+2)=(int)htonl(localtv_sec);
		printf("\nTransaction %d:\n\n",i+1);
		*((int*)(send_msg+6))=(int)htonl(localtv_usec);
		sent_bytes = 0;
		rec_bytes = 0; 
		char *buf_ptr = send_msg;
		char *rec_buf_ptr = rec_msg;
		
		/* sent message to server */
		while (sent_bytes<size){
			byteCnt = send(clientfd, buf_ptr, size-sent_bytes, MSG_DONTWAIT);
			if (byteCnt<0) byteCnt = 0;
			sent_bytes += byteCnt;
			buf_ptr += byteCnt;
			printf("%d bytes are sent this time. In total %d bytes are sent\n", byteCnt, sent_bytes);
		}
		printf("\nIn total %d bytes are sent\n\n", sent_bytes);
		
		/* receive message from server */
		memset(rec_msg, 0, sizeof(size));
		while (rec_bytes<size){
			byteCnt = recv(clientfd, rec_buf_ptr, size-rec_bytes, 0);
			if (byteCnt<0) byteCnt = 0;
			rec_bytes += byteCnt;
			rec_buf_ptr += byteCnt;
			printf("%d bytes are received this time. In total %d bytes are received\n", byteCnt, rec_bytes); 
		}
			
		if (gettimeofday(&recTime,NULL)!=-1)
		{
			average_sec += (double)(recTime.tv_sec-sendTime.tv_sec)/count;
			average_usec += (double)(recTime.tv_usec-sendTime.tv_usec)/count;
		}
		if (strcmp((char*)send_msg, (char*)rec_msg)==0) printf("\nIn total %d bytes are transacted. Transaction completed!\n", rec_bytes);
		else {
			perror("received message doesn't match the sent message!");
			abort();
		} 
	}
	free(send_msg);
	free(rec_msg);
	printf("\naverage latency is %.3f milisecond\n", average_sec*1000.0+average_usec/1000.0);
	exit(0);
}


