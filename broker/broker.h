#ifndef BROKER_H
#define BROKER_H

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/un.h>
#include<errno.h>
#include<pthread.h>


#define SERVER_PORT 			1024
#define FIXED_HEADER_SIZE 		5
#define REMAINING_LENGTH_SIZE 	4
#define TOPIC_NAME_LENGTH_SIZE 	2
#define MESSAGE_ID_SIZE 		2
#define CLIENT_ID_SIZE 			23

#define set_msg_type(x)  (x) << 4 
#define set_dup_flag(x)  (x) << 3
#define set_qos_level(x) (x) << 1
#define set_retain(x) 	 (x) 

#define get_msg_type(x)  (x) >> 4 &0b1111
#define get_dup_flag(x)  (x) >> 3 &0b1
#define get_qos_level(x) (x) >> 1 &0b11
#define	get_retain(x)	 (x) & 0b1

void setRemainingLen(char *pkt, int remainLen){
	*((int32_t *)(pkt + 1)) = remainLen;
}

int getRemainingLen(char *pkt){
	return *((int32_t *)(pkt + 1));
}

void setFixedHeader(char *pkt, int msgType, int dupFlag, int qosLevel, int retain, int remainLen){
	pkt[0] = set_msg_type(msgType) | set_dup_flag(dupFlag) | set_qos_level(qosLevel) | set_retain(retain);
	setRemainingLen(pkt, remainLen);
}

int setMsgID(char *pkt, int idx, int msgID){
	*((int16_t *)(pkt + idx)) = msgID;
	return idx + MESSAGE_ID_SIZE;
}

int getMsgID(char *pkt, int idx){
	return *((int16_t *)(pkt+idx));
}

// received from subscriber 
void connect(char *pkt){

}

//response to subscriber
int connack(char *pkt, int code){
	int idx = FIXED_HEADER_SIZE;
	pkt[idx++] = code;
	setFixedHeader(pkt, 2, 0, 0, 0, idx - FIXED_HEADER_SIZE);
	return idx;
}

/*publish pkt from sensor to broker
* or broker to subscriber
*/
int recvPublish(char *pkt){

}

int makePublish(char *pkt){

}

/*
*/
int makePuback(char *pkt, int msgID){
	int idx = FIXED_HEADER_SIZE;
	idx = setMsgID(pkt, idx, msgID);
	setFixedHeader(pkt, 4, 0, 1, 0, idx - FIXED_HEADER_SIZE);
	return idx;	
}

int makePubrec(char *pkt, int msgID){
	int idx = FIXED_HEADER_SIZE;
	idx = setMsgID(pkt, idx, msgID);
	setFixedHeader(pkt, 5, 0, 2, 0, idx - FIXED_HEADER_SIZE);
	return idx;
}

int makePubrel(char *pkt, int msgID){
	int idx = FIXED_HEADER_SIZE;
	idx = setMsgID(pkt, idx, msgID);
	setFixedHeader(pkt, 7, 0, 0, 0, idx - FIXED_HEADER_SIZE);
	return idx;
}


int recvSubscribe(char *pkt){

}

int makeSuback(char *pkt){

}

int passiveTCP(){
	struct sockaddr_in addr;
	int sock;
	if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(1);
	}
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(SERVER_PORT);

	if( bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("bind");
		exit(1);
	}
	if( listen(sock, LISTEN_MAX) < 0){
		perror("listen");
		exit(1);
	}
	printf("[*] Liten at port :%d\n", SERVER_PORT);
	return sock;
}

#endif