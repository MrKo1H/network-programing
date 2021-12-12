#include "./broker.h"

struct sub subscriber[SUBSCRIBER_MAX];

int cnt_sub = 0; // number of subscriber



pthread_mutex_t pub_recv_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  pub_recv_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pub_sent_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  pub_sent_cond = PTHREAD_COND_INITIALIZER;
int pub_com = 0; // number of publish sent

struct pub pub_recv;

static void *handler(void *arg);

int main(){
	struct sockaddr_in cli_addr;
	int  cli_addrlen;
	int connfd;
	int *cli_connfd;
	pthread_t tid;
	int err;

	connfd = passiveTCP();
	cli_addrlen = sizeof(struct sockaddr_in);

	while(1){
		cli_connfd = (int *)malloc(sizeof(int));
		*cli_connfd = accept(connfd, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);
 		printf("[+] received connection from %s\n", inet_ntoa(cli_addr.sin_addr));

 		if((err= pthread_create(&tid, NULL, &handler, (void *)cli_connfd)) < 0){
 			errno = err;
 			perror("pthread_create");
 			exit(1);
 		}
	}
}

static void *handler(void *arg){
	int connfd;	
	char recv_buff[BUFFER_SIZE], sent_buff[BUFFER_SIZE];
	int n_read, n_write;
	struct sub *usr;
	int msgtype;

	connfd = *((int *)arg);
	free(arg);
	pthread_detach(pthread_self());

	if( (n_read = read(connfd, recv_buff, BUFFER_SIZE)) < 0)
		return NULL;

	switch(get_msg_type(recv_buff[0])){
		case 1: // received connect
			puts("[-] received connect");
			viewConnect(recv_buff);
			puts("[-] sent connack");
			n_write = makeConnack(sent_buff, ACCECPT);
			write(connfd, sent_buff, n_write);
			Pthread_mutex_lock(&pub_recv_mutex);
			recvConnect(recv_buff, &subscriber[cnt_sub]);
			usr = &subscriber[cnt_sub++];
			Pthread_mutex_unlock(&pub_recv_mutex);
			while(1){
				Pthread_mutex_lock(&pub_recv_mutex);
				Pthread_cond_wait(&pub_recv_cond,&pub_recv_mutex);
				if( checkSubscribe(usr, &pub_recv)){
					Pthread_mutex_unlock(&pub_recv_mutex);
					continue;
				}
				n_write = makePublish(sent_buff, &pub_recv);
				msgtype = -1;
				while(msgtype != 4){ /// puback
					write(connfd, sent_buff, n_write);
					if( (n_read = read(connfd, recv_buff, BUFFER_SIZE)) == 0){
						Pthread_mutex_unlock(&pub_recv_mutex);
						return NULL;
					};
					msgtype = recvPuback(recv_buff);
				}
				Pthread_mutex_unlock(&pub_recv_mutex);

			}			
			break;
		case 2: 
			break;
		case 3: // received publish
			puts("[-] received publish ");
			viewPublish(recv_buff);
			Pthread_mutex_lock(&pub_recv_mutex);
			recvPublish(recv_buff, &pub_recv);	
			n_write = makePuback(sent_buff, 0);
			write(connfd, sent_buff, n_write);
			Pthread_cond_broadcast(&pub_recv_cond);
			Pthread_mutex_unlock(&pub_recv_mutex);
			break;
		case 8: // subscribe
			puts("[-] received subscribe"); 
			viewSubscribe(recv_buff);
			Pthread_mutex_lock(&pub_recv_mutex);
			recvSubscribe(recv_buff, subscriber, cnt_sub);
			Pthread_mutex_unlock(&pub_recv_mutex);
			break;
	}
	return NULL;
}
