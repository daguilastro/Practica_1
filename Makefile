CXX = g++
CXXFLAGS = 

all: build_sorted_index server_search client_search

build_sorted_index: build_sorted_index.cpp
	$(CXX) $(CXXFLAGS) -o build_sorted_index build_sorted_index.cpp

server_search: server_search.cpp func_server.cpp func_server.hpp
	$(CXX) $(CXXFLAGS) -o server_search server_search.cpp func_server.cpp

client_search: client_search.cpp func_client.cpp func_client.hpp
	$(CXX) $(CXXFLAGS) -o client_search client_search.cpp func_client.cpp

run: all
	./build_sorted_index
	./server_search & echo $$! > /tmp/server.pid
	sleep 2
	./client_search
	kill `cat /tmp/server.pid` 2>/dev/null || true
	rm -f /tmp/server.pid /tmp/demo_unix_epoll.sock

clean:
	rm -f build_sorted_index server_search client_search index_sorted.idx
	rm -f /tmp/server.pid /tmp/demo_unix_epoll.sock

.PHONY: all run clean