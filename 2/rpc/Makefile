all:allo

CC=g++
CXXFLAGS=-std=c++17 -g
INCLUDE=./include

THREADPOOL=./threadPool
THREADPOOLTARGET=threadPool.o
JSONPARSER=./json
JSONPARSERTARGET=jsonParser.o
RPC=./rpc
RPCTARGET=rpcParser.o rpcServer.o rpcClient.o
TCP=./tcp
TCPTARGET=tcpServer.o

$(THREADPOOLTARGET):$(THREADPOOL)/threadPool.cpp
	$(CC) -I$(INCLUDE) $(CXXFLAGS) -c $^ -o $@
%.o:$(TCP)/%.cpp
	$(CC) -I$(INCLUDE) $(CXXFLAGS) -c $^ -o $@
$(JSONPARSERTARGET):$(JSONPARSER)/jsonParser.cpp
	$(CC) -I$(INCLUDE) $(CXXFLAGS) -c $^ -o $@
%.o:$(RPC)/%.cpp
	$(CC) -I$(INCLUDE) $(CXXFLAGS) -c $^ -o $@

allo:$(JSONPARSERTARGET) $(RPCTARGET) $(THREADPOOLTARGET) $(TCPTARGET)
clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;

