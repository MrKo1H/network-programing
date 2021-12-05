#include "./broker.h"


int main(){
	struct sockaddr_in cli_addr;
	int serv_socklen , cli_addrlen;
	int connfd;
	char *cli_connfd;

	connfd = passiveTCP();
	
}