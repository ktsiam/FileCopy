# Makefile for COMP 117 End-to-End assignment
#
#    Copyright 2012 - Noah Mendelsohn
#
# The targets below are for samples that may be helpful, but:
#
#   YOUR MAIN JOB IN THIS ASSIGNMENT IS TO CREATE TWO 
#   NEW PROGRAMS: fileclient and fileserver
#
#   YOU WILL MODIFY THIS MAKEFILE TO BUILD THEM TOO
#
# Useful sample program targets:
#
#
#    nastyfiletest - demonstrates the c150nastyfile class that you 
#                    MUST use to read and write your files
#
#    sha1test -   sample code for computing SHA1 hash
#
#    makedatafile -  sometimes when testing file copy programs
#                    it's useful to have files in which it's
#                    relatively easy to spot changes.
#                    This program generates sample data files.
#
#  Maintenance targets:
#
#    Make sure these clean up and build your code too
#
#    clean       - clean out all compiled object and executable files
#    all         - (default target) make sure everything's compiled
#

# Do all C++ compies with g++
CPP = g++
CPPFLAGS = -g -std=c++17 -Wall -I$(C150LIB)

# Where the COMP 150 shared utilities live, including c150ids.a and userports.csv
# Note that environment variable COMP117 must be set for this to work!

C150LIB = $(COMP117)/files/c150Utils/
C150AR = $(C150LIB)c150ids.a

LDFLAGS = -lpthread -std=c++17
INCLUDES = $(C150LIB)c150dgmsocket.h $(C150LIB)c150nastydgmsocket.h $(C150LIB)c150network.h $(C150LIB)c150exceptions.h $(C150LIB)c150debug.h $(C150LIB)c150utility.h


all: client-e2e-check

client-e2e-check: client-e2e-check.cpp protocol.h protocol.cpp util.h $(C150AR) $(INCLUDES)
	$(CPP) -o client-e2e-check $(CPPFLAGS) client-e2e-check.cpp protocol.cpp util.cpp $(C150AR) -lssl -lcrypto

# all: nastyfiletest makedatafile sha1test e2eclient e2eserver fileCopyWriter fileCopyReader

# fileCopyWriter: fileCopyWriter.cpp $(C150AR) $(INCLUDES) fileCopyUtils.h
# 	$(CPP) $(LDFLAGS) -o fileCopyWriter $(CPPFLAGS) fileCopyWriter.cpp $(C150AR) -lssl -lcrypto

# fileCopyReader: fileCopyReader.cpp $(C150AR) $(INCLUDES) fileCopyUtils.h
# 	$(CPP) $(LDFLAGS) -o fileCopyReader $(CPPFLAGS) fileCopyReader.cpp $(C150AR) -lssl -lcrypto

# #
# # Build the nastyfiletest sample
# #
# nastyfiletest: nastyfiletest.cpp  $(C150AR) $(INCLUDES)
# 	$(CPP) -o nastyfiletest  $(CPPFLAGS) nastyfiletest.cpp $(C150AR)

# #
# # Build the sha1test
# #
# sha1test: sha1test.cpp
# 	$(CPP) -o sha1test sha1test.cpp -lssl -lcrypto

# e2eserver: e2eserver.cpp $(C150AR) $(INCLUDES)
# 	$(CPP) -o e2eserver $(CPPFLAGS) e2eserver.cpp -lssl -lcrypto $(C150AR)

# e2eclient: e2eclient.cpp $(C150AR) $(INCLUDES)
# 	$(CPP) -o e2eclient $(CPPFLAGS) e2eclient.cpp -lssl -lcrypto $(C150AR)

# #
# # Build the makedatafile 
# #
# makedatafile: makedatafile.cpp
# 	$(CPP) -o makedatafile makedatafile.cpp 

#
# To get any .o, compile the corresponding .cpp
#
%.o:%.cpp  $(INCLUDES)
	$(CPP) -c  $(CPPFLAGS) $< 


#
# Delete all compiled code in preparation
# for forcing complete rebuild#

clean:
	 rm -f copyFileWriter copyFileReader nastyfiletest sha1test makedatafile e2etest *.o 


