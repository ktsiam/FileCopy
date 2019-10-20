#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "c150grading.h"
#include <cassert>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <cmath>
#include "protocol.h"
#include "util.h"

using namespace C150NETWORK;

//#define GRADING &std::cout

const int DEFAULT_TIMEOUT = 1; // milliseconds

int main(int argc, char *argv[]) {
    GRADEME(argc, argv);
    assert(argc == 5);
    try{
    std::string server_name       = argv[1];
    const int network_nastiness   = std::stoi(argv[2]);
    const int file_nastiness      = std::stoi(argv[3]);
    const std::string dir_name    = argv[4];    
    
    std::vector<std::string> filenames;
    for (std::filesystem::directory_entry const& file :
             std::filesystem::directory_iterator(dir_name)) {
        if (file.is_directory()) continue;
        filenames.push_back(file.path());
    }

    C150NastyFile file_reader{file_nastiness};
    C150NastyDgmSocket sock{network_nastiness};

    sock.setServerName(&server_name.front());
    sock.turnOnTimeouts(DEFAULT_TIMEOUT);

    Packet::Reference ref_token = 0;

    // Open connection
    Packet::Client::Open open_pkt{ref_token, static_cast<uint32_t>(filenames.size())};
    util::send_to_server(sock, open_pkt);

    for (const std::string &fname : filenames) {
    	int attempt = 1;
	// -1 because of null terminator
        constexpr int data_pkt_capacity = Packet::Client::Data::DATA_SIZE - 1;
        bool e2e_success;

	
        do {
            std::string contents = util::get_contents(fname, file_reader);
            uint32_t packet_num = contents.size() / data_pkt_capacity + 
                (contents.size() % data_pkt_capacity == 0 ? 0 : 1);

            // Connect packet : transfers filename
            ++ref_token;
            Packet::Client::Connect connect_packet{ref_token, packet_num, fname.c_str()};
            util::send_to_server(sock, connect_packet);
        
            *GRADING << "File: " << fname << " beginning transmission, attempt " << attempt << endl;
            // Data transmission starts
            for (uint32_t i = 0; i < packet_num; ++i) {
                ++ref_token;
                uint32_t data_start = i * data_pkt_capacity;
                std::string data = contents.substr(data_start, data_pkt_capacity);
                assert(data.size() <= data_pkt_capacity);

                Packet::Client::Data data_packet{ref_token, i, data.c_str()};
                util::send_to_server(sock, data_packet);
            } 
	    *GRADING << "File: " << fname << " transmission complete, waiting for end-to-end check, attempt "
		<< attempt << endl;
            // E2E check packet : checks whether file has been transfered correctly.
            ++ref_token;        
            auto t1 = std::chrono::high_resolution_clock::now();
            std::string sha1 = util::get_SHA1_from_file(fname, file_reader);        
            auto t2 = std::chrono::high_resolution_clock::now();

            int duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1)
                .count();
        
            sock.turnOnTimeouts(std::max(duration/20, DEFAULT_TIMEOUT));

            Packet::Client::E2E_Check e2e_packet{ref_token, sha1.c_str()};
            e2e_success = util::send_to_server(sock, e2e_packet);

            sock.turnOnTimeouts(DEFAULT_TIMEOUT);
	    if (e2e_success) {
 	       	*GRADING << "File: " << fname << " end-to-end check succeeded, attempt " << attempt << endl;
	    	std::cout << "File: " << fname << " end-to-end check SUCCEEDS\n";
	    } else {
                *GRADING << "File: " << fname << " end-to-end check failed, attempt " << attempt << endl;
	    }

	    ++attempt;
        } while (!e2e_success && std::cerr << "File " << fname << " end-to-end check FAILS -- retrying\n");
    }
    
    
    // Close packet : informs server we're done
    ++ref_token;
    Packet::Client::Close close_packet{ref_token};
    util::send_to_server(sock, close_packet);

    }catch(C150NetworkException &e) {
        std::cerr << e.formattedExplanation() << std::endl;
    }
}
