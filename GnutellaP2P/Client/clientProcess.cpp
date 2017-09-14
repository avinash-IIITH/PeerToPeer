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

/*This function will download the file from another client*/

void* getFileFromMirror(void *cstr){
	string parsedMessage = string((char*)cstr);
	cout << parsedMessage << endl;
	
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
	    	cout << "Invalid Request";
	    	return "Invalid Request";
	    }
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
	string key;
	int responseCount=1;

	fileMirrors.clear();
	while(1){
		found=serverReponse.find("||");
		if (found != string::npos){
			key=serverReponse.substr(0,found);
			fileMirrors[responseCount]=key;
			serverReponse=serverReponse.substr(found+2);
		}else{
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
		if(it->first == 1){
			found=replacedString.find(":");
			replacedString=replacedString.substr(found+1);
		}
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


/*This function is the entry point for client processes*/
int main(int argc, char* argv[]){

	int clientSocketFD;
	string inputCommand;
	string parsedInputString;
	int readSize;
	char serverReponseArray[2000];
	string serverReponse;
	
	populateClientParam(argc, argv); //populate server parameter
	
	while(1){
		cout << "Please enter command for CRS" << endl;
		getline(cin, inputCommand);

		parsedInputString = parseInputCommand(inputCommand);

		if(parsedInputString == "Invalid Request" || parsedInputString == "continue"){
			continue;
		}

		clientSocketFD = connectServerSocket(serverPortNum,serverIPAddress); //Initiate connnection to Server Socket
		char * cstr = new char [parsedInputString.length()+1];
	    strcpy (cstr, parsedInputString.c_str());
	    cout << "Request Message: " << cstr <<endl;
	    if( send(clientSocketFD , cstr , strlen(cstr) , 0) < 0){
	        perror("send failed. Try again after sometime");
	    }		
		
		/*Begin- connection to server failed*/
		if(clientSocketFD == -1) {
			perror("oops: server down. Try again after some time");
		}
		/*End- connection to server failed*/
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