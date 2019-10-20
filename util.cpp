#include <openssl/sha.h>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <algorithm>
#include <map>
#include "util.h"
#include "c150nastyfile.h" 

// used to extract median of characters from `NUM_TRIES` different file reads
static const int NUM_TRIES = 5;

static char median(const std::vector<std::string> &data, int idx)
{
    std::map<char, int> map;
    std::vector<char> v;
    for (int i = 0; i < NUM_TRIES; ++i) {
        char c = data[i][idx];
        if (map.find(c) == map.end())
            map[c] = 0;
        map[c]++;
    }
    return std::max_element(map.begin(), map.end(), 
       [](auto &a, auto &b) { return a.second < b.second; })->first;
}

void util::set_contents(const std::string &fname, C150NastyFile &f_writer, const std::string &data) {
    void *fp = f_writer.fopen(fname.c_str(), "wb");
    if (!fp) throw C150Exception{"Cannot open file to write"};

    f_writer.fwrite(data.c_str(), sizeof(char), data.size());
    f_writer.fclose();

    if (get_contents(fname, f_writer) != data) // invariant check & repeat
        set_contents(fname, f_writer, data);
}

std::string util::get_contents(const std::string &fname, C150NastyFile &f_reader) {
    void *fp = f_reader.fopen(fname.c_str(), "rb");
    if (!fp) throw C150Exception{"Can not open file to read"};

    f_reader.fseek(0L, SEEK_END);
    std::size_t size = f_reader.ftell();    

    std::vector<std::string> data{NUM_TRIES};

    for (int i = 0; i < NUM_TRIES; ++i) {
        data[i].resize(size,'\n');
        f_reader.rewind();
        f_reader.fread(&data[i][0], sizeof(char), size);
    }

    f_reader.fclose();
    for (std::size_t i = 0; i < size; ++i) {
        data[0][i] = median(data, i);
    }
    return data[0];    
}

std::string util::get_SHA1_from_file(const std::string &fname, C150NastyFile &f_reader) {
    std::string contents = get_contents(fname, f_reader);

    unsigned char sha1_hash[20];
    SHA1(reinterpret_cast<const unsigned char*>(contents.c_str()),
         contents.size(), sha1_hash);
    return std::string(&sha1_hash[0], &sha1_hash[20]);
}

std::string util::remove_path(std::string const& fname) {
    std::string cut_fn;
    for (char c : fname) {
        if (c == '/') {
            cut_fn = "";
        } else {
            cut_fn.push_back(c);
        }
    }    
    return cut_fn;
}

void util::send_ack(C150NastyDgmSocket &sock, Packet::Reference ref, 
                                              bool success) {
    Packet::Server::Ack ack{ref, success};
    std::string msg = ack.serialize();
    sock.write(msg.c_str(), msg.size()+1);
}
