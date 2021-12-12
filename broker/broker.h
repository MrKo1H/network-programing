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


#define BUFFER_SIZE 			1024
#define SERVER_PORT 			1608
#define LISTEN_MAX				100
#define FIXED_HEADER_SIZE 		5
#define REMAINING_LENGTH_SIZE 	4
#define TOPIC_NAME_LENGTH_SIZE 	2
#define MESSAGE_ID_SIZE 		2
#define CLIENT_ID_SIZE 			23
#define CONNECT_FLAG_SIZE 		1
#define SUBSCRIBER_MAX 			200
#define LOCATION_LEN			20
#define SERVICE_LEN			 	20
#define SUBSCRIBE_MAX 			20
#define USERNAME_SIZE			20
#define	PASSWORD_SIZE 			20
#define PAYLOAD_LENGTH_SIZE		2


#define set_msg_type(x)  (x) << 4 
#define set_dup_flag(x)  (x) << 3
#define set_qos_level(x) (x) << 1
#define set_retain(x) 	 (x) 

#define get_msg_type(x)  (x) >> 4 &0b1111
#define get_dup_flag(x)  (x) >> 3 &0b1
#define get_qos_level(x) (x) >> 1 &0b11
#define	get_retain(x)	 (x) & 0b1

#define get_usr_flag(x)  (x) >> 7
#define get_pass_flag(x) (x) >> 6 &0b1
#define get_will_retain(x) (x) >> 5 &0b1
#define get_will_qos(x)   (x) >> 3 &0b11
#define get_will_flag(x)  (x) >> 2 &0b1 
#define get_clean_session(x) (x) >> 1 & 0b1

typedef struct _topic{
	char t_loc[LOCATION_LEN];  //location
	char t_ser[SERVICE_LEN]; //service
} topic;

struct sub{ /// subscriber 
	char *s_cli_id;
	char *s_usr_name;
	char *s_password;
	int s_cnt; // number of topic subscribed
	topic *s_top[SUBSCRIBE_MAX];
};
struct pub{
	topic *p_top;
	char *p_payload;
};

enum CONNACK_CODE{
	ACCECPT,
	UNACCECPTABLE, 
	IDENTIFIER_REJECTED, 
	SERVER_UNAVAILABLE,
	BAD_USER_NAME_OR_PASSWORD,
	NOT_AUTHORIZED
};

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
char * getClientID(char *pkt, int idx){
	char * s = malloc(CLIENT_ID_SIZE);

	for( int i = 0; i < CLIENT_ID_SIZE; i++)
		s[i] = pkt[idx++];
	return s;
}


void viewFixedHeader(char *pkt){
	printf("Message Type:%d\n", get_msg_type(pkt[0]));
	printf("DUP Flag:%d\n", get_dup_flag(pkt[0]));
	printf("QoS level:%d\n", get_qos_level(pkt[0]));
	printf("Retain:%d\n", get_retain(pkt[0]));
	printf("Remaing Length:%d\n", getRemainingLen(pkt));
}
char *getUsername(char *pkt, int idx){
	char *ptr;
	ptr = malloc(USERNAME_SIZE);
	for(int i = 0; i < USERNAME_SIZE; i++)
		ptr[i] = pkt[idx++];
	return ptr;
}

char *getPassword(char *pkt, int idx){
	char *ptr;
	ptr = malloc(PASSWORD_SIZE);
	for(int i = 0; i < PASSWORD_SIZE; i++)
		ptr[i] = pkt[idx++];
	return ptr;
}

// received from subscriber 
void recvConnect(char *pkt, struct sub *x){
	int idx;

	idx = FIXED_HEADER_SIZE + CONNECT_FLAG_SIZE;
	x->s_cli_id = getClientID(pkt, idx);
	idx += CLIENT_ID_SIZE;
	if(get_usr_flag(pkt[FIXED_HEADER_SIZE])){
		x->s_usr_name = getUsername(pkt, idx);
		idx += USERNAME_SIZE;
	}
	if(get_pass_flag(pkt[FIXED_HEADER_SIZE])){
		x->s_password = getPassword(pkt, idx);
		idx += PASSWORD_SIZE;
	}
}

void viewConnect(char *pkt){
	 viewFixedHeader(pkt);	
	 printf("user name flag:%d password flag: %d will retain: %d will QoS: %d will flag: %d clean session: %d\n", get_usr_flag(pkt[0]), get_pass_flag(pkt[0]), get_will_retain(pkt[0]), get_will_qos(pkt[0]), get_will_flag(pkt[0]), get_clean_session(pkt[0]));
	 printf("Client ID: %s\n", getClientID(pkt, FIXED_HEADER_SIZE + CONNECT_FLAG_SIZE));
}

//response to subscriber
int makeConnack(char *pkt, int code){
	int idx = FIXED_HEADER_SIZE;
	pkt[idx++] = code;
	setFixedHeader(pkt, 2, 0, 0, 0, idx - FIXED_HEADER_SIZE);
	return idx;
}

int getTopicLen(char *pkt, int idx){
	return *((int16_t*)(pkt + idx));
}

topic *getTopic(char *pkt, int idx){

	topic *x;
	int pos;
	
	pos = getTopicLen(pkt, idx);
	idx += TOPIC_NAME_LENGTH_SIZE;
	pos += idx;
	x = (topic *)malloc(sizeof(topic));
	int i;

	for(i = 0; pkt[++idx] != '/'; i++)
		x->t_loc[i] = pkt[idx];
	x->t_loc[i] = '\0';

	for(i = 0; ++idx < pos && pkt[idx] != '/'; i++)
		x->t_ser[i] = pkt[idx];
	x->t_ser[i] = '\0';

	return x;
}

int getPayloadLen(char *pkt, int idx){
	return *((int16_t*)(pkt+idx));
}

char *getPayload(char *pkt, int idx){
	char *x;
	int len;

	len = getPayloadLen(pkt,idx);
	idx += PAYLOAD_LENGTH_SIZE;
	x = malloc(len);

	for(int i = 0; i < len; i++)
		x[i] = pkt[idx++];
	return x;
}

/*publish pkt from sensor to broker
* or broker to subscriber
*/
void recvPublish(char *pkt, struct pub * pub_pkt){
	int paylen, relen, toplen;
	int idx;
	
	idx =FIXED_HEADER_SIZE + MESSAGE_ID_SIZE;
	toplen = getTopicLen(pkt, idx);
	pub_pkt->p_top= getTopic(pkt, idx);

	idx += PAYLOAD_LENGTH_SIZE + toplen;
	pub_pkt->p_payload = getPayload(pkt, idx);
}

int setTopicLen(char *pkt, int idx, int len){
	*((int16_t*)(pkt + idx)) = len;
	return idx + TOPIC_NAME_LENGTH_SIZE;
}

int setTopic(char *pkt, int idx, topic *x){
	int len;

	len = 1 + strlen(x->t_loc) + 1 + strlen(x->t_ser);
	idx = setTopicLen(pkt, idx, len);
	pkt[idx++] = '/';
	for(int i = 0; x->t_loc[i] != '\0'; i++)
		pkt[idx++] = x->t_loc[i];
	pkt[idx++] = '/';
	for(int i = 0; x->t_ser[i] != '\0'; i++)
		pkt[idx++] = x->t_ser[i];
	return idx;
}

int setPayloadLen(char *pkt, int idx, int len){
	*((int16_t *)(pkt + idx)) = len;
	return idx + PAYLOAD_LENGTH_SIZE;
}

int setPayload(char *pkt, int idx, char *payload){
	int len;

	len = strlen(payload);
	idx = setPayloadLen(pkt, idx, len);
	for(int i = 0; i < len; i++)
		pkt[idx++] = payload[i];
	return idx;
}

int makePublish(char *pkt, struct pub * pub_pkt){
	int idx;	

	idx = FIXED_HEADER_SIZE;	
	idx = setMsgID(pkt, idx, 0);
	idx = setTopic(pkt, idx, pub_pkt->p_top);
	idx = setPayload(pkt, idx, pub_pkt->p_payload);
	setFixedHeader(pkt, 3, 0, 1, 0, idx - FIXED_HEADER_SIZE); 
	return idx;
}


void viewPublish(char *pkt){

	topic *x;

	viewFixedHeader(pkt);
	printf("message ID:%d\n", getMsgID(pkt, FIXED_HEADER_SIZE));
	x = getTopic(pkt, FIXED_HEADER_SIZE + MESSAGE_ID_SIZE);
	printf("Location:%s\nService:%s\n", x->t_loc, x->t_ser);
	free(x);
}	
/*
*/
int makePuback(char *pkt, int msgID){
	int idx = FIXED_HEADER_SIZE;
	idx = setMsgID(pkt, idx, msgID);
	setFixedHeader(pkt, 4, 0, 1, 0, idx - FIXED_HEADER_SIZE);
	return idx;	
}

int recvPuback(char *pkt){
	int idx = FIXED_HEADER_SIZE;
	return get_msg_type(pkt[0]);
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


int checkSubscribe(struct sub *usr, struct pub * pub_pkt){
	for(int i = 0; i < usr->s_cnt; i++)
		if( !strcmp(pub_pkt->p_top->t_loc, usr->s_top[i]->t_loc) && 
			!strcmp(pub_pkt->p_top->t_ser, usr->s_top[i]->t_ser))
			return 0; /// subscribed to topic
	return 1;// none 
}

void recvSubscribe(char *pkt, struct sub *a, int len){	
	char *id;
	int idx;
	topic *x;
	int relen, toplen;
 
	idx = FIXED_HEADER_SIZE + MESSAGE_ID_SIZE;
	relen = getRemainingLen(pkt) - MESSAGE_ID_SIZE;
	id = getClientID(pkt, idx);
	idx += CLIENT_ID_SIZE;
	relen -= CLIENT_ID_SIZE;
	for(int i = 0; i < len; i++)
		if( strcmp(a[i].s_cli_id, id) == 0){
				while(relen){
					toplen = getTopicLen(pkt,idx);
					x = getTopic(pkt, idx);
					idx += TOPIC_NAME_LENGTH_SIZE;
					relen -= TOPIC_NAME_LENGTH_SIZE;
					idx += toplen;
					relen -= toplen;
					a[i].s_top[a[i].s_cnt++] = x;
				}
				break;
		}
}

void viewSubscribe(char *pkt){
	topic *x;
	int relen, toplen;
	int idx;

	idx = FIXED_HEADER_SIZE;
	relen = getRemainingLen(pkt);
	viewFixedHeader(pkt);
	printf("message ID:%d\n",getMsgID(pkt, idx));
	idx += MESSAGE_ID_SIZE;
	relen -= MESSAGE_ID_SIZE;
	printf("Client ID:%s\n", getClientID(pkt,idx));
	idx += CLIENT_ID_SIZE;
	relen -= CLIENT_ID_SIZE;
	while(relen){
		toplen = getTopicLen(pkt,idx);
		x = getTopic(pkt, idx);
		idx += TOPIC_NAME_LENGTH_SIZE;
		relen -= TOPIC_NAME_LENGTH_SIZE;
		printf("Location:%s\nService:%s\n", x->t_loc, x->t_ser);
		idx += toplen;
		relen -= toplen;
		free(x);
	}
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

#define	Pthread_mutex_lock(mptr) \
	{	int  n; \
		if ( (n = pthread_mutex_lock(mptr)) != 0) \
			{ errno = n; perror("pthread_mutex_lock error"); } \
	}
#define	Pthread_mutex_unlock(mptr) \
	{	int  n; \
		if ( (n = pthread_mutex_unlock(mptr)) != 0) \
			{ errno = n; perror("pthread_mutex_unlock error"); } \
	}
#define	Pthread_cond_wait(cptr,mptr) \
	{	int  n; \
		if ( (n = pthread_cond_wait(cptr,mptr)) != 0) \
			{ errno = n; perror("pthread_cond_wait error"); } \
	}
#define	Pthread_cond_signal(cptr) \
	{	int  n; \
		if ( (n = pthread_cond_signal(cptr)) != 0) \
			{ errno = n; perror("pthread_cond_signal error"); } \
	}
#define	Pthread_cond_broadcast(cptr) \
	{	int  n; \
		if ( (n = pthread_cond_broadcast(cptr)) != 0) \
			{ errno = n; perror("pthread_cond_signal error"); } \
	}
#endif