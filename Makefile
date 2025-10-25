CXX = g++

# Archivos que se van a mantener (fuentes)
KEEP_FILES = *.cpp *.hpp *.py dataset.csv

# Archivos generados que se borrarán
GENERATED = build_sorted_index server_search client_search \
            index_sorted.idx .server.pid server.pid \
            /tmp/search_* unique_players.txt

all: build_index server client
	./build_sorted_index
	./server_search &
	@sleep 1
	./client_search
	@pkill -F .server.pid || true
	@$(MAKE) cleanup

build_index: build_sorted_index.cpp
	$(CXX) -o build_sorted_index $<

server: server_search.cpp func_server.cpp func_server.hpp
	$(CXX) -o server_search server_search.cpp func_server.cpp

client: client_search.cpp func_client.cpp func_client.hpp
	$(CXX) -o client_search client_search.cpp func_client.cpp

cleanup:
	@rm -f build_sorted_index server_search client_search
	@rm -f index_sorted.idx .server.pid server.pid

clean:
	@rm -f $(GENERATED) *.o

.PHONY: all build_index server client cleanup clean