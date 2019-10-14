#include "protocol.h"
#include "util.h"
#include <cstdio>
#include <cstring>
#include <string.h>
#include <algorithm>

using namespace Packet;

template<class Pkt_T>
Base<Pkt_T>::Base(Reference reference_, Type type_)
    : reference(reference_), type(type_) {}

template<class Pkt_T>
void Base<Pkt_T>::set_checksum() {
    auto check = util::get_packet_checksum(static_cast<const Pkt_T&>(*this));
    checksum = check;
}

template<class Pkt_T>
bool Base<Pkt_T>::is_corrupted() const {
    checksum = util::get_packet_checksum(static_cast<const Pkt_T&>(*this));
    Checksum expected = util::get_packet_checksum(static_cast<const Pkt_T&>(*this));
    bool is_corrupted = expected != checksum;
    return is_corrupted;
}

template<class Pkt_T>
bool Base<Pkt_T>::is_valid_type() const { return type == my_type(); }

#define MY_TYPE_SPECIALIZATION(PKT_TP, TYPEID) \
template<> Type Base<PKT_TP>::my_type() const { return TYPEID; }

MY_TYPE_SPECIALIZATION(Client::Connect,   Type::CLIENT_CONNECT)
MY_TYPE_SPECIALIZATION(Client::E2E_Check, Type::CLIENT_E2E_CHECK)
MY_TYPE_SPECIALIZATION(Client::Data,      Type::CLIENT_DATA)
MY_TYPE_SPECIALIZATION(Client::Close,     Type::CLIENT_CLOSE)
MY_TYPE_SPECIALIZATION(Server::Ack,       Type::SERVER_ACK)

#undef MY_TYPE_SPECIALIZATION


Client::Connect::Connect(Reference reference_, uint16_t packet_count_, 
                         const char *filename_)
    : Base(reference_, Type::CLIENT_CONNECT), packet_count(packet_count_) {
    std::strcpy(filename, util::remove_path(filename_).c_str());    
    set_checksum();
}

Client::Data::Data(Reference reference_, uint16_t idx_, const char *data_)
    : Base(reference_, Type::CLIENT_DATA), idx(idx_) {
    std::strcpy(data, data_);
    set_checksum();
}

Client::E2E_Check::E2E_Check(Reference reference_, const char *sha1_file_checksum_) 
    : Base(reference_, Type::CLIENT_E2E_CHECK) {
    std::copy(&sha1_file_checksum_[0], &sha1_file_checksum_[20],
              &sha1_file_checksum[0]);
    set_checksum();
}

Client::Close::Close(Reference reference_)
    : Base(reference_, Type::CLIENT_CLOSE) {
    set_checksum();
}

Server::Ack::Ack(Reference reference_, bool success_) 
    : Base(reference_, Type::SERVER_ACK), success(success_) {
    set_checksum();
}
