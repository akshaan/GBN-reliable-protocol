#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include <unistd.h>

int main(int argc, char* argv[])
{
	struct addrinfo hints;
	struct addrinfo* res, *ptr;
	int err, socketfd, infd, flag = 0, yes = 1;
	socklen_t addr_size;
	struct sockaddr_storage in_addr;
	char buff[REQ_BUF_SIZ];
	int recvlen;

	if(argc < 2){
		printf("Usage: ./server port_number\n");
		exit(1);
	}


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


	if(bind(socketfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
	{
		fprintf(stderr, "Failed to bind to socket\n");
		exit(BIND_ERR);
	}

	freeaddrinfo(res);

	addr_size = sizeof in_addr;

		printf("Waiting for file request\n");

		if((recvlen = recvfrom(socketfd,buff,REQ_BUF_SIZ-1,0,(struct sockaddr*)&in_addr, &addr_size)) == -1){
			perror("recvfrom");
			exit(1);		

		}


		else printf("%s\n",buff);		

		close(socketfd);
		return 0;

}
