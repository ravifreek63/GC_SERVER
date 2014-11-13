gcserver: gc_server.hpp gc_server.cpp
	g++ -std=c++11 -o gc_server gc_server.cpp gc_server.hpp
	
clean: 
	rm -rf gc_server 