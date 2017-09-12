/* Begin -includes*/
#include "serverCreationHelper.h"
/* End -includes*/

using namespace std;

/*Begin- Global variables*/
int clientSocketFileDesc;
char ch;
char cha='A';
char *serverIP;
unsigned int serverPortNum;
char * repositoryFile;
char * mirrorList;
char * rootFolder;
map<string,vector<string> > repositoryFileDS;
unordered_map<string,string > mirrorListDS;
/*End- Global variables*/

/*This function handles individual client requests*/
void handleClientRequest(){
	//read(clientSocketFileDesc, &ch, 1);
	//sleep(5);ch++;write(clientSocketFileDesc, &cha, 1);
	//close(clientSocketFileDesc);

	exit(0);	
}

/*This function pushes data in repositoryFileDS map*/
void pushRepositoryFileDS(string lineString){
	string key,value;
	size_t found;
	map<string,vector<string> >::iterator iter;
	vector<string> repoFileDSVector;
	found=lineString.find(":");
	key=lineString.substr(0,found);
	value=lineString.substr(found+1);
	if(repositoryFileDS.empty()){
		repoFileDSVector.push_back(value);
	    repositoryFileDS[key]=repoFileDSVector;
	    
	}else{
		iter=repositoryFileDS.find(key);
		if(iter != repositoryFileDS.end()){    			
			repoFileDSVector = iter->second;
			repoFileDSVector.push_back(value);
			iter->second = repoFileDSVector;
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
	string key,value;
	size_t found;
	unordered_map<string,string>::iterator iter;
	found=lineString.find(":");
	key=lineString.substr(0,found);
	value=lineString.substr(found+1);
	if(mirrorListDS.empty()){		
	    mirrorListDS[key]=value;
	    cout << mirrorListDS[key] << " ";	    
	}else{
		iter=mirrorListDS.find(key);
		if(iter != mirrorListDS.end()){			
			iter->second = value;
			cout << " hi " << mirrorListDS[key];	    
		}else{		    
		    mirrorListDS[key]=value;
		    cout << mirrorListDS[key];	    		    
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


/*This function listens creates a Server socket
  For each incoming request, a seperate process is spawned*/
int main(int argc, char* argv[]){	

	populateServerParam(argc, argv); //populate server parameter
	populateRepositoryFileDS(); //populate Repository File DS
	populateMirrorListDS(); //populate Repository File DS

	createServerSocket(serverIP,serverPortNum); //Create Server socket and bind it to port num: 9734
	suppressSIGCHILD(); //preventing the transformation of children into zombies

	while(1){
		
		printf("server waiting\n");
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