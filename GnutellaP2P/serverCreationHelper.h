/* Begin -includes*/
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <functional> 
/* End -includes*/


/*Begin- Global variables*/
int serverSocketFD, clientSocketFD;
unsigned int serverLength, clientLength;
struct sockaddr_in serverAddress;
struct sockaddr_in clientAddress;
/*End- Global variables*/


/*This function creates a server socket*/
void createServerSocket(char * ip,unsigned int portNumber){
	serverSocketFD = socket(AF_INET, SOCK_STREAM,0); //ToDo-Exception Handling
	serverAddress.sin_family = AF_INET;
	//serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_addr.s_addr = inet_addr(ip);
	serverAddress.sin_port = htons(portNumber);
	serverLength = sizeof(serverAddress);
	bind(serverSocketFD,(struct sockaddr *)&serverAddress,serverLength); //ToDo-Exception Handling
	listen(serverSocketFD, 5); //ToDo-Exception Handling, Listen on the server socket and maintain a queue of length 5
}

/*This function is for preventing the transformation of children into zombies*/
void suppressSIGCHILD(){
	signal(SIGCHLD, SIG_IGN); // Ignore child exit details
}

/*This function is for accepting client connection once server is ready*/
int acceptClientConnection(){
	clientLength = sizeof(clientAddress);
	clientSocketFD = accept(serverSocketFD,(struct sockaddr *)&clientAddress, &clientLength); //ToDo-Exception Handling
	return clientSocketFD;
}