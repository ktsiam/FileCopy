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

template<typename Pkt_T>
bool send_to_server(C150NastyDgmSocket &sock, const Pkt_T &packet) {
    static char incomingMessage[512];
    static const int total_retries = 100;

    Packet::Reference ref_token = packet.reference;
    std::string msg = packet.serialize();    

    int readlen;
    sock.write(msg.c_str(), msg.size()+1);
    for (int i = 0; i < total_retries; ++i) {

        readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        if (sock.timedout()) { 
            sock.write(msg.c_str(), msg.size()+1);
            continue;
        }
        
        auto inc_opt = Packet::Server::Ack::deserialize(incomingMessage, readlen);
        if (!inc_opt.has_value()) continue;
        
        const Packet::Server::Ack &inc = inc_opt.value();
        if (inc.reference != ref_token) continue;

        return inc.success;
    }
    throw C150NetworkException("no response after 100 retries");
}


template<class Exp_T, class Ack_T = Exp_T>
Exp_T expect_x_ack_y(C150NastyDgmSocket &sock, Packet::Reference curr_ref, 
                                               Packet::Reference prev_ref,
                                               bool success_ack = true) {

    static char incomingMessage[512];
    while (1) {
        int readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        std::optional<Exp_T> curr_pkt_opt = Exp_T::deserialize(incomingMessage, readlen);
        
        if (curr_pkt_opt.has_value()) { // packet it correct type & intact
            const Exp_T &curr_pkt = curr_pkt_opt.value();
            if (curr_pkt.reference == curr_ref) {
                if constexpr (!std::is_same<Exp_T,Packet::Client::E2E_Check>::value) {
                    send_ack(sock, curr_ref); // optimization: send default ack immediately
                }
                return curr_pkt;
            }
        }

        std::optional<Ack_T> prev_pkt_opt = Ack_T::deserialize(incomingMessage, readlen);
        
        if (prev_pkt_opt.has_value() && // previous type (can be same as expected)
            prev_pkt_opt.value().reference == prev_ref) {
            send_ack(sock, prev_ref, success_ack);
            continue;
        }
    }
}

template<class Pkt_T>
Pkt_T expect_x(C150NastyDgmSocket &sock, Packet::Reference curr_ref) {
    return expect_x_ack_y<Pkt_T, Pkt_T>(sock, curr_ref, curr_ref);
}

} // namespace util

#endif // PACKET_UTILS_H_
