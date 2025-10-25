CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Archivos que se van a mantener (fuentes)
KEEP_FILES = *.cpp *.hpp *.py dataset.csv

# Archivos generados que se borrarán
GENERATED = build_sorted_index server_search client_search \
            index_sorted.idx .server.pid server.pid \
            /tmp/search_* unique_players.txt

all: build_index server client
	./build_sorted_index
	./server_search & echo $$! > .server.pid
	@sleep 1
	./client_search
	@kill `cat .server.pid` 2>/dev/null || true
	@$(MAKE) cleanup

build_index: build_sorted_index.cpp
	$(CXX) $(CXXFLAGS) -o build_sorted_index $<

server: server_search.cpp func_server.cpp func_server.hpp
	$(CXX) $(CXXFLAGS) -o server_search server_search.cpp func_server.cpp

client: client_search.cpp func_client.cpp func_client.hpp
	$(CXX) $(CXXFLAGS) -o client_search client_search.cpp func_client.cpp

cleanup:
	@echo "Limpiando archivos generados..."
	@rm -f build_sorted_index server_search client_search
	@rm -f index_sorted.idx .server.pid server.pid
	@echo "✓ Solo quedan archivos de código fuente (.cpp, .hpp, .py, .csv)"

clean:
	@rm -f $(GENERATED) *.o
	@echo "Limpieza completa"

.PHONY: all build_index server client cleanup clean