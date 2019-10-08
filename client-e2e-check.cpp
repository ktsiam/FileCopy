#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <openssl/sha.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <vector>
#include <filesystem>
#include "protocol.h"
#include "util.h"

using namespace C150NETWORK;

int main(int argc, char *argv[]) {
    
    assert(argc == 5);

    // ignoring argv[2], argv[3] for now
    std::string server_name = argv[1];
    std::string dir_name    = argv[4];
    
    std::vector<std::string> filenames;
    for (std::filesystem::directory_entry const& file :
             std::filesystem::directory_iterator(dir_name)) {
        if (file.is_directory()) continue;        
        filenames.push_back(file.path());
    }
    
    C150NastyDgmSocket sock{0};
    sock.setServerName(const_cast<char*>(server_name.c_str()));
    sock.turnOnTimeouts(3000);

    uint16_t ref_token = 0;
    char incomingMessage[512];
    for (const std::string &fname : filenames) {
        ++ref_token;
        Packet::Client::Connect connect_packet{ref_token, 0, fname.c_str()};
        std::string msg = util::serialize(connect_packet);

        int tries_left = 5;
        int readlen;
        while(1) {
            if (tries_left-- == 0) {
                throw C150NetworkException("No response after 5 tries");
            }
            sock.write(msg.c_str(), msg.size()+1);
            readlen = sock.read(incomingMessage, sizeof(incomingMessage));
            
            if (sock.timedout()) continue;
            std::optional<Packet::Server::Ack> inc_opt = 
                util::unserialize<Packet::Server::Ack>(incomingMessage, readlen);
            if (!inc_opt.has_value()) continue; // handles size, checksum and type errors
            Packet::Server::Ack inc = inc_opt.value();
            std::cerr << "Was file correct? : " << inc.success << std::endl;
            break;
        }
        
        // expecting Packet::Server::Close
    }
}
