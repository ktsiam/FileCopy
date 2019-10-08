#include "c150nastydgmsocket.h"
#include "c150grading.h"
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

    Packet::Reference ref_token = 0;
    for (const std::string &fname : filenames) {
        
        // Connect packet : transfers filename
        ++ref_token;
        Packet::Client::Connect connect_packet{ref_token, 0, fname.c_str()};
        util::send_to_server(sock, connect_packet);
        
        // E2E check packet : checks whether file has been transfered correctly.
        ++ref_token;
        std::string sha1 = util::get_SHA1_from_file(fname);
        
        Packet::Client::E2E_Check e2e_packet{ref_token, sha1.c_str()};
        bool success = util::send_to_server(sock, e2e_packet);
        std::cerr << (success ? "SUCCESSFULLY" : "UNSUCCESSFULLY")
                  << " transmitted " << fname << '\n';
    }
    
    // Close packet : informs server we're done
    ++ref_token;
    Packet::Client::Close close_packet{ref_token};
    util::send_to_server(sock, close_packet);
}