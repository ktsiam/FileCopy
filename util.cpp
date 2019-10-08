#include <openssl/sha.h>
#include <string>
#include <fstream>
#include <sstream>
#include "util.h"

std::string util::get_SHA1_from_file(std::string const& filename) {
    std::ifstream inf(filename);
    if (!inf) throw "Non-existant file";

    std::ostringstream oss;
    oss << inf.rdbuf();
    std::string contents = oss.str();

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
        }
        cut_fn.push_back(c);
    }    
    return cut_fn;
}
