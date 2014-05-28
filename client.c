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
	char buff[6] = "Hello";
	int sendlen;

	if(argc < 3){
		printf("Usage: ./client server_hostname server_port_number\n");
		exit(1);
	}


	bzero(&hints, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_UNSPEC;

	if(getaddrinfo(argv[1],argv[2],&hints,&res) != 0) {
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




		
		
	



		printf("Sending file request\n");		
		
		if((sendlen = sendto(socketfd,buff,5,0,ptr->ai_addr, ptr->ai_addrlen)) == -1){
			perror("sendto");
			exit(1);		

		}

		

		freeaddrinfo(res);
		close(socketfd);
		return(0);

}
