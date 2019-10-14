#ifndef PACKET_UTILS_H_
#define PACKET_UTILS_H_

#include <string>
#include <optional>
#include <type_traits>
#include "protocol.h"
#include "c150nastydgmsocket.h"

using C150NETWORK::C150NastyDgmSocket;
using C150NETWORK::C150NetworkException;

namespace util {
std::string get_SHA1_from_file(std::string const& filename);
std::string remove_path(std::string const& fname);

template<typename T>
Packet::Checksum get_packet_checksum(T &o) {
    static_assert(sizeof(T) % 2 == 0);
    
    Packet::Checksum old_checksum = o.checksum;
    o.checksum = 0;
    uint32_t check_sum_ = 0;
    for (std::size_t i = 1; i < sizeof(T)/2; ++i) {
        check_sum_ += reinterpret_cast<const uint16_t*>(&o)[i];
    }
    o.checksum = old_checksum;
    return (check_sum_ + (check_sum_ >> 8)) & 0xFF;
}

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
    if (!obj->is_valid_type()) return {};
    // NEEDSWORK: FIX CHECKSUM
    if (obj->is_corrupted()) { return {}; }
    return {*obj};
}

template<typename T>
bool send_to_server(C150NastyDgmSocket &sock, const T &packet) {
    static char incomingMessage[512];

    Packet::Reference ref_token = packet.reference;
    std::string msg = util::serialize(packet);

    int readlen;
    int tries_left = 10;
    while(1) {
        if (tries_left-- == 0) {
            throw C150NetworkException("No response after 10 tries");
        }
        sock.write(msg.c_str(), msg.size()+1);
        readlen = sock.read(incomingMessage, sizeof(incomingMessage));
        if (sock.timedout()) continue;

        std::optional<Packet::Server::Ack> inc_opt =
            util::deserialize<Packet::Server::Ack>(incomingMessage, readlen);
        if (!inc_opt.has_value()) continue; // handles size, checksum and type errors

        const Packet::Server::Ack &inc = inc_opt.value();
        if (inc.reference != ref_token) continue;
        return inc.success;
    }
}
} // namespace util

#endif // PACKET_UTILS_H_
