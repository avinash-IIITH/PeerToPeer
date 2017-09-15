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
/*End- Global variables*/





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

	cout << parsedMessage << endl;
	filename=parsedMessage.substr(parsedMessage.find("#@#")+3);
	parsedMessage= parsedMessage.substr(parsedMessage.find("[")+1);
	
	indexStr =parsedMessage.substr(0,2);

	if(!indexStr.empty()){
		index = stoi (indexStr,nullptr,10);
		it = fileMirrors.find(index);
		if(it != fileMirrors.end()){
			cout << it->second << endl;
			requestParam = splitMessage(it->second);
			filePath = requestParam.at(0);
			mirrorIPAddress = new char [requestParam.at(3).length()+1];
			strcpy (mirrorIPAddress, requestParam.at(3).c_str());
			portNum = (unsigned int)stoi(requestParam.at(5),nullptr,10);

			recieveFileFD = connectServerSocket(portNum,mirrorIPAddress);

			if(recieveFileFD == -1) {
				perror("SERVER_OFFLINE");
				threadCount--;
				pthread_exit(NULL);
			}


			cout << "recieveFileFD: " << recieveFileFD << endl;
			char * cstr = new char [filePath.length()+1];
		    strcpy (cstr, filePath.c_str());
		    if( send(recieveFileFD , cstr , strlen(cstr) , 0) < 0){
		        perror("send failed. Try again after sometime");
		        cout << "mara lo " << endl;
		    }			

			char * cFileName = new char [filename.length()+1];
		    strcpy (cFileName, filename.c_str());

			if ( (incomingFileDescriptor = open(cFileName, O_WRONLY|O_CREAT, 0644)) < 0 ){
				perror("error creating file");
			}

			while ( (recievedBytes = recv(recieveFileFD, recievedData, 256, 0)) > 0 ){
				if (write(incomingFileDescriptor, recievedData, recievedBytes) < 0 ){
					perror("error writing to file");
				}
			}
			close(incomingFileDescriptor); 

		}else{
			cout << "Invalid Get Request" << endl;
		}
	}else{
		cout << "Invalid Get Request" << endl;
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
	    	}else{
	    		char * cstr = new char [parsedString.length()+1];
    			strcpy (cstr, parsedString.c_str());
	    		if(pthread_create(&getFileThread[threadCount++], NULL, getFileFromMirror, (void*)cstr) != 0) {
					fprintf(stderr, "Error creating thread\n");					
				}
				return "continue";
	    	}
	    }else{
	    	cout << "INVALID_ARGUEMENT";
	    	return "INVALID_ARGUEMENT";
	    }
	}else{
		cout << "INVALID_ARGUEMENT";
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
    		flag = flag^flag;
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
		if(responseCount ==1 ){
			found=serverReponse.find("#@#");
			fileName = serverReponse.substr(0,found);
		}

		found=serverReponse.find("||");
		if (found != string::npos){
			key=serverReponse.substr(0,found);
			if(responseCount !=1){
				key = fileName+"#@#"+key;
			}
			fileMirrors[responseCount]=key;
			serverReponse=serverReponse.substr(found+2);
		}else{
			if(responseCount !=1){
				serverReponse = fileName+"#@#"+serverReponse;
			}
			fileMirrors[responseCount]=serverReponse;
			break;
		}
		responseCount++;		
	}
	cout << "FOUND:" <<fileMirrors.size() <<endl ;

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


/*This function handles response from server*/
void handleServerResponse(string serverReponse, string parsedInputString){
	size_t found;
	string key;

	if(serverReponse == "INVALID_REQUEST" ||serverReponse == "FILE_NOT_FOUND" ||serverReponse == "SUCCESS"){
		cout << serverReponse << endl;
		return;
	}

	found=parsedInputString.find("#@#");
	if (found != string::npos){
		key=parsedInputString.substr(0,found);
		if(key == "search"){
			handleSearchResponse(serverReponse, parsedInputString);
		}
	}
}


/*This function is used to serve the thread for file transfer requests*/
void *transferFile(void *threadid){
	int clientSocketFileDesc, readSize, outgoingFileDescriptor;
	char clientMessageArray[2000];
	ssize_t sentBytes, readBytes;
	char sendBuffer[256]; 

	createServerSocket(nodeIPAddress,nodeDownloadPortNum); //Create Server socket and bind it to port num: 9734
	cout << "Socket Ready: " << nodeIPAddress << ":" << nodeDownloadPortNum << endl;
	suppressSIGCHILD(); //preventing the transformation of children into zombies

	while(1){
		clientSocketFileDesc= acceptClientConnection(); //Accept the connection
		cout << "Connection accept: " << clientSocketFileDesc << endl;

		memset(clientMessageArray, '\0', 2000);
		readSize = recv(clientSocketFileDesc , clientMessageArray , 2000 , 0);   	
	    if(readSize == 0){
	        printf("Client disconnected");
	        fflush(stdout);
	    }else if(readSize == -1){
	        perror("recv failed");
	    }else{
	    	//clientMessage = string(clientMessageArray);
	    	cout << "File Download Request: " << clientMessageArray << endl;
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
			close(outgoingFileDescriptor);
		} 

	    close(clientSocketFileDesc);
	}
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
	
	populateClientParam(argc, argv); //populate server parameter

	if(pthread_create(&threadForFileTransfer, NULL, transferFile, NULL)) {

		fprintf(stderr, "Error creating thread\n");
		
	}
	
	while(1){
		cout << "Please enter command for CRS" << endl;
		getline(cin, inputCommand);

		parsedInputString = parseInputCommand(inputCommand);

		if(parsedInputString == "Invalid Request" || parsedInputString == "continue"){
			continue;
		}

		clientSocketFD = connectServerSocket(serverPortNum,serverIPAddress); //Initiate connnection to Server Socket

		/*Begin- connection to server failed*/
		if(clientSocketFD == -1) {
			perror("SERVER_OFFLINE");
			continue;
		}
		/*End- connection to server failed*/


		char * cstr = new char [parsedInputString.length()+1];
	    strcpy (cstr, parsedInputString.c_str());
	    cout << "Request Message: " << cstr <<endl;
	    if( send(clientSocketFD , cstr , strlen(cstr) , 0) < 0){
	        perror("send failed. Try again after sometime");
	    }		
		
		
		memset(serverReponseArray, '\0', 2000);
		readSize = recv(clientSocketFD , serverReponseArray , 2000 , 0);   

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
	exit(0);
}