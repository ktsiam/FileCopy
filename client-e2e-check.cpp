#include "c150nastydgmsocket.h"
#include "c150grading.h"
#include <cassert>
#include <vector>
#include <filesystem>
#include "protocol.h"
#include "util.h"

using namespace C150NETWORK;

#define GRADING &std::cout

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
    sock.setServerName(server_name.data());
    sock.turnOnTimeouts(30);

    Packet::Reference ref_token = 0;
    for (const std::string &fname : filenames) {
        
        // Connect packet : transfers filename
        ++ref_token;
        Packet::Client::Connect connect_packet{ref_token, 0, fname.c_str()};
        *GRADING << "File: " << fname << ", beginning transmission, attempt 0\n";
        util::send_to_server(sock, connect_packet);
        *GRADING << "File: " << fname << " transmission complete, "
                 << "waiting for end-to-end check, attempt 0\n";

        
        // E2E check packet : checks whether file has been transfered correctly.
        ++ref_token;
        std::string sha1 = util::get_SHA1_from_file(fname);
        
        Packet::Client::E2E_Check e2e_packet{ref_token, sha1.c_str()};
        bool success = util::send_to_server(sock, e2e_packet);
        
        *GRADING << "File: " << fname << " end-to-end check " 
                 << (success ? "succeeded" : "failed") << ", attempt 0\n";
    }
    
    // Close packet : informs server we're done
    ++ref_token;
    Packet::Client::Close close_packet{ref_token};
    util::send_to_server(sock, close_packet);
}
