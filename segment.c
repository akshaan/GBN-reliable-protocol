#include "constants.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int seq_no;
	int ack_no;
	int data_len;
	char data[MAX_PACKET_SIZ];
	char fin;
} segment;
