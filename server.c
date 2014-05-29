#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include <unistd.h>
#include <cmath>
#include "segment.c"
#include <sys/fcntl.h>

int main(int argc, char* argv[])
{
	/* Defs and Inits */
	struct addrinfo hints;
	struct addrinfo* res, *ptr;
	int err, socketfd, infd, flag = 0, recvlen, numpackets, base = 0, congwin, currack = 0, currseq = 0; 
	socklen_t addr_size;
	struct sockaddr_storage in_addr;
	char buff[MAX_PACKET_SIZ];
	FILE* fp;
	long int sz;
	segment* window;
	
	
	/* Check for required args */
	if(argc < 3){
		printf("Usage: ./server port_number congestion_window_size\n");
		exit(1);
	}

	/* Get addrinfo */
	bzero(&hints, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;

	if(getaddrinfo(NULL,argv[1],&hints,&res) != 0) {
		fprintf(stderr,"Failed to get address info\n");
		exit(GETADDR_ERR);
	}

	ptr = res;
	while(ptr != NULL)
	{
		if((socketfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) != -1){
			flag = 1;
			break;
		}

		else 
			ptr = ptr->ai_next;

	}

	if(flag == 0){
		fprintf(stderr, "Failed to create socket\n");
		exit(SOCK_ERR);
	}

	fcntl(socketfd,F_SETFL, O_NONBLOCK);

	/* Bind to socket */
	if(bind(socketfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
	{
		fprintf(stderr, "Failed to bind to socket\n");
		exit(BIND_ERR);
	}
	
	addr_size = sizeof in_addr;

	/* Begin receive-send loop */
	printf("\nWaiting for file request\n\n");
	while(1){
		/* Receive request */
		segment seg;
		while((recvlen = recvfrom(socketfd,&seg,sizeof(segment),0,(struct sockaddr*)&in_addr,\
		&addr_size)) == -1)
		{		}
		
		/* Get request from received segment */
		segment req;
		memcpy(&req,&seg,sizeof(segment));
		printf("DATA received seq#%d, ACK#%d, FIN %d, content-length: %d\n\n",\
			req.seq_no,req.ack_no,req.fin,req.data_len);
		
		printf("File requested is %s\n\n....................\n\n", req.data);		

		/* Open requested resource and find number of packets required */
		fp = fopen(req.data, "rb");
		if(!fp){
			fprintf(stderr, "Failed to open requested file\n");
			exit(1);
		}
		fseek(fp, 0 , SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0 , SEEK_SET);	
		numpackets = ceil((float)sz/MAX_PACKET_SIZ);
	
		/* Set base and congestion window */
		base = 0;
		congwin = atoi(argv[2]);
		printf("%d",congwin);
		currack = req.data_len;

		
		/* Allocate array of segments and buffer to read in the file */
		window = (segment*)malloc(numpackets * sizeof(segment));
		char* temp = (char*)malloc(sz*sizeof(char));
		if(fread(temp,1,sz,fp) != sz){
			fprintf(stderr,"Error reading file");
			exit(1);

		
		}		
		/* Send packets in sets of size congwin */
	
		int remain = sz;
		while(1){
			
			for(int i = 0; i < congwin; ++i)
			{
				if(remain <= 0)
					break;
			
				segment s;
				printf("%d",i);
				int sendsiz = MAX_PACKET_SIZ;
				if(remain - MAX_PACKET_SIZ <= 0){
					sendsiz = remain;
				}
				build_segment(&s,currseq,currack,sendsiz,temp+(sz-remain),0);

				if(sendto(socketfd,&s,sizeof(segment),0,(struct sockaddr*)&in_addr, addr_size)\
					 ==-1){
					perror("sendto");
					exit(1);
				}
				
				remain -= sendsiz;				
			
			}	

		}
	}	
		close(socketfd);
		return 0;

}
