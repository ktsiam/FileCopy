#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <cassert>
#include <string>
#include "protocol.h"
#include "util.h"

using namespace C150NETWORK;

//#define GRADING &std::cout

int main(int argc, char *argv[]) {
    GRADEME(argc, argv);
    assert(argc == 4);
   
    int network_nastiness = std::stoi(argv[1]);
    int file_nastiness    = std::stoi(argv[2]);
    std::string target_dir = argv[3];

    C150NastyDgmSocket sock{network_nastiness};    
    C150NastyFile file_reader{file_nastiness};

    bool e2e_success = false;
    Packet::Reference curr_ref = 0;

    while (1) { // server main loop

        // Start new directory transfer (potentially acknowledge prev connection close)
        Packet::Client::Open open_pkt = 
            util::expect_x_ack_y<Packet::Client::Open, Packet::Client::Close>(
                sock, 0, curr_ref);
        
        curr_ref = 0;
        bool just_openned_connection = true;

        for (uint32_t file_idx = 0; file_idx < open_pkt.file_count; ++file_idx) {
            // Connect
            ++curr_ref;
            Packet::Client::Connect connect_pkt = just_openned_connection
                ? util::expect_x_ack_y<Packet::Client::Connect, Packet::Client::Open>(
                    sock, curr_ref, curr_ref-1)
                : util::expect_x_ack_y<Packet::Client::Connect, Packet::Client::E2E_Check>(
                    sock, curr_ref, curr_ref-1, e2e_success);

            just_openned_connection = false;

            uint32_t packet_count = connect_pkt.packet_count;
            Packet::Reference file_ref = connect_pkt.reference;        
            std::string filename = target_dir + connect_pkt.filename;

            // Receiving data
            uint32_t pkt_idx = 0;
            Packet::Client::Data data_pkt = 
                util::expect_x_ack_y<Packet::Client::Data, Packet::Client::Connect>(
                    sock, file_ref + 1, file_ref);
        
            std::string data;
            while (++pkt_idx < packet_count) {
                data += data_pkt.data;
            
                data_pkt = 
                    util::expect_x_ack_y<Packet::Client::Data, Packet::Client::Data>(
                        sock, (file_ref + pkt_idx + 1), (file_ref + pkt_idx));            
            }
            data += data_pkt.data;
            util::set_contents(filename + ".TMP", file_reader, data);

            // E2E Check
            Packet::Client::E2E_Check e2e_pkt = 
                util::expect_x_ack_y<Packet::Client::E2E_Check, Packet::Client::Data>(
                    sock, (file_ref + pkt_idx + 1), (file_ref + pkt_idx));
        
            std::string sha1 = util::get_SHA1_from_file(filename + ".TMP", file_reader);
            e2e_success = sha1 == std::string(&e2e_pkt.sha1_file_checksum[0],
                                              &e2e_pkt.sha1_file_checksum[20]);
            curr_ref = file_ref + pkt_idx + 1;
            util::send_ack(sock, curr_ref, e2e_success); 
            if (e2e_success == false) {
                --file_idx; // starting loop over for same file
            } else {
                std::rename((filename + ".TMP").c_str(), filename.c_str());
            }
        }

        ++curr_ref;
        Packet::Client::Close close_pkt =
            util::expect_x_ack_y<Packet::Client::Close, Packet::Client::E2E_Check>(
                sock, curr_ref, curr_ref-1, e2e_success);
        (void)close_pkt;
    }
}
