all:
	g++ -o build_sorted_index build_sorted_index.cpp
	g++ -o server_search server_search.cpp
	g++ -o client_search client_search.cpp

run: all
	./build_sorted_index
	./server_search & echo $$! > /tmp/server.pid
	sleep 2 
	./client_search
	kill `cat /tmp/server.pid` 2>/dev/null || true
	rm -f /tmp/server.pid /tmp/search_request /tmp/search_response

clean:
	rm -f build_sorted_index server_search client_search index_sorted.idx
	rm -f /tmp/search_request /tmp/search_response /tmp/server.pid
