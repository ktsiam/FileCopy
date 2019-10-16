#ifndef _FILECOPY_PROTOCOL_H_
#define _FILECOPY_PROTOCOL_H_
#include <cstdint>
#include <optional>

namespace Packet {

using Checksum  = uint16_t; // Makes sure packet is intact
using Reference = uint32_t; // Client: idempotency token
                            // Server: reference to Client packet

// Slightly redundant because of Reference, but useful in practice
enum Type : uint16_t { CLIENT_OPEN, CLIENT_CONNECT, CLIENT_DATA, CLIENT_E2E_CHECK, 
        CLIENT_CLOSE, SERVER_ACK };

// Make sure no padding for all Packets (for checksum)
#pragma pack(push, 1)


template<class Pkt_T> // CRTP pattern
struct Base {
    Reference reference;
    
    std::string serialize() const;
    static std::optional<Pkt_T> deserialize(char *msg, std::size_t len);

//protected:
    void     set_valid_checksum() const;
    Checksum get_valid_checksum() const;

    Base(Reference reference_);
    ~Base() = default;

//private:
    static Type my_type();
    bool is_corrupted()  const;
    bool is_valid_type() const;

    Type stored_type;
    mutable Checksum checksum; // modified & reverted during some methods
};

namespace Client {
    
    struct Open : Base<Open> {
        Open(Reference reference_, uint32_t file_count_);

        uint32_t file_count; // total number of files to be transferred
    };

    struct Connect : Base<Connect> {
        Connect(Reference reference_, uint32_t packet_count_, const char *filename_);

        uint32_t packet_count;    // total number of packets
        char filename[256] = {0}; // path is not included
    };

    struct Data : Base<Data> {
        Data(Reference reference_, uint32_t idx_, const char *data_);

        uint32_t idx; // index (< Connect::packet_count) of current data packe

        // -2 for null terminator in packet & retaining size multiple of 2
        static constexpr int DATA_SIZE = 512-sizeof(idx)-sizeof(Base)-2;
        char data[DATA_SIZE] = {0};
    };

    struct E2E_Check : Base<E2E_Check> {
        E2E_Check(Reference reference_, const char *sha1_file_checksum_);
        
        char sha1_file_checksum[20] = {0};
    };

    struct Close : Base<Close> {
        Close(Reference reference_);
    };

} // namespace Client

namespace Server {
struct Ack : Base<Ack> {
    Ack(Reference reference_, bool success_);

    // defaults to true. Allows information about failure of E2E check 
    uint16_t success; 
};
} // namespace Server

#pragma pack(pop) // No longer tightly pack structs
} // namespace Packet


// instantiating Packet::Base templates
template struct Packet::Base<Packet::Client::Open>;
template struct Packet::Base<Packet::Client::Connect>;
template struct Packet::Base<Packet::Client::Data>;
template struct Packet::Base<Packet::Client::E2E_Check>;
template struct Packet::Base<Packet::Client::Close>;
template struct Packet::Base<Packet::Server::Ack>;

#endif // _FILECOPY_PROTOCOL_H_
