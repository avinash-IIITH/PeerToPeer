/* Begin -includes*/
#include <stdio.h>
#include <string.h>
#include "clientRequestHelper.h"
/* End -includes*/

/*Begin- Global variables*/
/*End- Global variables*/


/*This function is the entry point for client processes*/
int main(){
	
	char ch = 'A';
	char *ip;
	ip = strdup("10.1.134.72");
	int serverConnectResponse;

	serverConnectResponse = connectServerSocket(9734,ip); //Initiate connnection to Server Socket
	
	/*Begin- connection to server failed*/
	if(serverConnectResponse == -1) {
		perror("oops: client1");
		exit(1);
	}
	/*End- connection to server failed*/

	write(socketFD, &ch, 1);
	read(socketFD, &ch, 1);
	printf("char from server = %c\n", ch);
	close(socketFD);
	exit(0);
}