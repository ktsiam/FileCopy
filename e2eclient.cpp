#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <filesystem>
#include <openssl/sha.h>
#include "c150nastydgmsocket.h"

using namespace C150NETWORK;

std::string get_SHA_from_file(std::string const& filename) {
    std::ifstream inf(filename);
    if (!inf) return "";
    
    std::ostringstream oss;
    oss << inf.rdbuf();
    std::string contents = oss.str();

    unsigned char sha1_hash[20];
    SHA1(reinterpret_cast<const unsigned char*>(contents.c_str()), 
         contents.size(), sha1_hash);
    
    return std::string(&sha1_hash[0], &sha1_hash[20]);
}


int main(int argc, char *argv[])
{
    assert(argc == 3 && "Server name & source directory required");
    std::string serverName{argv[1]};

    std::vector<std::string> filenames;
    for (auto&& file : std::filesystem::recursive_directory_iterator(argv[2])) {
        filenames.push_back(file.path());
    }
    
    std::vector<std::string> hashes;
    for (std::string const& f : filenames) {
        hashes.push_back(get_SHA_from_file(f));
    }

    C150NastyDgmSocket sock{4};
    sock.setServerName(const_cast<char*>(serverName.c_str()));
    sock.turnOnTimeouts(3000);

    char incomingMessage[512];
    for (std::size_t i = 0; i < filenames.size(); ++i) {
        std::string msg = hashes[i] + filenames[i];
        
        int tries_left = 10;
        int readlen;
        do {
            sock.write(msg.c_str(), msg.size()+1);
            readlen = 0;
            readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        } while (readlen <= 0 && sock.timedout() && --tries_left > 0);
        if (sock.timedout()) {
            throw C150NetworkException("No response after 10 tries");
        } 
    }
}
