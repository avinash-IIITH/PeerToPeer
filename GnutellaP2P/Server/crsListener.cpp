/* Begin -includes*/
#include <pthread.h>
#include "serverCreationHelper.h"
/* End -includes*/

using namespace std;

/*Begin- Global variables*/
int clientSocketFileDesc;
char *serverIP;
unsigned int serverPortNum;
char * repositoryFile;
char * mirrorList;
char * rootFolder;
map<string,vector<string> > repositoryFileDS;
unordered_map<string,string > mirrorListDS;
pthread_t threadForFlushData;
int threadCount;
ofstream ofserver;
time_t rawtime;
unordered_map<string,time_t > liveMirrorListDS;
/*End- Global variables*/



/*Begin- Function Declaration*/
void pushRepositoryFileDS(string lineString);
void pushMirrorListDS(string lineString);
void flushDataToFile();
/*End- Function Declaration*/


/*This function is for logging Messages*/
void logMessage(string message){
	char *c_time_string;
	int len_of_new_line;

	c_time_string = ctime(&rawtime);
    len_of_new_line = strlen(c_time_string) - 1;
    c_time_string[len_of_new_line] = '\0';

	ofserver << c_time_string << ": " << message << endl; 
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


/*This function searches for file in File repository*/
string performSearch(vector<string> requestParam){

	if(requestParam.size() < 2){
		return "INVALID_REQUEST";
	}
	string searchParam,fileName;
	map<string,vector<string> >::iterator iterRepoDS;
	unordered_map<string,string >::iterator iterMirrorListDS;
	vector<string> repoFileDSVector;
	string searchResponse, temp, alias, value;
	size_t found;
	int flag =0;

	searchParam = requestParam.at(1);
	transform(searchParam.begin(), searchParam.end(), searchParam.begin(), ::tolower);


	if(repositoryFileDS.empty()){
		
		return "FILE_NOT_FOUND";
	}else{
		iterRepoDS=repositoryFileDS.find(searchParam);
		
		if(iterRepoDS != repositoryFileDS.end()){  		
						
			repoFileDSVector = iterRepoDS->second;
			for (vector<string>::iterator it = repoFileDSVector.begin() ; it != repoFileDSVector.end(); ++it){
				
    			temp = *it;
    			found = temp.find_last_of(':'); // to find alias
    			alias = temp.substr(found+1); // to find alias
    			
    			iterMirrorListDS=mirrorListDS.find(alias); //to find alias in mirrorListDS
    			
    			if(iterMirrorListDS != mirrorListDS.end()){  //if found
    				
					found=temp.find(":"); //end of filename
					value=temp.substr(found+1); //from filepath till alias

					if(!searchResponse.empty()){
						searchResponse = searchResponse+"||";
						flag =1;
					}

					if(searchResponse.empty()){
						searchResponse=temp.substr(0,found); // put filename in output string
						fileName =  searchResponse; // hold filename for empty comparison
					}				
					
					if(flag ==1){
						searchResponse = searchResponse+value;
					}else{
						searchResponse = searchResponse+"#@#"+value;
					}
					
					found = searchResponse.find_last_of(':');
    				searchResponse = searchResponse.substr(0,found)+"#@#"+alias+"#@#"; 
					value = iterMirrorListDS->second;
					found = value.find(':');
    				searchResponse = searchResponse+value.substr(0,found)+"#@#";
    				value = value.substr(found+1);
    				found = value.find(':');
    				searchResponse = searchResponse+value.substr(0,found)+"#@#"+value.substr(found+1);
				}
			}

			if(searchResponse == fileName || searchResponse.empty()){
				 return "FILE_NOT_FOUND";	
			}else{
				return searchResponse;	
			}							
		}else{  
			 			
		    return "FILE_NOT_FOUND";		    
		}
	}
}


/*This function searches for file in File repository*/
string performShare(vector<string> requestParam){
	string lineStringRepoDS, lineStringMirrorList;

	if(requestParam.size() < 7){
		return "INVALID_REQUEST";
	}

	lineStringRepoDS = requestParam.at(1)+":";
	lineStringRepoDS = lineStringRepoDS + requestParam.at(2) +":" + requestParam.at(3);
	pushRepositoryFileDS(lineStringRepoDS);	

	lineStringMirrorList = requestParam.at(3)+":";
	lineStringMirrorList = lineStringMirrorList + requestParam.at(4) +":"+ requestParam.at(5) +":"+ requestParam.at(6);
	pushMirrorListDS(lineStringMirrorList);
	return "SUCCESS";
}

/*This function searches for file in File repository*/
string performGet(vector<string> requestParam){
	string searchString,fileName;
	size_t found;
	map<string,vector<string> >::iterator iterRepoDS;
	unordered_map<string,string >::iterator iterMirrorListDS;
	vector<string> repoFileDSVector;
	string temp,key;

	if(requestParam.size() < 4){
		return "INVALID_REQUEST";
	}

	found = requestParam.at(2).find_last_of('/');
	if(found == string::npos){
		fileName=requestParam.at(2);
	}else{
		fileName = requestParam.at(2).substr(found+1);
	}

	searchString =  fileName+":"+requestParam.at(2)+":"+requestParam.at(1);
	temp = requestParam.at(1);

	if(!repositoryFileDS.empty()){
		iterRepoDS=repositoryFileDS.find(fileName);
		if(iterRepoDS == repositoryFileDS.end()){
    		return "FILE_NOT_FOUND";
    	}

		if((mirrorListDS.empty())||(mirrorListDS.find(temp) == mirrorListDS.end())){		
		    return "FILE_NOT_FOUND";
		}else{
			iterMirrorListDS = mirrorListDS.find(temp);
			temp = iterMirrorListDS->second;
		}

		searchString =  fileName+"#@#"+requestParam.at(2)+"#@#"+requestParam.at(1);

		while(1){
			found=temp.find(":");
			if (found != string::npos){
				searchString=searchString +"#@#"+ temp.substr(0,found);
				temp=temp.substr(found+1);
			}else{
				searchString=searchString +"#@#"+ temp;
				break;
			}		
		}
		searchString = searchString+"#@#"+requestParam.at(3);
		return searchString;

    }else{
    	return "FILE_NOT_FOUND";
    }
	
	return "SUCCESS";
}


/*This function searches for file in File repository*/
string performDelete(vector<string> requestParam){
	string searchParam, compareValue;
	map<string,vector<string> >::iterator iterRepoDS;
	unordered_map<string,string >::iterator iterMirrorListDS;
	vector<string> repoFileDSVector;
	string temp;

	if(requestParam.size() < 4){
		return "INVALID_REQUEST";
	}

	searchParam = requestParam.at(1);
	transform(searchParam.begin(), searchParam.end(), searchParam.begin(), ::tolower);

	compareValue = requestParam.at(1) +":"+ requestParam.at(2) + ":"+requestParam.at(3);

	if(!repositoryFileDS.empty()){
		iterRepoDS=repositoryFileDS.find(searchParam);
		if(iterRepoDS != repositoryFileDS.end()){    			
			repoFileDSVector = iterRepoDS->second;
			for (vector<string>::iterator it = repoFileDSVector.begin() ; it != repoFileDSVector.end(); ++it){
    			temp = *it;
    			
    			if(temp == compareValue){
    				
    				repoFileDSVector.erase(it);
    				iterRepoDS->second = repoFileDSVector;
    				break;
    			}else{
    				return "FILE_NOT_FOUND";
    			}
    		}
    	}else{
    		return "FILE_NOT_FOUND";
    	}
    }
    return "SUCCESS";
}

/*This function updates the live nodes*/
string performPing(vector<string> requestParam){
	string mirrorAlias= requestParam.at(1);
	time_t now;
	unordered_map<string,time_t > ::iterator iter;
	string pushIntoMirrorDS;

	if(requestParam.size() < 5){
		return "INVALID_REQUEST";
	}

	pushIntoMirrorDS= requestParam.at(1)+":"+requestParam.at(2)+":"+requestParam.at(3)+":"+requestParam.at(4);
	pushMirrorListDS(pushIntoMirrorDS);
	time(&now);
	if(!liveMirrorListDS.empty()){
		iter = liveMirrorListDS.find(mirrorAlias);
		if(iter != liveMirrorListDS.end()){
			iter->second = now;			
		}else{
			liveMirrorListDS[mirrorAlias] = now;
			
		}
	}else{
		liveMirrorListDS[mirrorAlias] = now;		
	}
	return "PING SUCCESS";
}


/*This function handles individual client requests*/
void handleClientRequest(){
	//read(clientSocketFileDesc, &ch, 1);
	//sleep(5);ch++;write(clientSocketFileDesc, &cha, 1);
	//close(clientSocketFileDesc);

	int readSize;
	char clientMessageArray[2000];
	string clientMessage;
	vector<string> requestParam;
	string response;

	logMessage("Waiting for client");
	clientSocketFileDesc= acceptClientConnection(); //Accept the connection

	memset(clientMessageArray, '\0', 2000);
	readSize = recv(clientSocketFileDesc , clientMessageArray , 2000 , 0);   

	logMessage("Message Recieved: " + string(clientMessageArray));	
    if(readSize == 0){
        printf("Client disconnected");
        fflush(stdout);
    }else if(readSize == -1){
        perror("recv failed");
    }else{
    	clientMessage = string(clientMessageArray);
    	cout << "Input request: " << clientMessage << endl;
    }

    requestParam = splitMessage(clientMessage);
    if(requestParam.at(0) == "search"){
    	response = performSearch(requestParam);
    }else if(requestParam.at(0) == "share"){
    	response = performShare(requestParam);
    }else if(requestParam.at(0) == "del"){
    	response = performDelete(requestParam);
    }else if(requestParam.at(0) == "get"){
    	response = performGet(requestParam);
    }else if(requestParam.at(0) == "ping"){
    	response = performPing(requestParam);
    }else{
    	cout << "Invalid request";
    	response = "INVALID_REQUEST";
    }

    char * cstr = new char [response.length()+1];
    strcpy (cstr, response.c_str());
    logMessage("Message sent to Client: " + response);
    cout << "Response: " << cstr << endl;
    if( send(clientSocketFileDesc , cstr , strlen(cstr) , 0) < 0){
        perror("send failed");
        exit (1);
    }

    close(clientSocketFileDesc);
	//exit(0);	//To-Do Uncomment
}


/*This function pushes data in repositoryFileDS map*/
void pushRepositoryFileDS(string lineString){
	string key,value;
	size_t found;
	map<string,vector<string> >::iterator iter, iter1;
	vector<string> repoFileDSVector;

	found=lineString.find(":");
	key=lineString.substr(0,found);
	transform(key.begin(), key.end(), key.begin(), ::tolower);
	value=lineString;

	if(repositoryFileDS.empty()){
		repoFileDSVector.push_back(value);
	    repositoryFileDS[key]=repoFileDSVector;
	    
	}else{
		iter=repositoryFileDS.find(key);
		if(iter != repositoryFileDS.end()){    			
			repoFileDSVector = iter->second;
			repoFileDSVector.push_back(value);
			repositoryFileDS[key]=repoFileDSVector;
		}else{    			
		    repoFileDSVector.push_back(value);
		    repositoryFileDS[key]=repoFileDSVector;
		}

	}

}


/*This function reads RepositoryFile content and populates repositoryFileDS*/
void populateRepositoryFileDS(){
	string lineString;
	ifstream infile;
	size_t found;

	infile.open (repositoryFile);
    if(getline(infile,lineString)){
    	pushRepositoryFileDS(lineString);
	    while ( infile ){
	        getline(infile,lineString);
	        pushRepositoryFileDS(lineString);
	    }
	}
	infile.close();  
} 


/*This function pushes data in mirrorListDS map*/
void pushMirrorListDS(string lineString){
	cout << lineString << endl;
	string key,value;
	size_t found;
	unordered_map<string,string>::iterator iter;

	found=lineString.find(":");
	key=lineString.substr(0,found);	
	value=lineString.substr(found+1);
	if(mirrorListDS.empty()){		
	    mirrorListDS[key]=value;	        
	}else{
		iter=mirrorListDS.find(key);
		if(iter != mirrorListDS.end()){			
			iter->second = value;			    
		}else{		    
		    mirrorListDS[key]=value;		        		    
		}
	}
}


/*This function reads Mirror List file content and populates mirrorListDS*/
void populateMirrorListDS(){
	string lineString;
	ifstream infile;
	size_t found;

	infile.open (mirrorList);
    if(getline(infile,lineString)){
    	pushMirrorListDS(lineString);
	    while ( infile ){
	        getline(infile,lineString);
	        pushMirrorListDS(lineString);
	    }
	}
	infile.close();  
} 

/*This function is to populate server parameter*/
void populateServerParam(int argc, char* argv[]){
	if(argc < 6){
		printf("Insufficient parameters. Exiting Server initialization");
		exit(EXIT_FAILURE);
	}else{
		serverIP = argv[1];
		//serverIP = strdup("10.1.134.72");
		serverPortNum = (unsigned int)strtol(argv[2],NULL,10);
		repositoryFile = argv[3];
		mirrorList = argv[4];
		rootFolder = argv[5];		
	}
}


void *flushDataToFileThread(void *threadid){
	flushDataToFile();
}


/*This function is executed by a thread continuously to flush the data to file from DS*/
void flushDataToFile(){
	string data;
	vector<string> repoFileDSVector;
	while(1){
		sleep(10);
		ofstream ofsRepo ("repo.txt", std::ofstream::out);
		for (map<string,vector<string> >::iterator it=repositoryFileDS.begin(); it!=repositoryFileDS.end(); ++it){
			repoFileDSVector = it->second;
			if(!repoFileDSVector.empty() && (!(it->first).empty())){
				for (vector<string>::iterator iter = repoFileDSVector.begin() ; iter != repoFileDSVector.end(); ++iter){
					
					ofsRepo << *iter << endl; 				
				}
			}
		}
		ofsRepo.close();
		ofstream ofsMirror ("list.txt", std::ofstream::out);
		for (unordered_map<string,string >::iterator it=mirrorListDS.begin(); it!=mirrorListDS.end(); ++it){
			if((!(it->first).empty()) && (!(it->second).empty())){			
				data= it->first;
				data =data +":" +(it->second);
				ofsMirror << data << endl; 			
			}
		}
		ofsMirror.close();
	}
	pthread_exit(NULL);
}


/*This function is for detecting mirrors which are no more active */
void *removeDeadMirrors(void *threadid){
	time_t now,mirrorLastPing;
	unordered_map<string,time_t > ::iterator iter;
	string mirrorAlias;
	size_t found;
	double seconds;

	while(1){
		sleep(30);
		if(!mirrorListDS.empty()){
			for (unordered_map<string,string >::iterator it=mirrorListDS.begin(); it!=mirrorListDS.end(); ++it){				
				if((!(it->first).empty()) && (!(it->second).empty())){
							
					mirrorAlias= it->first;
					
					if(!liveMirrorListDS.empty()){
						
						iter = liveMirrorListDS.find(mirrorAlias);
						
						if(iter != liveMirrorListDS.end()){
							
							mirrorLastPing =  iter->second;
							
							time(&now);
							
							if(difftime(now,mirrorLastPing) > 20){
								mirrorListDS.erase(it);
								liveMirrorListDS.erase(iter);
								
							}
						}else{
							
							mirrorListDS.erase(it);
							
						}
					}else{
						mirrorListDS.erase(it);
						
					}					
				}
			}
		}
	}
}

/*This function listens creates a Server socket
  For each incoming request, a seperate process is spawned*/
int main(int argc, char* argv[]){	

	pthread_t threadForDetectingLiveMirror;

	populateServerParam(argc, argv); //populate server parameter
	populateRepositoryFileDS(); //populate Repository File DS
	populateMirrorListDS(); //populate Repository File DS
	atexit(flushDataToFile);

	ofserver.open("crs.log", ofstream::out | ofstream::app);

	if(pthread_create(&threadForFlushData, NULL, flushDataToFileThread, NULL)) {

		fprintf(stderr, "Error creating thread\n");
		
	}

	if(pthread_create(&threadForDetectingLiveMirror, NULL, removeDeadMirrors, NULL)) {

		fprintf(stderr, "Error creating thread\n");		
	}


	threadCount=0;

	logMessage("Creating server socket");
	createServerSocket(serverIP,serverPortNum); //Create Server socket and bind it to port num: 9734
	suppressSIGCHILD(); //preventing the transformation of children into zombies

	while(1){
		
		printf("server waiting\n");	

		//clientSocketFileDesc= acceptClientConnection(); //Accept the connection

		/*if(fork() == 0) {
			handleClientRequest(); //ChildProcess - Client request handler

		}else{
			close(clientSocketFileDesc); //ParentProcess - Free ClientSocketFD
		}*/

		/*if(pthread_create(&threadForFlushData, NULL, handleClientRequest, NULL)) {

			fprintf(stderr, "Error creating thread\n");
		
		}*/
		handleClientRequest();
	}
	ofserver.close();
}