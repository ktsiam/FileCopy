#include "fileCopyUtils.h"
#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <vector>
#include <string>

using namespace C150NETWORK;

int main(int argc, char *argv[]) {
    assert(argc == 4);

    std::string target_dir = argv[3];
    C150NastyDgmSocket sock{0};

    char incomingMessage[512];
    int readlen;
    while ((readlen = sock.read(incomingMessage, sizeof(incomingMessage)))) {
        if (readlen == 0) continue;

        Client_e2e_packet e2e_packet = 
            *reinterpret_cast<Client_e2e_packet*>(incomingMessage);
        
        if (e2e_packet.tp != Packet_tp::CLIENT_E2E) continue;

        uint16_t idempotency_tok = e2e_packet.idempotency_token;
        
        std::string sha1_dst = 
            get_SHA1_from_file(target_dir + '/' + std::string(e2e_packet.filename));
        std::string sha1_src{&e2e_packet.file_checksum[0], 
                             &e2e_packet.file_checksum[20]};
        bool success = sha1_dst == sha1_src;
        std::cout << "File " << (target_dir + std::string(e2e_packet.filename)) 
                  << " was transmitted " 
                  << (success ? "successfully" : "unsuccessfully") << std::endl;
        
        Server_ack ack{idempotency_tok, success};
        sock.turnOnTimeouts(3000);
        int tries_left = 5;
        do {
            if (tries_left-- == 0) {
                throw C150NetworkException("No response after 5 tries");
            }
            const char* ack_serial = reinterpret_cast<const char*>(&ack);
            std::string msg{&ack_serial[0], &ack_serial[sizeof(ack)]};
            
            sock.write(msg.c_str(), msg.size()+1);
            readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        } while (sock.timedout() || 
                 (!packet_ok(incomingMessage, readlen, Packet_tp::CLIENT_E2E, idempotency_tok+1) &&
                  !packet_ok(incomingMessage, readlen, Packet_tp::CLIENT_E2E, idempotency_tok+1))
        sock.turnOffTimeouts();
        // NEEDS WORK: this packet is not processed.
    }
    
}
