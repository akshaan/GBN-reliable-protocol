#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include <unistd.h>
#include <cmath>

int main(int argc, char* argv[])
{
	/* Defs and Inits */
	struct addrinfo hints;
	struct addrinfo* res, *ptr;
	int err, socketfd, infd, flag = 0, recvlen, numpackets, base, congwin; 
	socklen_t addr_size;
	struct sockaddr_storage in_addr;
	char buff[MAX_PACKET_SIZ];
	FILE* fp;
	long int sz;
	
	
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

	/* Bind to socket */
	if(bind(socketfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
	{
		fprintf(stderr, "Failed to bind to socket\n");
		exit(BIND_ERR);
	}

	freeaddrinfo(res);
	addr_size = sizeof in_addr;

	/* Begin receive-send loop */
	printf("Waiting for file request\n");
	while(1){
		/* Receive request */
		if((recvlen = recvfrom(socketfd,buff,MAX_PACKET_SIZ,0,(struct sockaddr*)&in_addr,\
		&addr_size)) == -1)
		{
			perror("recvfrom");
			exit(1);		

		}
		
		/* Open requested resource and find number of packets required */
		fp = fopen(buff, "rb");
		if(!fp){
			fprintf(stderr, "Failed to open requested file\n");
			exit(1);
		}
		fseek(fp, 0 , SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0 , SEEK_SET);	
		numpackets = ceil((float)sz/MAX_PACKET_SIZ);
					
		/* Set base and congestion window*/
		base = 0;
		congwin = argv[2]; 

		printf("%d\n",numpackets);		
		}

	
		close(socketfd);
		return 0;

}
