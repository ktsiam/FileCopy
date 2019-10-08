#include "protocol.h"
#include "util.h"
#include <cstdio>
#include <cstring>
#include <string.h>
#include <algorithm>

using namespace Packet;

Base::Base(Reference reference_, Type type_)
    : reference(reference_), type(type_) {
    checksum = util::get_packet_checksum(this);
}

Client::Connect::Connect(Reference reference_, uint16_t packet_count_, 
                         const char *filename_)
    : Base(reference_, Type::CLIENT_CONNECT), packet_count(packet_count_) {
    std::strcpy(filename, util::remove_path(filename_).c_str());    
    checksum = util::get_packet_checksum(this);
}

Client::Data::Data(Reference reference_, uint16_t idx_, const char *data_)
    : Base(reference_, Type::CLIENT_DATA), idx(idx_) {
    std::strcpy(data, data_);
    checksum = util::get_packet_checksum(this);
}

Client::E2E_Check::E2E_Check(Reference reference_, const char *sha1_file_checksum_) 
    : Base(reference_, Type::CLIENT_E2E_CHECK) {
    // NEEDSWORK -- writes null-terminator out of struct
    std::copy(&sha1_file_checksum_[0], &sha1_file_checksum_[20],
              &sha1_file_checksum[0]);
    checksum = util::get_packet_checksum(this);
}

Client::Close::Close(Reference reference_)
    : Base(reference_, Type::CLIENT_CLOSE) {
    checksum = util::get_packet_checksum(this);
}

Server::Ack::Ack(Reference reference_, bool success_) 
    : Base(reference_, Type::SERVER_ACK), success(success_) {
    checksum = util::get_packet_checksum(this);
}
