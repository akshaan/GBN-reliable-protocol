#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include <unistd.h>
#include <sys/fcntl.h>
#include "segment.c"
#include <vector>
#include <time.h>


using namespace std;

bool isCorrupt(float prob)
{
        srand(time(NULL));
        float random = (float)(rand())/(float)(RAND_MAX);

        if(random < prob){
                return true;
	}
        else return false;
}


bool isLost(float prob)
{
        srand(time(NULL));
        float random = (float)(rand())/(float)(RAND_MAX);

        if(random < prob){	
                return true;
	}
        else return false;
}



int main(int argc, char* argv[])
{
	struct addrinfo hints;
	struct addrinfo* res, *ptr;
	int err, socketfd, infd, flag = 0, yes = 1;
	socklen_t addr_size;
	struct sockaddr_storage in_addr;
	int sendlen, currseq = 0;
	vector<segment> packets;
	int expseq = 0;
	float prob_corr;
	float prob_loss;

	if(argc < 6){
		printf("Usage: ./client server_hostname server_port_number filename prob_loss prob_corruption\n");
		exit(1);
	}


	prob_loss = atof(argv[4]);
	prob_corr = atof(argv[5]);
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

	fcntl(socketfd,F_SETFL,O_NONBLOCK);



	segment req;
	build_segment(&req,0,0,strlen(argv[3]),argv[3],0);
	printf("Sending request to server.\n\n");		
	if((sendlen = sendto(socketfd,&req,sizeof(segment),0,ptr->ai_addr, ptr->ai_addrlen)) == -1){
			perror("sendto");
			exit(1);		

	}

	addr_size = sizeof in_addr;	
	segment rsp;
	while(1){
	
		if((recvfrom(socketfd,&rsp,sizeof(segment),0,(struct sockaddr*)&in_addr,&addr_size)) != -1)
		{		
			segment ack;
			if(rsp.seq_no == expseq && !isCorrupt(prob_corr) && !isLost(prob_loss)){
				printf("DATA received seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
					rsp.seq_no,rsp.ack_no,rsp.fin,rsp.data_len);
			
				currseq = rsp.ack_no +1;
				packets.push_back(rsp);
				if(rsp.fin == 1)
					break;
				build_segment(&ack,currseq,rsp.seq_no+rsp.data_len,0,NULL,0);
				expseq = rsp.seq_no + rsp.data_len;	
			}


			else{
				if(rsp.fin == 1)
					break;
			
	printf("POOP received seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
		rsp.seq_no,rsp.ack_no,rsp.fin,rsp.data_len);

			
				printf("Packet lost or corrupted\n\n");
				build_segment(&ack,currseq,expseq,0,NULL,0);
			}
				
		/* Send ACK for received data packets */			
		
	
			if(sendto(socketfd,&ack,sizeof(segment),0,(struct sockaddr*)&in_addr,addr_size) == -1)
			{
				perror("sendto");
				exit(1);
			}

			printf("ACK sent seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",
					 	ack.seq_no, ack.ack_no, ack.fin, ack.data_len);
		}

	}

	/* Close connection */
		segment finack;
		build_segment(&finack,rsp.ack_no,rsp.seq_no+1,0,NULL,1);
		if(sendto(socketfd,&finack,sizeof(segment),0,(struct sockaddr*)&in_addr,addr_size) == -1){
			perror("sendto");
			exit(1);
		}

		printf("FINACK sent seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",
		              finack.seq_no, finack.ack_no, finack.fin, finack.data_len);


		segment finack2;
		while(recvfrom(socketfd,&finack2,sizeof(segment),0,(struct sockaddr*)&in_addr,&addr_size) == -1)
		{		}

		printf("FINACK received seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",    	
                      	finack2.seq_no, finack2.ack_no, finack2.fin, finack2.data_len);

		printf("Closed connection\n\n");

		/* Write packets to file */
		FILE* fp = fopen("test.txt","w");
		for(int j = 0; j < packets.size(); ++j)
		{
			for(int i = 0; i < packets[j].data_len; i++)
			{
				fputc(packets[j].data[i],fp);
			}
		}
		fclose(fp);
	
	freeaddrinfo(res);
	close(socketfd);
	return(0);

}
