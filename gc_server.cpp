
#include "gc_server.hpp"

void* GC_SERVER::writeToMemory(string msg){
	return memcpy (shm, msg.c_str(), msg.size());
}

ClientSignal GC_SERVER::strToCS(string str){
	if(str.compare("Q") == 0)
		return Query;
	else if(str.compare("GCB") == 0)
		return GCBusy;
	else if(str.compare("I") == 0)
		return GCIdle;
	else
		return NoneC;
}

ServerSignal GC_SERVER::strToSS(string str){
	if(str.compare("W") == 0)
		return Wait;
	else if(str.compare("GCS") == 0)
		return GCStart;
	else
		return NoneS;
}

int GC_SERVER::strToInt(string str){
	istringstream buffer(str);
	int value;
	buffer >> value;
	return value;
}

long int GC_SERVER::strToLongInt(string str){
	istringstream buffer(str);
	long int value;
	buffer >> value;
	return value;
}

bool GC_SERVER::resetProcessIds(vector<string> process_Ids){
	numberProcesses=0;
	vector<string>::iterator it;
	for(it=process_Ids.begin(); it!=process_Ids.end(); it++){
		if(!registerClient(strToInt(*it))){
			cout << "Error in resetting processIds" << endl;
			return false;
		}
	}
	return true;
}

bool GC_SERVER::registerClient(pid_t processId){
	if(numberProcesses >= MAX_PROCESSES){
		cout << "Too Many Processes, Could Not Register Process id ::" << processId << endl;
		return false;
	}
	processIds[numberProcesses] = processId;
	numberProcesses++;
	return true;
}

void GC_SERVER::processMessageLine(string line){
	int processId;
	vector<string> strVec = splitStrings(line);
	vector<string>::iterator strVecIt;
	string tokenStr;
	int count=0;
	pid_t processId;
	ClientSignal clientSignal;
	ServerSignal serverSignal;
	time_t cst, sst;
	for(strVecIt=strVec.begin(); strVecIt!=strVec.end(); strVecIt++){
		tokenStr=*strVecIt;
		switch(count){
			case 1:
				processId = strToInt(tokenStr);
			break;

			case 2:
				clientSignal = strToCS(tokenStr);
			break;

			case 3:
				serverSignal = strToCS(tokenStr);
			break;

			case 4:
				cst = strToLongInt(tokenStr);
				break;

			case 5:
				sst = strToLongInt(tokenStr);
				break;

			default:
			 cout << "Default value reached for processMessageLine" << endl;
			return;
		}
		count++;
	}
	// Pushing the client state in the container
	clientStates.push_back(new Client_State(processId, clientSignal, serverSignal, cst, sst));
}

void GC_SERVER::clearReqQueue(){
   std::queue<Client_State*> empty;
   std::swap(requestQueue, empty);
}

int GC_SERVER::getClientForGC(){
	int clientIndex = 0, sClientIndex = 0;
	time_t stime = 0;
	Client_State* cs;
	vector <Client_State*>::iterator csi /*clientStatesIterator*/;
	for(csi=clientStates.begin();csi!=clientStates.end();csi++){
		cs = *csi;
		if(cs->isClientBusy()){ // If any one client is busy, all other clients wait
			return -1;
		}
		else if(cs->isClientWaiting()){
			if((cs->getClientSignalTime() < stime) || (stime == 0)){
				sClientIndex = clientIndex;
			}
		}
		clientIndex++;
	}
	return sClientIndex;
}

void GC_SERVER::buildRequestQueue(){
	clearReqQueue();
	Client_State* cs;
	vector <Client_State*>::iterator csi /*clientStatesIterator*/;
	for(csi=clientStates.begin();csi!=clientStates.end();csi++){
		cs = *csi;
		if(cs->isClientWaiting()){
			requestQueue.push(cs); // Currently we just push them so that they do not overlap
		}
	}
}

// Processing the shared memory
void GC_SERVER::processSharedMemory(){
	int lineCount = 0;
	std::string line;
	std::stringstream stringStream(std::string(shm));
	// Clearing the client state
	clientStates.clear();
	while(true){
		lineCount++;
		while(std::getline(stringStream, line)){
			// The first line is the the set of processes.
			if(lineCount == 1){
					vector<string> process_Ids = splitStrings(line);
					if(!resetProcessIds(process_Ids)){
						cout << "Error in processing shared memory state in processSharedMemory()" << endl;
					}
			} else { // Code for reading the subsequent lines
				std::getline(stringStream, line);
				processMessageLine(line);
			}
		}
	}
}

vector<string> GC_SERVER::splitStrings(string inputString){
	std::vector<string> wordVector;
	std::stringstream stringStream(inputString);
	std::string line;
	while(std::getline(stringStream, line))
	{
	    std::size_t prev = 0, pos;
	    while ((pos = line.find_first_of(":", prev)) != std::string::npos)
	    {
	        if (pos > prev)
	            wordVector.push_back(line.substr(prev, pos-prev));
	        prev = pos+1;
	    }
	    if (prev < line.length())
	        wordVector.push_back(line.substr(prev, std::string::npos));
	}
	return wordVector;
}

int findNthPositionOfCharAfter(string str, int n, char ch, int startIndex){
	int index = startIndex;
	while(n>0){
		index = str.find(ch, index+1);
		n--;
	}
	return index;
}

long int getCurrentTime(){
	struct timeval tp;
	gettimeofday(&tp, NULL);
	long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
	return ms;
}

string to_string(long int value){
	  std::ostringstream os ;
      //throw the value into the string stream
      os << value ;
      //convert the string stream into a string and return
      return os.str() ;
}

void GC_SERVER::signalClients(){
    int clientIndex = getClientForGC(), count = -1;
    string str(shm);
    int pos = 0, startPos, endPos, length;
    char newLine = '\n', delimiter =':';
    if(clientIndex > -1){
        startPos = findNthPositionOfCharAfter(str, clientIndex+1, newLine, -1);
        pos = findNthPositionOfCharAfter(str, 2, delimiter, startPos);
        str.erase(pos+1, 1);
        str.insert(pos+1, string("GCS"));
        pos = findNthPositionOfCharAfter(str, 4, delimiter, startPos);
        endPos = str.find(newLine, startPos+1);
        length = endPos-pos-1;
        str.erase(pos+1, length);
        str.insert(pos+1, to_string(getCurrentTime()));
    }
    memcpy(shm, str.c_str(), str.size());
}

bool GC_SERVER::runServer(){
		char c;
	    /*
	     * Create the segment.
	     */
	    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
	        perror("shmget");
	        return false;
	    }

	    /*
	     * Now we attach the segment to our data space.
	     */
	    if ((shm = (char *) shmat(shmid, NULL, 0)) == (char *) -1) {
	        perror("shmat");
	        return false;
	    }

	    /*
	     *  Creating a semaphore
	     */
	    int oflags = O_CREAT | O_EXCL;
	    mode_t mode = 0644;
	    unsigned int initialValue = 1;
	    mutex = sem_open(sem_name, oflags, mode,initialValue);
	    if(mutex == SEM_FAILED){
	    	perror("unable to create semaphore");
	    	sem_unlink(sem_name);
	    	exit(-1);
	    }

	    while(true){
	    	// Getting the lock
	    	sem_wait(mutex);
	    	// Checking the memory segment
	    	processSharedMemory();
	    	signalClients();
	    	// Releasing the lock
	    	sem_post(mutex);
	    	usleep(SERVER_SLEEP_TIME);
	    }

	    return true;
}

int main()
{
	GC_SERVER* gc_server = new GC_SERVER();
	gc_server->runServer();
	return 0;
}