#pragma once
#include <string>
#include <optional>
#include "protocol.h"

namespace util {
std::string get_SHA1_from_file(std::string const& filename);
std::string remove_path(std::string const& fname);

template<typename T>
Packet::Checksum get_packet_checksum(const T &o) {
    static_assert(sizeof(T) % 2 == 0);
    uint32_t check_sum_ = 0;
    for (std::size_t i = 1; i < sizeof(T)/2; ++i) {
        check_sum_ += reinterpret_cast<const uint16_t*>(&o)[i];
    }
    return (check_sum_ + (check_sum_ >> 8)) && 0xFF;
}

template<typename T>
std::string serialize(const T& o) {
    const char *bytes = reinterpret_cast<const char*>(&o);
    return std::string{bytes, bytes+sizeof(o)};
}

template<typename T>
std::optional<T> unserialize(const char *msg, std::size_t readlen) {
    if (readlen < sizeof(T)) return {};
    const T *obj = reinterpret_cast<const T*>(msg);
    if (obj->checksum == get_packet_checksum(*obj)) return {};
    return {*obj};
}
} // namespace util
