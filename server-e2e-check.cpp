#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <cassert>
#include <string>
#include "protocol.h"
#include "util.h"

using namespace C150NETWORK;

#define GRADING &std::cout

int main(int argc, char *argv[]) {
    GRADEME(argc, argv);
    assert(argc == 4);
   
    int network_nastiness = std::stoi(argv[1]);
    // ignoring argv[2] for now
    std::string target_dir = argv[3];
    C150NastyDgmSocket sock{network_nastiness};    

    bool e2e_success = false;
    Packet::Reference curr_ref = 0;

    // Open connection
    std::cerr << "OPENING CONNECTION\n";
    Packet::Client::Open open_pkt = 
        util::expect_x<Packet::Client::Open>(sock, curr_ref);
    std::cerr << "CONNECTION OPENNED\n";    

    for (uint32_t file_idx = 0; file_idx < open_pkt.file_count; ++file_idx) {
        // Connect
        ++curr_ref;
        Packet::Client::Connect connect_pkt = (file_idx == 0) 
            ? util::expect_x_ack_y<Packet::Client::Connect, Packet::Client::Open>(
                sock, curr_ref, curr_ref-1)
            : util::expect_x_ack_y<Packet::Client::Connect, Packet::Client::E2E_Check>(
                sock, curr_ref, curr_ref-1, e2e_success);


        uint32_t packet_count = connect_pkt.packet_count;
        Packet::Reference file_ref = connect_pkt.reference;        
        std::string filename = target_dir + connect_pkt.filename;

        std::ofstream outf{filename};
        
        // Receiving data
        uint32_t pkt_idx = 0;
        Packet::Client::Data data_pkt = 
            util::expect_x_ack_y<Packet::Client::Data, Packet::Client::Connect>(
                sock, file_ref + 1, file_ref);
        
        while (++pkt_idx < packet_count) {
            outf << data_pkt.data;
            
            data_pkt = 
                util::expect_x_ack_y<Packet::Client::Data, Packet::Client::Data>(
                    sock, (file_ref + pkt_idx + 1), (file_ref + pkt_idx));            
        }
        outf << data_pkt.data;
        outf.close();

        // E2E Check
        Packet::Client::E2E_Check e2e_pkt = 
            util::expect_x_ack_y<Packet::Client::E2E_Check, Packet::Client::Data>(
                sock, (file_ref + pkt_idx + 1), (file_ref + pkt_idx));
        
        std::string sha1 = util::get_SHA1_from_file(filename);
        e2e_success = sha1 == std::string(&e2e_pkt.sha1_file_checksum[0],
                                          &e2e_pkt.sha1_file_checksum[20]);

        curr_ref = file_ref + pkt_idx + 1;
        *GRADING << "File: " << filename << " end-to-end check " 
                 << (e2e_success ? "succeeded\n" : "failed\n");
    }


    // NEEDSWORK: Should we allow another OPEN packet?
    ++curr_ref;
    std::cerr << "WE ARE DONE. EXPECTING CLOSE PACKET with ref = " << curr_ref << '\n';
    Packet::Client::Close close_pkt =
        util::expect_x_ack_y<Packet::Client::Close, Packet::Client::E2E_Check>(
            sock, curr_ref, e2e_success);

    (void)close_pkt;
    
    std::cerr << "SENDING 15 ACKS AND QUITTING\n";

    // sending 15 acknowledgements that Client::Close arrived
    Packet::Server::Ack ack{curr_ref, true};
    std::string msg = util::serialize(ack);
    for (int i = 0; i < 15; ++i) {
        sock.write(msg.c_str(), msg.size()+1);
    }
}
