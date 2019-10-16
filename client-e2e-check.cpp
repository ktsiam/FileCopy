#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <cassert>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>
#include "protocol.h"
#include "util.h"

using namespace C150NETWORK;

#define GRADING &std::cout

constexpr int DEFAULT_TIMEOUT = 1; // microseconds

int main(int argc, char *argv[]) {
    GRADEME(argc, argv);
    assert(argc == 5);

    // ignoring argv[3] for now
    std::string server_name = argv[1];
    int network_nastiness   = std::stoi(argv[2]);
    std::string dir_name    = argv[4];    
    
    std::vector<std::string> filenames;
    for (std::filesystem::directory_entry const& file :
             std::filesystem::directory_iterator(dir_name)) {
        if (file.is_directory()) continue;
        filenames.push_back(file.path());
    }

    C150NastyDgmSocket sock{network_nastiness};

    sock.setServerName(&server_name.front());
    sock.turnOnTimeouts(DEFAULT_TIMEOUT);

    Packet::Reference ref_token = 0;

    // Open connection
    Packet::Client::Open open_pkt{ref_token, static_cast<uint32_t>(filenames.size())};
    std::cerr << "TRYING TO OPEN CONNECTION\n";
    util::send_to_server(sock, open_pkt);
    std::cerr << "ACKNOWLEDGEMENT RECEIVED\n";

    for (const std::string &fname : filenames) {
        // -1 because of null terminator
        constexpr int data_pkt_capacity = Packet::Client::Data::DATA_SIZE - 1;

        std::ifstream inf{fname};
        std::string contents{std::istreambuf_iterator<char>(inf),
                             std::istreambuf_iterator<char>()};

        uint32_t packet_num = contents.size() / data_pkt_capacity + 
                             (contents.size() % data_pkt_capacity == 0 ? 0 : 1);
        
        // Connect packet : transfers filename
        ++ref_token;
        Packet::Client::Connect connect_packet{ref_token, packet_num, fname.c_str()};
        *GRADING << "File: " << fname << ", beginning transmission, attempt 0\n";
        util::send_to_server(sock, connect_packet);
        
        // Data transmission starts
        for (uint32_t i = 0; i < packet_num; ++i) {
            ++ref_token;
            uint32_t data_start = i * data_pkt_capacity;
            std::string data = contents.substr(data_start, data_pkt_capacity);
            assert(data.size() <= data_pkt_capacity);

            Packet::Client::Data data_packet{ref_token, i, data.c_str()};
            try {
                std::cerr << "sending data with ref " << ref_token << std::endl;
                util::send_to_server(sock, data_packet);
                std::cerr << "data sent!\n";
            } catch (C150NetworkException& e) { std::cerr<< e.formattedExplanation(); throw; }
        }

        // E2E check packet : checks whether file has been transfered correctly.
        ++ref_token;
        
        // giving the server time to calculate checksum
        auto t1 = std::chrono::high_resolution_clock::now();
        std::string sha1 = util::get_SHA1_from_file(fname);
        auto t2 = std::chrono::high_resolution_clock::now();
        uint64_t duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1)
                            .count();
        
        Packet::Client::E2E_Check e2e_packet{ref_token, sha1.c_str()};
        std::cerr << "SENDING E2E with ref = " << ref_token << std::endl;
        bool success = util::send_to_server(sock, e2e_packet, duration/5);
        
        sock.turnOnTimeouts(DEFAULT_TIMEOUT);

        *GRADING << "File: " << fname << " end-to-end check " 
                 << (success ? "succeeded" : "failed") << ", attempt 0\n";
    }
    
    
    // Close packet : informs server we're done
    ++ref_token;
    Packet::Client::Close close_packet{ref_token};
    std::cerr << "SENDING CLOSE with ref = " << ref_token << '\n';
    util::send_to_server(sock, close_packet);
    std::cerr << "DONE!!!\n";
}
