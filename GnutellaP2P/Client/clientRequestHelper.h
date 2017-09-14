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
int socketFD;
int length;
struct sockaddr_in serverSocketAddress;
int result;
/*End- Global variables*/


/*This function is for connecting to server socket*/
int connectServerSocket(unsigned int portNumber, char* ipAddress){
	socketFD = socket(AF_INET, SOCK_STREAM, 0); //ToDo-Exception Handling
	serverSocketAddress.sin_family = AF_INET;
	serverSocketAddress.sin_addr.s_addr = inet_addr(ipAddress);
	serverSocketAddress.sin_port = htons(portNumber);
	length = sizeof(serverSocketAddress);
	result = connect(socketFD, (struct sockaddr *)&serverSocketAddress, length); //ToDo-Exception Handling
	if(result == 0){
		return socketFD;
	}else{
		return -1;
	}
}

/*This function is for connecting to server socket*/
int closeConnection(int clientSocketFD){
	close(clientSocketFD);
	return 0;
}

