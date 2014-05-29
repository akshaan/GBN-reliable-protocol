#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include <unistd.h>
#include "segment.c"

int main(int argc, char* argv[])
{
	struct addrinfo hints;
	struct addrinfo* res, *ptr;
	int err, socketfd, infd, flag = 0, yes = 1;
	socklen_t addr_size;
	struct sockaddr_storage in_addr;
	int sendlen;

	if(argc < 4){
		printf("Usage: ./client server_hostname server_port_number filename\n");
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





	segment req;
	build_segment(&req,0,0,strlen(argv[3]),argv[3],0);
	printf("Sending request to server.\n");		
	if((sendlen = sendto(socketfd,&req,sizeof(segment),0,ptr->ai_addr, ptr->ai_addrlen)) == -1){
			perror("sendto");
			exit(1);		

	}

	addr_size = sizeof in_addr;	
	while(1){
		segment rsp;
		if((recvfrom(socketfd,&rsp,sizeof(segment),0,(struct sockaddr*)&in_addr,&addr_size)) == -1){
			perror("recvfrom");
			exit(1);
		}

		printf("DATA received seq#%d, ACK#%d, FIN %d, content-length: %d\n",\
			rsp.seq_no,req.ack_no,rsp.fin,rsp.data_len);			

	}
		
	freeaddrinfo(res);
	close(socketfd);
	return(0);

}
