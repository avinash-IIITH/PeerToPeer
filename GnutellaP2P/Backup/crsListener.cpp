/* Begin -includes*/
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
/* End -includes*/


/*Begin- Global variables*/
int serverSocketFD, clientSocketFD;
unsigned int serverLength, clientLength;
struct sockaddr_in serverAddress;
struct sockaddr_in clientAddress;
/*End- Global variables*/


/*This function creates a server socket*/
void createServerSocket(){
	serverSocketFD = socket(AF_INET, SOCK_STREAM,0); //ToDo-Exception Handling
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(9734);
	serverLength = sizeof(serverAddress);
	bind(serverSocketFD,(struct sockaddr *)&serverAddress,serverLength); //ToDo-Exception Handling
	listen(serverSocketFD, 5); //ToDo-Exception Handling, Listen on the server socket and maintain a queue of length 5
}

/*This function is for preventing the transformation of children into zombies*/
void suppressSIGCHILD(){
	signal(SIGCHLD, SIG_IGN); // Ignore child exit details
}

/*This function listens creates a Server socket*/
/*For each incoming request, a seperate process is spawned*/
int main(){
	char cha='A';
	createServerSocket(); //Create Server socket and bind it to port num: 9734	

	suppressSIGCHILD(); //preventing the transformation of children into zombies

	while(1){
		char ch;

		printf("server waiting\n"); //ToDo- To be removed
		
		/*Begin- Accept the connection*/
		clientLength = sizeof(clientAddress);
		clientSocketFD = accept(serverSocketFD,(struct sockaddr *)&clientAddress, &clientLength); //ToDo-Exception Handling
		/*End- Accept the connection*/
		cha++;
		if(fork() == 0) {

			/*Begin- ChildProcess - Client request handler*/
			read(clientSocketFD, &ch, 1);
			sleep(5);
			ch++;
			printf("%c\n", cha);
			write(clientSocketFD, &cha, 1);
			close(clientSocketFD);
			exit(0);
			/*End- ChildProcess - Client request handler*/

		}else{
			/*Begin- ParentProcess - Free ClientSocketFD*/
			close(clientSocketFD);
			/*End- ParentProcess - Free ClientSocketFD*/
		}
	}

}