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

const extern int DEFAULT_TIMEOUT;
namespace util {

std::string get_SHA1_from_file(std::string const& filename);
std::string remove_path(std::string const& fname);

void send_ack(C150NastyDgmSocket &sock, Packet::Reference ref, bool success = true);

template<typename T>
bool send_to_server(C150NastyDgmSocket &sock, const T &packet, int init_timeout = DEFAULT_TIMEOUT) {
    static char incomingMessage[512];
    static const int total_retries = 100;

    Packet::Reference ref_token = packet.reference;
    std::string msg = packet.serialize();
    
    int timeout = init_timeout;

    int readlen;
    sock.write(msg.c_str(), msg.size()+1);
    for (int i = 0; i < total_retries; ++i) {

        readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        if (sock.timedout()) { 
            std::cerr << "timedout\n";
            sock.write(msg.c_str(), msg.size()+1);
            timeout += DEFAULT_TIMEOUT;
            sock.turnOnTimeouts(timeout);
            continue;
        }
        
        auto inc_opt = Packet::Server::Ack::deserialize(incomingMessage, readlen);
        if (!inc_opt.has_value()) { std::cerr << "wrong type/checksum\n"; continue; }
        
        const Packet::Server::Ack &inc = inc_opt.value();
        if (inc.reference != ref_token) { std::cerr << "EXPECTED ref = " << ref_token << " but got " << inc.reference << std::endl; continue; }        

        sock.turnOnTimeouts(init_timeout);
        return inc.success;
    }
    throw C150NetworkException("no response after 100 retries");
}


template<class Curr_T, class Prev_T = Curr_T>
Curr_T expect_x_ack_y(C150NastyDgmSocket &sock, Packet::Reference curr_ref, 
                                                Packet::Reference prev_ref,
                                                bool success = true) {
    static char incomingMessage[512];
    while (1) {
        int readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        std::optional<Curr_T> curr_pkt_opt = Curr_T::deserialize(incomingMessage, readlen);
        
        if (curr_pkt_opt.has_value()) { // packet it correct type & intact
            std::cerr << "Packet ok! " << Curr_T::my_type() << "\n";
            const Curr_T &curr_pkt = curr_pkt_opt.value();
            if (curr_pkt.reference == curr_ref) {
                std::cerr << "Packet ref ok!\n";
                send_ack(sock, curr_ref);
                return curr_pkt;
            }
            std::cerr << "Packet ref BAD!\n";
            std::cerr << "Expected : " << curr_ref << " but got " << curr_pkt.reference << std::endl;
        }
        std::cerr << "WRONG PACKET TYPE RECEIVED. Got " << ((Packet::Base<Packet::Server::Ack>*)incomingMessage)->stored_type
                  << " but expected " << Curr_T::my_type() << std::endl;
        
        std::optional<Prev_T> prev_pkt_opt = Prev_T::deserialize(incomingMessage, readlen);
        
        if (prev_pkt_opt.has_value() && // previous type (can be same as expected)
            prev_pkt_opt.value().reference == prev_ref) {
            std::cerr << "SENDING ACK FOR OLD\n";
            send_ack(sock, prev_ref);
        }
        std::cerr << "NOT SENDING ACK for : " << ((Packet::Base<Packet::Server::Ack>*)incomingMessage)->stored_type << "(got), " << Prev_T::my_type() << "(want) & ref = " << ((Packet::Base<Packet::Server::Ack>*)incomingMessage)->reference << ", expected = " << prev_ref << '\n';
    }
    assert(false && "Unreachable");
}

template<class Pkt_T>
Pkt_T expect_x(C150NastyDgmSocket &sock, Packet::Reference curr_ref) {
    return expect_x_ack_y<Pkt_T, Pkt_T>(sock, curr_ref, curr_ref);
}

} // namespace util

#endif // PACKET_UTILS_H_
