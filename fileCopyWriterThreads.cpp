#include "c150nastydgmsocket.h"
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <thread>
#include <unordered_map>
#include <vector>
#include <mutex>

using namespace C150NETWORK;

const int READER = 0;
const int WRITER = 1;

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

template<typename T>
uint16_t get_packet_checksum(const T *o) {
    uint32_t check_sum_ = 0;
    for (std::size_t i = 2; i < sizeof(T)-1; i += 2) {
        check_sum_ += reinterpret_cast<const uint16_t*>(o)[i];
    }
    return (check_sum_ + (check_sum_ >> 8)) && 0xFF;
}

struct Client_handshake_packet {
    Client_handshake_packet(uint16_t file_id_, uint16_t packets_num_, char* filename_) 
          : file_id(file_id_), packets_num(packets_num_) {
        std::strcpy(filename, filename_);
        packet_checksum = get_packet_checksum(this);
    }
    uint16_t packet_checksum;
    uint16_t file_id;
    uint16_t packets_num;
    char     filename[256] = {0};
};

struct Client_data_packet {
    uint16_t packet_checksum;
    uint16_t file_id;
    char     data[512 - sizeof(packet_checksum) - sizeof(file_id)];
};

struct Client_e2e_packet {
    Client_e2e_packet(const char* file_checksum_, const char* filename_) {        
        std::strcpy(file_checksum, file_checksum_);
        std::strcpy(filename, filename_);
        std::strcpy(file_checksum, get_SHA1_from_file(filename).c_str());
        packet_checksum = get_packet_checksum(this);
    }
    uint16_t packet_checksum;
    char     file_checksum[20];
    char     filename[256] = {0};
};

struct Server_acknowledgement {
    Server_acknowledgement(uint16_t file_id_, bool success_) : file_id(file_id_), success(success_) {
        packet_checksum = get_packet_checksum(this);
    }
    uint16_t packet_checksum;
    uint16_t file_id;
    bool success;
};

std::mutex write_mut;
void send_file(const uint16_t id, const char *filename, FILE* sock_reader, C150DgmSocket *sock_writer) {
    /* * * * * * *
     * NEEDSWORK *
     * * * * * * */
    
    /* Calculating final checksum */
    std::string file_checksum = get_SHA1_from_file(filename);
    Client_final_header final_header{file_checksum.c_str(), filename};
    
    int retries_left = 5;
    do {
        
        sock.write(msg.c_str(), msg.size()+1);
        readlen = 0;
        readlen = sock.read(incomingMessage, sizeof(incomingMessage));
    } while (readlen <= 0 && sock.timedout() && --tries_left > 0);
    if (sock.timedout()) {
        throw C150NetworkException("No response after 10 tries");
    }
}

int main(int argc, char *argv[]) {
    std::vector<std::thread> workers;
    std::unordered_map<uint16_t, FILE*> pipes(argc-1);
    C150NastyDgmSocket sock{0};
    sock.setServerName("117-01");
    sock.turnOnTimeouts(3000);

    /* Setting up threads */
    for (uint16_t i = 1; i < argc; ++i) {
        int fd[2]; 
        assert(pipe(fd) == 0);
        pipes[i] = fdopen(fd[WRITER], "w");

        workers.emplace_back(&send_file, i, argv[i], fdopen(fd[READER], "r"), &sock);
    }
    
    /* Set up input delegator */
    std::thread delegator([&workers, &pipes, &sock]() {
        while (1) {
            ssize_t readlen;
            char msg_buf[512];
            try {
                readlen = sock.read(msg_buf, sizeof(msg_buf)-1);
                if (readlen < (ssize_t)sizeof(Server_header)) {
                    continue;
                }
                msg_buf[readlen] = '\0';
                std::string incoming{msg_buf};
                cleanString(incoming);
            
                uint16_t id = 1;

                if (pipes.find(id) == pipes.end()) {
                    continue;
                }            
                fwrite(incoming.c_str(), 1, incoming.size()+1, pipes[id]);
            } catch (C150NetworkException& e) {
                std::cerr << e.formattedExplanation() << std::endl;
            }
        }
    });

    /* wait for workers to finish */
    for (std::thread &t : workers) 
        t.join();

    /* delegator destructor called */
}
