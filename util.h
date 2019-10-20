#ifndef PACKET_UTILS_H_
#define PACKET_UTILS_H_

#include <string>
#include <optional>
#include <type_traits>
#include <cassert>
#include "protocol.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"

using C150NETWORK::C150NastyDgmSocket;
using C150NETWORK::C150NetworkException;
using C150NETWORK::C150NastyFile;
using C150NETWORK::C150Exception;

namespace util {

// These functions attempt to work correctly, but are not failproof
std::string get_contents(const std::string &fname, C150NastyFile &f_reader);
void        set_contents(const std::string &fname, C150NastyFile &f_reader, 
                         const std::string &data);

// Uses above get_contents method 
std::string get_SHA1_from_file(const std::string &fname, C150NastyFile &f_reader);

// e.g. /path/to/text.txt --> text.txt 
std::string remove_path(const std::string &fname);

// sends acknowledgement of given reference number
void send_ack(C150NastyDgmSocket &sock, Packet::Reference ref, bool success = true);


// attempts sending a packet, retrying until a packet is received
template<typename Pkt_T>
bool send_to_server(C150NastyDgmSocket &sock, const Pkt_T &packet) {
    static char incomingMessage[512];

    Packet::Reference ref_token = packet.reference;
    std::string msg = packet.serialize(); 

    int readlen;
    sock.write(msg.c_str(), msg.size()+1);
    
    while (1) {
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
}


// keeps sending acknowledgements for packets of Reference `prev_ref` and type `Ack_T`
// until a valid packet of type `Exp_T` with Reference `curr_ref` arrives and is returned
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
                    // optimization : sending ack immediately
                    send_ack(sock, curr_ref); 
                }
                return curr_pkt;
            }
        }
        // fallthrough if packet doesn't match expected
        std::optional<Ack_T> prev_pkt_opt = Ack_T::deserialize(incomingMessage, readlen);
        
        if (prev_pkt_opt.has_value() && // previous type (can be same as expected)
            prev_pkt_opt.value().reference == prev_ref) {
            send_ack(sock, prev_ref, success_ack);
            continue;
        }
    }
}

} // namespace util

#endif // PACKET_UTILS_H_
