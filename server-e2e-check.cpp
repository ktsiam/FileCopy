#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <cassert>
#include <string>
#include "protocol.h"
#include "util.h"

using namespace C150NETWORK;

int main(int argc, char *argv[]) {
    GRADEME(argc, argv);
    assert(argc == 4);

    // ignoring argv[1], argv[2] for now
    std::string target_dir = argv[3];
    C150NastyDgmSocket sock{0};
    
    char incomingMessage[512];
    int readlen;
    std::string filename = "";
    std::unordered_map<Packet::Reference, std::string> ref_to_filename;
    while ((readlen = sock.read(incomingMessage, sizeof(incomingMessage)))) {
        // if message is CONNECT
        std::optional<Packet::Client::Connect> connect_pkt_opt =
            util::deserialize<Packet::Client::Connect>(incomingMessage, readlen,
                                                       Packet::CLIENT_CONNECT);
        
        if (connect_pkt_opt.has_value()) {
            const Packet::Client::Connect &connect_pkt = connect_pkt_opt.value();
            *GRADING << "File: " << (target_dir + connect_pkt.filename) 
                      << " starting to receive file\n";
            *GRADING << "File: " << (target_dir + connect_pkt.filename) 
                      << " received, beginning end-to-end check\n";

            // NEEDSWORK: E2E must have Reference of Connect-reference + 1 right now
            Packet::Reference ref = connect_pkt.reference;
            ref_to_filename[ref+1] = target_dir + connect_pkt.filename;
            
            // Sending acknowledgement
            Packet::Server::Ack ack{ref, true};
            std::string msg = util::serialize(ack);
            sock.write(msg.c_str(), msg.size()+1);
            continue;
        }
        
        //if message is E2E_CHECK
        std::optional<Packet::Client::E2E_Check> e2e_pkt_opt =
            util::deserialize<Packet::Client::E2E_Check>(incomingMessage, readlen,
                                                         Packet::CLIENT_E2E_CHECK);
        
        if (e2e_pkt_opt.has_value()) {
            const Packet::Client::E2E_Check &e2e_pkt = e2e_pkt_opt.value();
            Packet::Reference e2e_ref = e2e_pkt.reference;

            if (ref_to_filename.find(e2e_ref) == ref_to_filename.end()) continue;

            std::string sha1 = util::get_SHA1_from_file(ref_to_filename[e2e_ref]);
        
            bool success_e2e = sha1 == std::string(&e2e_pkt.sha1_file_checksum[0],
                                                   &e2e_pkt.sha1_file_checksum[20]);
        
            *GRADING << "File: " << ref_to_filename[e2e_ref] << " end-to-end check " 
                      << (success_e2e ? "succeeded\n" : "failed\n");
        
            // No acknowledgement retries are done. The client will send another E2E
            // packet if no acknowledgement arrives in a certain interval.
            Packet::Server::Ack ack{e2e_ref, success_e2e};
            std::string msg = util::serialize(ack);
            sock.write(msg.c_str(), msg.size()+1);
            continue;
        }

        // if message is CLOSE
        std::optional<Packet::Client::Close> close_pkt_opt =
            util::deserialize<Packet::Client::Close>(incomingMessage, readlen, 
                                                     Packet::CLIENT_CLOSE);        
        if (close_pkt_opt.has_value()) {
            Packet::Server::Ack ack{close_pkt_opt.value().reference, true};
            std::string msg = util::serialize(ack);
            for (int i = 0; i < 5; ++i) {
                // sending it 5 times to make sure it arrives
                sock.write(msg.c_str(), msg.size()+1);
            }
            return 0;
        }
    }
}
