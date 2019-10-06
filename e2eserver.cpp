#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <openssl/sha.h>
#include "c150nastydgmsocket.h"
#include "c150grading.h"

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
    assert(argc == 2 && "Target directory required");

    std::unordered_map<std::string, std::string> file_to_sha;
    for (std::filesystem::directory_entry const& file : 
             std::filesystem::recursive_directory_iterator(argv[1])) {
        if (file.is_directory()) continue;

        std::string path = file.path();
        file_to_sha[path] = get_SHA_from_file(path);
    }

    C150NastyDgmSocket sock{4};
    
    char incomingMessage[512];
    while (1) {
        int readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        if (readlen <= 0) continue;
        incomingMessage[readlen] = '\0';
        std::string incoming{incomingMessage};
        cleanString(incoming);
        
        std::string hash{&incomingMessage[0], &incomingMessage[20]};
        std::string filename{&incomingMessage[20]};
        
        if (file_to_sha[filename] == hash) {
            sock.write("ok", sizeof("ok"));
            std::cout << "File " << filename 
                      << " was transferred successfully" << std::endl;
        }
    }
}
