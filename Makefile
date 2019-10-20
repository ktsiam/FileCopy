CPP = g++
CPPFLAGS = -g -std=c++17 -O2 -Wall -I$(C150LIB)

# Where the COMP 150 shared utilities live, including c150ids.a and userports.csv
# Note that environment variable COMP117 must be set for this to work!

C150LIB = $(COMP117)/files/c150Utils/
C150AR = $(C150LIB)c150ids.a

LDFLAGS = -lpthread
INCLUDES = $(C150LIB)c150dgmsocket.h $(C150LIB)c150nastydgmsocket.h $(C150LIB)c150network.h $(C150LIB)c150exceptions.h $(C150LIB)c150debug.h $(C150LIB)c150utility.h protocol.h util.h


all: fileclient fileserver

fileclient: fileclient.o protocol.o util.o $(C150AR) $(INCLUDES)
	$(CPP) -o fileclient fileclient.o protocol.o util.o $(C150AR) -lssl -lcrypto

fileserver: fileserver.o protocol.o util.o $(C150AR) $(INCLUDES)
	$(CPP) -o fileserver fileserver.o protocol.o util.o $(C150AR) -lssl -lcrypto

protocol.o: protocol.cpp $(C150AR) $(INCLUDES)
	$(CPP) -c $(CPPFLAGS) protocol.cpp

util.o: util.cpp protocol.o $(C150AR) $(INCLUDES)
	$(CPP) -c $(CPPFLAGS) util.cpp

clean:
	rm -f fileclient fileserver *.o GRADELOG.*


