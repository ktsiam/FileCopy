#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <openssl/sha.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <vector>
#include <filesystem>
#include "fileCopyUtils.h"

using namespace C150NETWORK;

int main(int argc, char *argv[]) {
    
    assert(argc == 5);
    std::string server_name = argv[1];
    std::string dir_name = argv[4];
    
    std::vector<std::string> filenames;
    for (std::filesystem::directory_entry const& file :
             std::filesystem::directory_iterator(dir_name)) {
        if (file.is_directory()) continue;
        
        filenames.push_back(file.path());
    }
    
    C150NastyDgmSocket sock{0};
    sock.setServerName(const_cast<char*>(server_name.c_str()));
    sock.turnOnTimeouts(3000);

    uint16_t idempotency_token = 0;
    char incomingMessage[512];
    for (std::string &fname : filenames) {
        ++idempotency_token;
        Client_e2e_packet e2e_packet{ idempotency_token, get_SHA1_from_file(fname).c_str(), remove_path(fname).c_str() };
        
        int tries_left = 5;
        int readlen;
        do {
            if (tries_left-- == 0) {
                throw C150NetworkException("No response after 5 tries");
            }            
            const char* e2e_serial = reinterpret_cast<const char*>(&e2e_packet);
            std::string msg{&e2e_serial[0], &e2e_serial[sizeof(e2e_packet)]};

            sock.write(msg.c_str(), msg.size()+1);
            readlen = sock.read(incomingMessage, sizeof(incomingMessage));            
        } while (sock.timedout() || 
                 !packet_ok(incomingMessage, readlen, SERVER_ACK, idempotency_token));    

        Server_ack ack = *reinterpret_cast<Server_ack*>(incomingMessage);
        if (ack.success) {
            std::cout << "SUCCESSFULLY COPIED FILE: " << fname << std::endl;
        } else {
            std::cout << "FAILED TO CORRECTLY COPY FILE: " << fname << std::endl;
        }
    }
}
