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
#include <time.h>

using namespace std;

bool isCorrupt(float prob)
{
	srand(time(NULL));
	float random = (float)(rand())/(float)(RAND_MAX); 

	if(random < prob)
		return true;
	else return false;
}

bool isLost(float prob)
{
	srand(time(NULL));
	float random = (float)(rand())/(float)(RAND_MAX); 

	if(random < prob)
		return true;
	else return false;
}


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
	vector<segment> packets;
	float prob_corr,prob_loss;	
	int lastack = 0;
	
	/* Check for required args */
	if(argc < 5){
		printf("Usage: ./server port_number congestion_window_size prob_loss prob_corruption\n");
		exit(1);
	}

	/* Initialize corruption and loss probabilities */
	prob_corr = atof(argv[4]);
	prob_loss = atof(argv[3]);

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
		currack = req.data_len+1;	
	
		/* Allocate buffer to read in the file */
		char* temp = (char*)malloc(sz*sizeof(char));
		if(fread(temp,1,sz,fp) != sz){
			fprintf(stderr,"Error reading file");
			exit(1);

		
		}		
		
		int remain = sz;
		while(remain > 0)
		{
			segment a;
			int sendsiz =MAX_PACKET_SIZ;
			if(remain - sendsiz < 0)
				sendsiz = remain;
			build_segment(&a,sz-remain,0,sendsiz,temp+(sz-remain),0);
			packets.push_back(a);
			remain -= sendsiz;
		}

		/* Send first window of packets*/
			for(int i = 0; i < min(congwin,(int)packets.size()); ++i)
			{	
						
				if(sendto(socketfd,&packets[i],sizeof(segment),0,(struct sockaddr*)&in_addr, addr_size)\
					 ==-1){
					perror("sendto");
					exit(1);
				}
			 		
				
				currseq += packets[i].data_len;				
				printf("DATA sent seq# %d, ACK# %d, FIN# %d, content-length: %d\n\n"\
					,packets[i].seq_no,packets[i].ack_no,packets[i].fin,packets[i].data_len);
			
			}	
			

		segment ack;
		while(1){	
			
		   fd_set readfds;
		   struct timeval timeout;
		   timeout.tv_sec = 5;
		   timeout.tv_usec = 0;
		   FD_ZERO(&readfds);
		   FD_SET(socketfd, &readfds);
		   int success = 0;
		   while(1)
		   {  

		      timeout.tv_sec = 5;
	              if(select(socketfd+1, &readfds,NULL,NULL, &timeout) == 0)
		      {
			 printf("(ACK lost or corrupted) Timeout\n\n");
		         for(int i = base; i < min(base+congwin,(int)packets.size()); ++i)
			 {
					sendto(socketfd,&packets[i],sizeof(segment),0,(struct sockaddr*)\
									&in_addr,addr_size);


					
					printf("DATA sent seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
							packets[i].seq_no,packets[i].ack_no,packets[i].fin,\
											packets[i].data_len);
							
			 }


		//	timeout.tv_sec = 5;
		//	timeout.tv_usec = 0;
			break;

		     }
	

		     else {
			
			 recvfrom(socketfd,&ack,sizeof(segment),0,(struct sockaddr*)&in_addr,&addr_size);
			 
			    
			    if(ack.ack_no > lastack && !isLost(prob_loss) && !isCorrupt(prob_corr)){
				   
				   lastack = ack.ack_no;
				   printf("ACK received seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
					 	ack.seq_no,ack.ack_no, ack.fin, ack.data_len);
				
				
				
				/* Slide window forward and send next packet */
		
				   base = ceil((float)ack.ack_no/MAX_PACKET_SIZ) - 1;
				   currack++;
				   printf("Sliding window\n\n");
		
				   if(!(base+congwin >= packets.size()))
				   {	
					segment next = packets[base+congwin];
					sendto(socketfd,&next,sizeof(segment),0,(struct sockaddr*)\
							&in_addr,addr_size);	

					printf("DATA sent seq# %d, ACK# %d, FIN %d, content-length:%d\n\n",\
 									next.seq_no, next.ack_no, next.fin,\
										 next.data_len);
				
					currseq += next.data_len;	
				   
			//		timeout.tv_sec = 5;	
					break;
								
				  }			
					
			     }

			
			else {
				//printf("ACK received seq# %d, ACK# %d, FIN %d, content-length: %d\n\n",\
					ack.seq_no,ack.ack_no, ack.fin, ack.data_len);
				FD_ZERO(&readfds);
				FD_SET(socketfd, &readfds);
				
			
			}
			
	
		
}}
			if(base == numpackets -1) break;
}		

	
	
	close:
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

		close(socketfd);
		return 0;

}
