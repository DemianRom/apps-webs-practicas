CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -I src/common
LDFLAGS  :=
NCURSES  := -lncurses

JSON_URL := https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
JSON_DST := src/common/json.hpp

SERVER_SRCS := src/server/main_server.cpp \
               src/server/chat_server.cpp \
               src/server/sala.cpp \
               src/server/cliente.cpp

CLIENT_SRCS := src/client/main_client.cpp \
               src/client/chat_client.cpp \
               src/client/tui.cpp

SERVER_BIN := server
CLIENT_BIN := client

.PHONY: all server client clean deps

all: deps server client

server: deps $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) $(SERVER_SRCS) -o $(SERVER_BIN) $(LDFLAGS)

client: deps $(CLIENT_SRCS)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRCS) -o $(CLIENT_BIN) $(LDFLAGS) $(NCURSES)

deps: $(JSON_DST)

$(JSON_DST):
	@echo "Descargando nlohmann/json.hpp..."
	@curl -sL $(JSON_URL) -o $(JSON_DST) || wget -q $(JSON_URL) -O $(JSON_DST)
	@echo "json.hpp descargado en $(JSON_DST)"

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	rm -rf logs/*.log
