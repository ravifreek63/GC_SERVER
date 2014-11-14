all: gcserver gclient

gcserver: gc_server.cpp
	g++ -lpthread -std=c++11 -o gc_server gc_server.cpp
	
gcclient: shm_client.cpp
	g++ -lpthread -o shm_client shm_client.cpp 
clean: 
	rm -rf gc_server 