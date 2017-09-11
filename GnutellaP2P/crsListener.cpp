/* Begin -includes*/
#include <stdio.h>
#include "serverCreationHelper.h"
/* End -includes*/


/*Begin- Global variables*/
int clientSocketFileDesc;
char ch;
char cha='A';
/*End- Global variables*/

/*This function handles individual client requests*/
void handleClientRequest(){
	read(clientSocketFileDesc, &ch, 1);
	sleep(5);
	ch++;
	write(clientSocketFileDesc, &cha, 1);
	close(clientSocketFileDesc);
	exit(0);	
}


/*This function listens creates a Server socket
  For each incoming request, a seperate process is spawned*/
int main(){	

	createServerSocket(9734); //Create Server socket and bind it to port num: 9734
	suppressSIGCHILD(); //preventing the transformation of children into zombies

	while(1){
		
		printf("server waiting\n"); //ToDo- To be removed
		cha++;	

		clientSocketFileDesc= acceptClientConnection(); //Accept the connection

		if(fork() == 0) {
			handleClientRequest(); //ChildProcess - Client request handler

		}else{

			/*Begin- ParentProcess - Free ClientSocketFD*/
			close(clientSocketFileDesc);
			/*End- ParentProcess - Free ClientSocketFD*/

		}
	}
}