/* Begin -includes*/
#include "clientRequestHelper.h"
#include <pthread.h>
#include "serverCreationHelper.h"
/* End -includes*/

using namespace std;

/*Begin- Global variables*/
char *nodeAlias;
char *nodeIPAddress;
unsigned int nodePortNum;
char *serverIPAddress;
unsigned int serverPortNum;
unsigned int nodeDownloadPortNum;
char * repositoryFile;
char * mirrorList;
char * rootFolder;
map<int, string> fileMirrors;
int threadCount;
pthread_t getFileThread[10000];
ofstream ofs;
time_t rawtime;
/*End- Global variables*/



/*This function is for logging Messages*/
void logMessage(string message){	
	char *c_time_string;
	int len_of_new_line;
	
	c_time_string = ctime(&rawtime);
    len_of_new_line = strlen(c_time_string) - 1;
    c_time_string[len_of_new_line] = '\0';

	ofs << c_time_string << ": " << message << endl; 
}

/*This function is to populate client parameter*/
void populateClientParam(int argc, char* argv[]){
	if(argc < 8){
		printf("Insufficient parameters. Exiting Client initialization \n");
		exit(EXIT_FAILURE);
	}else{
		nodeAlias = argv[1];
		nodeIPAddress = argv[2];
		nodePortNum = (unsigned int)strtol(argv[3],NULL,10);
		serverIPAddress = argv[4];
		serverPortNum = (unsigned int)strtol(argv[5],NULL,10);
		nodeDownloadPortNum = (unsigned int)strtol(argv[6],NULL,10);
		rootFolder = argv[7];	
		threadCount =0;	
	}
}

/*This function a String based on Delimiters*/
vector<string> splitMessage(string clientMessage){
	vector<string> requestParam;
	size_t found;
	string key;
	while(1){
		found=clientMessage.find("#@#");
		if (found != string::npos){
			key=clientMessage.substr(0,found);
			requestParam.push_back(key);
			clientMessage=clientMessage.substr(found+3);
		}else{
			requestParam.push_back(clientMessage);
			break;
		}		
	}

	return requestParam;
}

/*This function will download the file from another client*/

void* getFileFromMirror(void *cstr){
	string parsedMessage = string((char*)cstr);
	string filename,indexStr;
	size_t found;
	ssize_t recievedBytes;
	int index,recieveFileFD;
	vector<string> requestParam;
	map<int, string >::iterator it;
	char* mirrorIPAddress;
	string filePath, newFileName;
	unsigned portNum;
	int incomingFileDescriptor;
	char recievedData[256]; 

	filename=parsedMessage.substr(parsedMessage.find("#@#")+3);
	parsedMessage= parsedMessage.substr(parsedMessage.find("[")+1);
	
	//indexStr =parsedMessage.substr(0,2);
	indexStr = parsedMessage.substr(0,parsedMessage.find("]"));
	
	if(!indexStr.empty()){
		index = stoi (indexStr,nullptr,10);
		cout << "index: " << index << endl;
		it = fileMirrors.find(index);
		if(it != fileMirrors.end()){
			cout << "it->second: " << it->second << endl;
			requestParam = splitMessage(it->second);
			filePath = requestParam.at(2);
			cout << "filePath: " << filePath << endl;
			mirrorIPAddress = new char [requestParam.at(4).length()+1];
			strcpy (mirrorIPAddress, requestParam.at(4).c_str());
			portNum = (unsigned int)stoi(requestParam.at(6),nullptr,10);
			cout << "portNum: " << portNum << " mirrorIPAddress: " << mirrorIPAddress << endl;
			recieveFileFD = connectServerSocket(portNum,mirrorIPAddress);

			if(recieveFileFD == -1) {
				perror("SERVER_OFFLINE");
				threadCount--;
				pthread_exit(NULL);
			}

			char * cstr = new char [filePath.length()+1];
		    strcpy (cstr, filePath.c_str());
		    if( send(recieveFileFD , cstr , strlen(cstr) , 0) < 0){
		        perror("send failed. Try again after sometime");
		        
		    }			

			char * cFileName = new char [filename.length()+1];
		    strcpy (cFileName, filename.c_str());

			if ( (incomingFileDescriptor = open(cFileName, O_WRONLY|O_CREAT, 0644)) < 0 ){
				perror("error creating file");
			}

			while ( (recievedBytes = recv(recieveFileFD, recievedData, 256, 0)) > 0 ){
				
				if(string(recievedData) != "FILE_NOT_FOUND"){
					if (write(incomingFileDescriptor, recievedData, recievedBytes) < 0 ){
						perror("error writing to file");
					}
				}else{
					cout << "FILE_NOT_FOUND" <<  endl << endl;
					pthread_exit(NULL);
				}
			}
			close(incomingFileDescriptor); 

			cout << "FILE DOWNLOAD SUCCESSFULL" << endl << endl;
			cout << "Please enter command for CRS" << endl << endl;

		}else{
			cout << "Invalid Get Request" << endl << endl;
		}
	}else{
		cout << "Invalid Get Request" << endl << endl;
	}

	threadCount--;
	pthread_exit(NULL);
}


/*This function is to modify the command string as per the commands*/
string updateCommandString(string parsedString){
	size_t found;
	string key,temp;
	string commandString,fileName;

	found=parsedString.find("#@#");
	if (found != string::npos){
		key=parsedString.substr(0,found);
		if(key == "search"){
	    	return parsedString;
	    }else if(key == "share" || key == "del"){
	    	temp= parsedString.substr(found+3);
	    	found = temp.find_last_of('/');
	    	if(found == string::npos){
	    		fileName=temp;
	    	}else{
	    		fileName = temp.substr(found+1);
	    	}
	    	
	    	commandString = key+"#@#"+fileName+"#@#"+temp+"#@#"+string(nodeAlias)+"#@#";
	    	commandString = commandString + string(nodeIPAddress)+"#@#"+to_string(nodePortNum)+"#@#"+to_string(nodeDownloadPortNum);
	    	return commandString;
	    }else if(key == "get"){
	    	string temp;
	    	parsedString =parsedString.substr(found+3);
	    	found=parsedString.find("[");
	    	if(found == string::npos){
	    		//To Do get command to CRS
	    		parsedString = key + "#@#" + parsedString;
	    		return parsedString;
	    	}else{
	    		
	    		char * cstr = new char [parsedString.length()+1];
    			strcpy (cstr, parsedString.c_str());
	    		if(pthread_create(&getFileThread[threadCount++], NULL, getFileFromMirror, (void*)cstr) != 0) {
					fprintf(stderr, "Error creating thread\n");					
				}
				return "continue";
	    	}
	    }else{
	    	cout << "INVALID_ARGUEMENT" << endl << endl;
	    	return "INVALID_ARGUEMENT";
	    }
	}else{
		cout << "INVALID_ARGUEMENT" << endl << endl;
		return "INVALID_ARGUEMENT";
	}	
}


/*This function is to parsing command parameter*/
string parseInputCommand(string inputCommand){
	int flag=0;
	char c;
	string parsedString="";

	for (unsigned i=0; i<inputCommand.length(); ++i){
    	c = inputCommand.at(i);

    	if(c == '\\'){
    		i++;
    		c = inputCommand.at(i);
    		parsedString = parsedString + c;
    	}else if(c == '\"'){
    		if(flag ==0){
    			flag = 1;
    		}else{
    			flag = 0;
    		}
    	}else{

	    	if(c == ' ' && flag == 1){
	    		parsedString = parsedString + c;
	    	}else if(c == ' ' && flag == 0){
	    		parsedString = parsedString + "#@#";
	    	}else{
	    		parsedString = parsedString + c;
	    	}
	    }
	}
	parsedString = updateCommandString(parsedString);
	return parsedString;
}


/*This function handles search response from server*/
void handleSearchResponse(string serverReponse, string parsedInputString){
	vector<string> requestParam;
	size_t found;
	string key,fileName;
	int responseCount=1;

	fileMirrors.clear();
	while(1){
		//if(responseCount ==1 ){
			found=serverReponse.find("#@#");
			fileName = serverReponse.substr(0,found);
		//}

		found=serverReponse.find("||");
		if (found != string::npos){
			key=serverReponse.substr(0,found);
			//if(responseCount !=1){
				key = fileName+"#@#"+key;
			//}
			fileMirrors[responseCount]=key;
			serverReponse=serverReponse.substr(found+2);
		}else{
			//if(responseCount !=1){
				serverReponse = fileName+"#@#"+serverReponse;
			//}
			fileMirrors[responseCount]=serverReponse;
			break;
		}
		responseCount++;		
	}
	cout << "FOUND:" <<fileMirrors.size() <<endl << endl;

	for (map<int, string>::iterator it = fileMirrors.begin() ; it != fileMirrors.end(); ++it){
		string mirrorDetail = it->second;
		string replacedString;
		while(1){
			found=mirrorDetail.find("#@#");
			if (found != string::npos){				
				replacedString= replacedString + mirrorDetail.substr(0,found)+":";						
				mirrorDetail=mirrorDetail.substr(found+3);
			}else{
				replacedString= replacedString + mirrorDetail;
				break;
			}
		}
		
		found=replacedString.find(":");
		replacedString=replacedString.substr(found+1);
		
		cout << "[" << it->first << "] " << replacedString << endl;
	}
}


/*This function handles search response from server*/
void handleGetResponse(string serverReponse, string parsedInputString){
	vector<string> requestParam;
	size_t found;
	string key,fileName;

	fileMirrors.clear();

	found=serverReponse.find("#@#");
	serverReponse = serverReponse.substr(0,found) + "#@#" + serverReponse;
	cout << "serverReponse: " << serverReponse << endl;
	cout << "parsedInputString: " << parsedInputString << endl;
	fileMirrors[1] = serverReponse;

	while(1){
		found=serverReponse.find("#@#");
		if (found != string::npos){
			serverReponse = serverReponse.substr(found+3);
		}else{
			fileName = serverReponse;
			break;
		}

	}

	serverReponse = "[1]#@#"+fileName;

	char * cstr = new char [serverReponse.length()+1];
	strcpy (cstr, serverReponse.c_str());
	if(pthread_create(&getFileThread[threadCount++], NULL, getFileFromMirror, (void*)cstr) != 0) {
		fprintf(stderr, "Error creating thread\n");					
	}
	//return "continue";

}


/*This function handles response from server*/
void handleServerResponse(string serverReponse, string parsedInputString){
	size_t found;
	string key;

	if(serverReponse == "INVALID_REQUEST" ||serverReponse == "FILE_NOT_FOUND" ||serverReponse == "SUCCESS"){
		cout << serverReponse << endl << endl;
		return;
	}

	if(serverReponse == "PING SUCCESS"){
		return;
	}

	found=parsedInputString.find("#@#");
	if (found != string::npos){
		key=parsedInputString.substr(0,found);
		if(key == "search"){
			handleSearchResponse(serverReponse, parsedInputString);
		}else if(key == "get"){
			handleGetResponse(serverReponse, parsedInputString);
		}
	}
}


/*This function is used to serve the thread for file transfer requests*/
void *transferFile(void *threadid){
	int clientSocketFileDesc, readSize, outgoingFileDescriptor;
	char clientMessageArray[2000];
	ssize_t sentBytes, readBytes;
	char sendBuffer[256]; 

	logMessage("creating server listening socket");
	createServerSocket(nodeIPAddress,nodeDownloadPortNum); //Create Server socket and bind it to port num: 9734
	
	suppressSIGCHILD(); //preventing the transformation of children into zombies

	while(1){
		clientSocketFileDesc= acceptClientConnection(); //Accept the connection
		
		memset(clientMessageArray, '\0', 2000);
		readSize = recv(clientSocketFileDesc , clientMessageArray , 2000 , 0); 
		logMessage("Message Recieved: "+ string(clientMessageArray));  	
	    if(readSize == 0){
	        printf("Client disconnected");
	        fflush(stdout);
	    }else if(readSize == -1){
	        perror("recv failed");
	    }else{
	    	//clientMessage = string(clientMessageArray);	    	
	    	cout << "File Download Request: " << clientMessageArray << endl << endl;
	    }

		if( (outgoingFileDescriptor = open(clientMessageArray, O_RDONLY)) < 0){
			perror(clientMessageArray);
			if((sentBytes = send(clientSocketFileDesc, "FILE_NOT_FOUND",strlen("FILE_NOT_FOUND"), 0)) < 0 ){
				perror("send error");
			}
		}else{
			while( (readBytes = read(outgoingFileDescriptor, sendBuffer, 256)) > 0 ){
				if( (sentBytes = send(clientSocketFileDesc, sendBuffer, readBytes, 0))< readBytes ){
					perror("send error");
				}
			}
			logMessage("File transfer complete: "+ string(clientMessageArray));  	
			close(outgoingFileDescriptor);
		} 

	    close(clientSocketFileDesc);
	}
	pthread_exit(NULL);
}

/*This function performs at exit operations */
void cleanOfstream(){
	ofs.close();
}


/*This function sends heartbeat to the CRS at an interval of 5 min*/
void *sendHeartBeat(void *threadid){
	int pingThreadFD,readSize;
	string pingMessage;
	char pingResponseArray[200];
	pingMessage = string("ping#@#")+ string(nodeAlias)+"#@#"+string(nodeIPAddress)+"#@#"+to_string(nodePortNum);
	pingMessage=pingMessage+"#@#"+to_string(nodeDownloadPortNum);
	char * cstr = new char [pingMessage.length()+1];
    strcpy (cstr, pingMessage.c_str());
    
    while(1){
    	sleep(30);
		pingThreadFD = connectServerSocket(serverPortNum,serverIPAddress);
		if(pingThreadFD == -1) {
			perror("SERVER_OFFLINE");
			continue;
		}

	    if( send(pingThreadFD , cstr , strlen(cstr) , 0) < 0){
	        perror("send failed. Try again after sometime");
	    }	
		
		memset(pingResponseArray, '\0', 200);
		readSize = recv(pingThreadFD , pingResponseArray , 200 , 0);   

	    if(readSize == 0){
	        printf("server disconnected");
	        fflush(stdout);
	    }else if(readSize == -1){
	        perror("recv failed");
	    }
		close(pingThreadFD);
	}
	pthread_exit(NULL);
}


/*This function is the entry point for client processes*/
int main(int argc, char* argv[]){

	int clientSocketFD;
	string inputCommand;
	string parsedInputString;
	int readSize;
	char serverReponseArray[2000];
	string serverReponse;
	pthread_t threadForFileTransfer;
	pthread_t threadForHeartBeat;
	
	populateClientParam(argc, argv); //populate server parameter

	ofs.open("client.log", ofstream::out | ofstream::app);

	if(pthread_create(&threadForFileTransfer, NULL, transferFile, NULL)) {

		fprintf(stderr, "Error creating thread\n");
		
	}

	if(pthread_create(&threadForHeartBeat, NULL, sendHeartBeat, NULL)) {

		fprintf(stderr, "Error creating thread\n");		
	}
	
	while(1){
		cout << "Please enter command for CRS" << endl << endl;
		getline(cin, inputCommand);

		parsedInputString = parseInputCommand(inputCommand);

		if(parsedInputString == "Invalid Request" || parsedInputString == "continue" || parsedInputString == "INVALID_ARGUEMENT"){
			continue;
		}
		
		logMessage("Connecting Server");
		clientSocketFD = connectServerSocket(serverPortNum,serverIPAddress); //Initiate connnection to Server Socket

		/*Begin- connection to server failed*/
		if(clientSocketFD == -1) {
			perror("SERVER_OFFLINE");
			continue;
		}
		/*End- connection to server failed*/


		char * cstr = new char [parsedInputString.length()+1];
	    strcpy (cstr, parsedInputString.c_str());

	    logMessage("Request Message- "+parsedInputString);

	    if( send(clientSocketFD , cstr , strlen(cstr) , 0) < 0){
	        perror("send failed. Try again after sometime");
	    }		
		logMessage("Message Sent Successful- "+parsedInputString);
		
		memset(serverReponseArray, '\0', 2000);
		readSize = recv(clientSocketFD , serverReponseArray , 2000 , 0);   

	    logMessage("Server Response- "+string(serverReponseArray));
	    if(readSize == 0){
	        printf("server disconnected");
	        fflush(stdout);
	    }else if(readSize == -1){
	        perror("recv failed");
	    }else{
	    	serverReponse = string(serverReponseArray);
	    	handleServerResponse(serverReponse,parsedInputString);
	    }

		close(clientSocketFD);
	}

	ofs.close();
	exit(0);
}