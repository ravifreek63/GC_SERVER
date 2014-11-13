gcserver: gc_server.cpp
	g++ -std=c++11 -o gc_server gc_server.cpp
	
clean: 
	rm -rf gc_server 