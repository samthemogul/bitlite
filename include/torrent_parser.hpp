#pragma once

#include <string>
#include <memory>
#include "bencode.hpp"
#include "file_manager.hpp"

using namespace bencode;

class TorrentMetadata {
public:
    std::string announce;
    std::string name;
    int64_t pieceLength;
    std::string pieces;
    int64_t length = 0;

    void print() const;
};

class TorrentParser {
public:
    static std::shared_ptr<TorrentMetadata> parse(const std::string& filepath);
    static std::string parse_and_calculate_info_hash(const std::string& filepath);
};
