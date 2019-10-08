#include <cstdint>

namespace Packet {

using Checksum  = uint16_t; // Makes sure packet is intact
using Reference = uint16_t; // Client: idempotency token
                            // Server: reference to Client packet

// Slightly redundant because of Reference, but useful in practice
enum Type : uint16_t { CLIENT_CONNECT, CLIENT_DATA, CLIENT_E2E_CHECK,
        SERVER_ACK, SERVER_CLOSE };


// Make sure no padding for all Packets (for checksum)
#pragma pack(push, 1)

struct Base {
    Checksum  checksum;
    Reference reference;
    Type      type;
protected:
    Base(Reference reference_, Type type_);
};

namespace Client {
    struct Connect : Base {
        Connect(Reference reference_, uint16_t packet_count_, const char *filename_);

        uint16_t packet_count;    // total number of packets
        char filename[256] = {0}; // path is not included
    };

    struct Data : Base {
        Data(Reference reference_, uint16_t idx_, const char *data_);

        uint16_t idx; // index (< Connect::packet_count) of current data packet
        char data[512-sizeof(idx)-sizeof(Base)] = {0}; // null terminated
    };

    struct E2E_Check : Base {
        E2E_Check(Reference reference_, const char *sha1_file_checksum_);
        
        char sha1_file_checksum[20] = {0};
    };
} // namespace Client

namespace Server {
struct Ack : Base {
    Ack(Reference reference_, bool success_);

    // defaults to true. Allows information about failure of E2E check 
    uint16_t success; 
};

struct Close : Base {
    Close(Reference reference_);
};
} // namespace Server

#pragma pack(pop) // No longer tightly pack structs
} // namespace Packet
