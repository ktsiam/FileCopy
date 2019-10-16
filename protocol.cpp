#include "protocol.h"
#include "util.h"
#include <cstdio>
#include <cstring>
#include <string.h>
#include <algorithm>

using namespace Packet;

template<class Pkt_T>
Base<Pkt_T>::Base(Reference reference_)
    : reference(reference_), stored_type(my_type()) {}

template<class Pkt_T>
std::string Base<Pkt_T>::serialize() const {
    const char *bytes = reinterpret_cast<const char*>(this);
    return std::string{bytes, bytes+sizeof(Pkt_T)};
}

template<class Pkt_T>
std::optional<Pkt_T> Base<Pkt_T>::deserialize(char *msg, std::size_t len) {
    msg[len] = '\0'; // NEEDSWORK clean message
    if (len < sizeof(Pkt_T)) return {};
    const Pkt_T *obj = reinterpret_cast<const Pkt_T*>(msg);
    if (!obj->is_valid_type() || obj->is_corrupted()) {
        std::cerr << obj->stored_type << '-' << obj->checksum << std::endl;
        std::cerr << "EXPECTED CHECKSUM : " << obj->get_valid_checksum() << std::endl;
        std::cerr << "OBJECT IS CORRUPT: " << obj->is_corrupted() << std::endl;
        if constexpr (std::is_same<Pkt_T,Packet::Client::Open>::value) {
                std::cerr << "file_count = " << obj->file_count << std::endl;
            }
        return {};
    }
    return {*obj};
}

template<class Pkt_T>
void Base<Pkt_T>::set_valid_checksum() const {
    checksum = 0;
    // temporary side effects on checksum
    auto check = get_valid_checksum(); 
    checksum = check;
}

template<class Pkt_T>
Checksum Base<Pkt_T>::get_valid_checksum() const {
    static_assert(sizeof(Pkt_T) % 2 == 0);

    // removing checksum from calculation
    Checksum old_checksum = checksum; 
    checksum = 0;

    uint32_t check_sum_ = 0;
    for (std::size_t i = 0; i < sizeof(Pkt_T)/2; ++i) {
        check_sum_ += reinterpret_cast<const uint16_t*>(this)[i];
    }
    checksum = old_checksum; // re-adding old checksum
    return (check_sum_ + (check_sum_ >> 8)) & 0xFF;
}

template<class Pkt_T>
bool Base<Pkt_T>::is_corrupted() const {
    Checksum expected = get_valid_checksum();
    return expected != checksum;
}

template<class Pkt_T>
bool Base<Pkt_T>::is_valid_type() const { 
    return stored_type == my_type(); 
}

// mapping from Packet type to enum Packet::Type
#define MY_TYPE_SPECIALIZATION(PKT_TP, TYPEID) \
template <> Type Base<PKT_TP>::my_type() { return TYPEID; }

MY_TYPE_SPECIALIZATION(Client::Open,      Type::CLIENT_OPEN)
MY_TYPE_SPECIALIZATION(Client::Connect,   Type::CLIENT_CONNECT)
MY_TYPE_SPECIALIZATION(Client::E2E_Check, Type::CLIENT_E2E_CHECK)
MY_TYPE_SPECIALIZATION(Client::Data,      Type::CLIENT_DATA)
MY_TYPE_SPECIALIZATION(Client::Close,     Type::CLIENT_CLOSE)
MY_TYPE_SPECIALIZATION(Server::Ack,       Type::SERVER_ACK)

#undef MY_TYPE_SPECIALIZATION




/* Specialized Packets */
Client::Open::Open(Reference reference_, uint32_t file_count_)
    : Base(reference_), file_count(file_count_) {
    set_valid_checksum();
}

Client::Connect::Connect(Reference reference_, uint32_t packet_count_, 
                         const char *filename_)
    : Base(reference_), packet_count(packet_count_) {
    std::strcpy(filename, util::remove_path(filename_).c_str());    
    set_valid_checksum();
}

Client::Data::Data(Reference reference_, uint32_t idx_, const char *data_)
    : Base(reference_), idx(idx_) {
    std::strcpy(data, data_);
    set_valid_checksum();
}

Client::E2E_Check::E2E_Check(Reference reference_, const char *sha1_file_checksum_) 
    : Base(reference_) {
    std::copy(&sha1_file_checksum_[0], &sha1_file_checksum_[20],
              &sha1_file_checksum[0]);
    set_valid_checksum();
}

Client::Close::Close(Reference reference_)
    : Base(reference_) {
    set_valid_checksum();
}

Server::Ack::Ack(Reference reference_, bool success_) 
    : Base(reference_), success(success_) {
    set_valid_checksum();
}
