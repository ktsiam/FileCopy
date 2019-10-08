#include <openssl/sha.h>
#include <string>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cstring>

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

enum Packet_tp : uint16_t { CLIENT_E2E, SERVER_ACK, SERVER_CLOSE };

struct Client_e2e_packet {
    Client_e2e_packet(uint16_t idempotency_token_,
                      const char* file_checksum_, 
                      const char* filename_) 
            : idempotency_token(idempotency_token_) {        
        std::strcpy(file_checksum, file_checksum_);
        std::strcpy(filename, filename_);
    }
    Packet_tp tp = Packet_tp::CLIENT_E2E;
    uint16_t idempotency_token;
    char file_checksum[20];
    char filename[256] = {0};
};

struct Server_ack {
    Server_ack(uint16_t idempotency_token_, bool success_) 
            : idempotency_token(idempotency_token_), success(success_) { }
    Packet_tp tp = Packet_tp::SERVER_ACK;
    uint16_t idempotency_token;
    bool success;
};

struct Server_close_connection {
    Packet_tp tp = Packet_tp::SERVER_CLOSE;
};

bool packet_ok(char *msg, int msg_len, Packet_tp pkt_tp, uint16_t idempotency_tok) {
    if (msg_len < (int)(sizeof(pkt_tp) + sizeof(idempotency_tok))) 
        return false;

    Packet_tp tp = *reinterpret_cast<Packet_tp*>(msg);
    uint16_t tok = *reinterpret_cast<uint16_t*>(msg + sizeof(pkt_tp));
    // NEEDSWORK -- packet checksum
    if (idempotency_tok == tok) {
        std::cerr << "SUCCESS" << std::endl;
    } else {
        std::cerr << "EXPETED : " << idempotency_tok << " but got : " << tok << std::endl;
    }
    return (pkt_tp == tp) && (idempotency_tok == tok);
}

std::string remove_path(std::string const& fname) {
    std::string cut_fn;
    for (char c : fname) {
        if (c == '/') {
            cut_fn = "";
        }
        cut_fn.push_back(c);
    }    
    return cut_fn;
}
