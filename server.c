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
#include <vector>

using namespace std;

int main(int argc, char* argv[])
{
	/* Defs and Inits */
	struct addrinfo hints;
	struct addrinfo* res, *ptr;
	int err, socketfd, flag = 0, recvlen, numpackets, base = 0, congwin, currack = 0, currseq = 0; 
	socklen_t addr_size;
	struct sockaddr_storage in_addr;
	char buff[MAX_PACKET_SIZ];
	FILE* fp;
	long int sz;
	vector<segment> window;
	
	
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
	
	while(1){

		printf("\nWaiting for file request\n\n");
		/* Receive request */
		segment seg;
		while((recvlen = recvfrom(socketfd,&seg,sizeof(segment),0,(struct sockaddr*)&in_addr,\
		&addr_size)) == -1)
		{		}
		
		/* Get request from received segment */
		segment req;
		memcpy(&req,&seg,sizeof(segment));
		printf("DATA received seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
			req.seq_no,req.ack_no,req.fin,req.data_len);
		
//		printf("File requested is %s\n\n....................\n\n", req.data);		

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
		currack = req.data_len+1;	
	
		/* Allocate buffer to read in the file */
		char* temp = (char*)malloc(sz*sizeof(char));
		if(fread(temp,1,sz,fp) != sz){
			fprintf(stderr,"Error reading file");
			exit(1);

		
		}		
		/* Send packets in sets of size congwin */
	
		int remain = sz;
		while(1){



			window.clear();			
			for(int i = 0; i < congwin; ++i)
			{
				if(remain <= 0) break;
					
				segment s;
				int sendsiz = MAX_PACKET_SIZ;
				if(remain - MAX_PACKET_SIZ <= 0){
					sendsiz = remain;
				}
				build_segment(&s,currseq,currack,sendsiz,temp+(sz-remain),0);
				
			//	window.push_back(s);			
				if(sendto(socketfd,&s,sizeof(segment),0,(struct sockaddr*)&in_addr, addr_size)\
					 ==-1){
					perror("sendto");
					exit(1);
				}
			 		
				remain -= sendsiz;
				currseq += sendsiz;				
				printf("DATA sent seq# %d, ACK# %d, FIN# %d, content-length: %d\n\n"\
					,s.seq_no,s.ack_no,s.fin,s.data_len);
			
			}	

			segment ack;
			if(recvfrom(socketfd,&ack,sizeof(segment),0,(struct sockaddr*)&in_addr,&addr_size) != -1)
			{	
				
				printf("ACK received seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
					 	ack.seq_no,ack.ack_no, ack.fin, ack.data_len);
				

				/* Simulate corrupted or lost ACK */

				/* Slide window forward */
					if(ceil((float)ack.ack_no/MAX_PACKET_SIZ)-1  == base){
						base++;
						currack++;
						printf("Sliding window\n\n");
					}

					if(base == numpackets){
						break;
					}
			}

						

		/* After every window sent, wait for timeout */

		}

		printf("File transfer complete\n\n");

		/* Close connection */
		segment fin;
		build_segment(&fin,currseq,currack,0,NULL,1);
		if(sendto(socketfd,&fin,sizeof(segment),0,(struct sockaddr*)&in_addr,addr_size) == -1){
			perror("sendto");
			exit(1);
		}
		printf("DATA sent seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
				 fin.seq_no,fin.ack_no,fin.fin,fin.data_len);	

		segment finack;
		while(recvfrom(socketfd,&finack,sizeof(segment),0,(struct sockaddr*)&in_addr,&addr_size) == -1)
		{		}
		
		printf("FINACK received seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
		        		 finack.seq_no,finack.ack_no,finack.fin,finack.data_len);	

		segment finack2;
		build_segment(&finack2,finack.ack_no,finack.seq_no+1,0,NULL,1);
		if(sendto(socketfd,&fin,sizeof(segment),0,(struct sockaddr*)&in_addr,addr_size) == -1){
			perror("sendto");
			exit(1);
		}	
		
		printf("FINACK sent seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
			        	finack2.seq_no,finack2.ack_no,finack2.fin,finack2.data_len);

		printf("Closed connection\n\n");		
	}	
		close(socketfd);
		return 0;

}
