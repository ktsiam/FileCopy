#ifndef PACKET_UTILS_H_
#define PACKET_UTILS_H_

#include <string>
#include <optional>
#include <type_traits>
#include <cassert>
#include "protocol.h"
#include "c150nastydgmsocket.h"

using C150NETWORK::C150NastyDgmSocket;
using C150NETWORK::C150NetworkException;

namespace util {

std::string get_SHA1_from_file(std::string const& filename);
std::string remove_path(std::string const& fname);

void send_ack(C150NastyDgmSocket &sock, Packet::Reference ref, bool success = true);


template<typename T>
std::string serialize(const T& o) {
    const char *bytes = reinterpret_cast<const char*>(&o);
    return std::string{bytes, bytes+sizeof(o)};
}

template<typename T>
std::optional<T> deserialize(char *msg, std::size_t readlen) {
    msg[readlen] = '\0'; // NEEDSWORK clean message
    if (readlen < sizeof(T)) return {};
    const T *obj = reinterpret_cast<const T*>(msg);
    if (!obj->is_valid_type() || obj->is_corrupted()) {
        std::cerr << obj->stored_type << '-' << obj->checksum << std::endl;
        std::cerr << "EXPECTED CHECKSUM : " << obj->get_valid_checksum() << std::endl;
        std::cerr << "OBJECT IS CORRUPT: " << obj->is_corrupted() << std::endl;
        if constexpr (std::is_same<T, Packet::Client::Open>::value) {
                std::cerr << "file_count = " << obj->file_count << std::endl;
            }
        return {};
    }
    return {*obj};
}

template<typename T>
bool send_to_server(C150NastyDgmSocket &sock, const T &packet) {
    static char incomingMessage[512];

    Packet::Reference ref_token = packet.reference;
    std::string msg = util::serialize(packet);

    int readlen;
    int tries_left = 100;
    while(1) {
        if (--tries_left <= 0) {
            std::cerr<<"tries left = " << tries_left << '\n';
            throw C150NetworkException("No response after 30 tries");
        }
        sock.write(msg.c_str(), msg.size()+1);
        readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        if (sock.timedout()) { std::cerr << "timeout\n"; continue; }

        std::optional<Packet::Server::Ack> inc_opt =
            util::deserialize<Packet::Server::Ack>(incomingMessage, readlen);
        if (!inc_opt.has_value()) { std::cerr << "wrong type/checksum\n"; continue; } // handles size, checksum and type errors

        const Packet::Server::Ack &inc = inc_opt.value();
        if (inc.reference != ref_token) { std::cerr << "EXPECTED ref = " << ref_token << " but got " << inc.reference << std::endl; continue; }
        return inc.success;
    }
}


template<class Curr_T, class Prev_T = Curr_T>
Curr_T expect_x_ack_y(C150NastyDgmSocket &sock, Packet::Reference curr_ref, 
                                                Packet::Reference prev_ref,
                                                bool success = true) {
    static char incomingMessage[512];
    while (1) {
        int readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        std::optional<Curr_T> curr_pkt_opt =
            util::deserialize<Curr_T>(incomingMessage, readlen);
        
        if (curr_pkt_opt.has_value()) { // packet it correct type & intact
            std::cerr << "Packet ok!\n";
            const Curr_T &curr_pkt = curr_pkt_opt.value();
            if (curr_pkt.reference == curr_ref) {
                std::cerr << "Packet ref ok!\n";
                send_ack(sock, curr_ref);
                return curr_pkt;
            }
            std::cerr << "Packet ref BAD!\n";
            std::cerr << "Expected : " << curr_ref << " but got " << curr_pkt.reference << std::endl;
        }
        std::cerr << "WRONG PACKET TYPE RECEIVED\n";
        
        std::optional<Prev_T> prev_pkt_opt = 
            util::deserialize<Prev_T>(incomingMessage, readlen);
        
        if (prev_pkt_opt.has_value() && // previous type (can be same as expected)
            prev_pkt_opt.value().reference == prev_ref) {
            send_ack(sock, prev_ref);
        }
    }
    assert(false && "Unreachable");
}

template<class Pkt_T>
Pkt_T expect_x(C150NastyDgmSocket &sock, Packet::Reference curr_ref) {
    return expect_x_ack_y<Pkt_T, Pkt_T>(sock, curr_ref, curr_ref);
}

} // namespace util

#endif // PACKET_UTILS_H_
