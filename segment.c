#include "constants.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int seq_no;
	int ack_no;
	int data_len;
	char data[MAX_PACKET_SIZ];
	int fin;
} segment;

void build_segment(segment* seg,int s, int a, int len, char* dat, int f)
{
	seg->seq_no = s;
	seg->ack_no = a;
	seg->data_len = len;

	if(dat)
		memcpy(&seg->data, dat, sizeof(char)*MAX_PACKET_SIZ);
	
	seg->fin = f;
}
	
