all:rppc clientA clientB

CC=g++
CXXFLAGS=-std=c++17 -g

INCLUDE=./rpc/include

mrpcTARGET=./rpc/jsonParser.o ./rpc/threadPool.o ./rpc/rpcParser.o ./rpc/rpcServer.o ./rpc/rpcClient.o ./rpc/tcpServer.o

PAILLIERTARGET=./paillierGmpImp.o

MNISTTARGET=./mnist.o

rppc:
	cd rpc&&make
$(PAILLIERTARGET):./paillierGmpImp.cpp
	$(CC) -I$(INCLUDE) $(CXXFLAGS) -c $^ -o $@
$(MNISTTARGET):./mnist.cpp
	$(CC) -I$(INCLUDE) $(CXXFLAGS) -c $^ -o $@
clientA.o:clientA.cpp
	$(CC) -I$(INCLUDE) $(CXXFLAGS) -c $^ -o $@
clientA:clientA.o $(PAILLIERTARGET) $(MNISTTARGET)
	$(CC) -o $@ $(mrpcTARGET) $^ -lssl -lcrypto -lpthread -lgmp -lgmpxx
clientB.o:clientB.cpp
	$(CC) -I$(INCLUDE) $(CXXFLAGS) -c $^ -o $@
clientB:clientB.o $(PAILLIERTARGET) $(MNISTTARGET)
	$(CC) -o $@ $(mrpcTARGET) $^ -lssl -lcrypto -lpthread -lgmp -lgmpxx
clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	rm clientA
	rm clientB
	rm server