/*
 * gc_server.hpp
 *
 *  Created on: Nov 12, 2014
 *      Author: ravitandon
 */

#ifndef GC_SERVER_HPP_
#define GC_SERVER_HPP_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <string.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <vector>
#include <sstream>
#include <sys/time.h>
#include <queue>
#include <stdlib.h>
#include <signal.h>
#define LOG_LEVEL_HIGH

using namespace std;

#define _PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define SHMSZ _PAGE_SIZE
#define MAX_PROCESSES 5
#define SERVER_SLEEP_TIME 5
#define SERVER_KEY 5678
// First Line -> List of Process Ids in order to identify (100 bytes)
// Second Line Onwards -> Process Id:Client Request:Server Response(100 bytes)

enum ClientSignal { Query, GCBusy, GCIdle, NoneC};
enum ServerSignal { Wait, GCStart, NoneS};

class Client_State{
private:
	pid_t processId;
	ClientSignal clientSignal;
	ServerSignal serverSignal;
	long int  clientSignalTime;
	long int  serverSignalTime;

public:
	Client_State(pid_t pid, ClientSignal cs, ServerSignal ss, long int cst, long int sst){
		processId = pid;
		clientSignal = cs;
		serverSignal = ss;
		clientSignalTime = cst;
		serverSignalTime = sst;
	}

	pid_t getProcessId() { return processId; }
	ClientSignal getClientSignal() { return clientSignal; }
	ServerSignal getServerSignal() { return serverSignal; }
	long int getClientSignalTime() { return clientSignalTime; }
	long int getServerSignalTime() { return serverSignalTime; }
	bool isClientWaiting(){ return (clientSignal == Query); }
	bool isClientBusy(){ return (clientSignal == GCBusy); }
	string toString(void);
};

class GC_SERVER {
	private:
    	key_t key;
    	string sem_name;
    	sem_t* mutex;
    	int processIds[MAX_PROCESSES];
    	int numberProcesses;
    	vector <Client_State*> clientStates;
    	queue <Client_State*> requestQueue;

	public:
    	static int shmid;
    	static char* shm;

    	GC_SERVER(){
    		numberProcesses = 0;
    		 /*
    		     * We'll name our shared memory segment
    		     * "5678".
    		     */
    		    key = 5678;
    		    sem_name = string("gc_sem");
    		    mutex = NULL;
    	}
    	void* writeToMemory(string);
    	bool runServer(void);
    	bool registerClient(pid_t);
    	bool deRegisterClient(pid_t);
    	void processSharedMemory(void);
    	vector<string> splitStrings(string);
    	bool resetProcessIds(vector<string>);
    	void processMessageLine(string str);
    	ClientSignal strToCS(string str);
    	ServerSignal strToSS(string str);
    	int strToInt(string str);
    	long int strToLongInt(string str);
    	void clearReqQueue(void);
    	void buildRequestQueue(void);
    	int getClientForGC(void);
    	void signalClients();
};


#endif /* GC_SERVER_HPP_ */
