#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <openssl/sha.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <vector>
#include <filesystem>

using namespace C150NETWORK;

std::string get_SHA1_from_file(std::string const& filename) {
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

enum Packet_tp : uint16_t { CLIENT_E2E, SERVER_ACK };

struct Client_e2e_packet {
    Client_e2e_packet(uint16_t idempotency_token,
                      const char* file_checksum_, 
                      const char* filename_) {        
        std::strcpy(file_checksum, file_checksum_);
        std::strcpy(filename, filename_);
    }
    Packet_tp tp = Packet_tp::CLIENT_E2E;
    uint16_t idempotency_token;
    char file_checksum[20];
    char filename[256] = {0};
};

struct Server_ack {
    Server_ack(uint16_t file_id_, bool success_) 
        : file_id(file_id_), success(success_) { }
    Packet_tp tp;
    uint16_t idempotency_token;
    uint16_t file_id;
    bool success;
};

bool packet_ok(char *msg, int msg_len, Packet_tp pkt_tp, int16_t idempotency_tok) {
    if (msg_len < (int)(sizeof(pkt_tp) + sizeof(idempotency_tok))) 
        return false;

    Packet_tp tp = *reinterpret_cast<Packet_tp*>(msg);
    uint16_t tok = *reinterpret_cast<uint16_t*>(msg + sizeof(pkt_tp));
    // NEEDSWORK -- packet checksum
    return (pkt_tp == tp) && (idempotency_tok == tok);
}

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
    for (std::string const& fname : filenames) {
        ++idempotency_token;
        Client_e2e_packet e2e_packet{ idempotency_token, get_SHA1_from_file(fname).c_str(), fname.c_str() };
        
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
