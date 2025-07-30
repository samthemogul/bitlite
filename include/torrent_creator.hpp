#ifndef TORRENT_CREATOR_HPP
#define TORRENT_CREATOR_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "bencode.hpp"

namespace torrent {

    struct TorrentFile {
        std::string announce;  // Not required for offline mode
        std::string file_name;
        int64_t file_length;
        std::string piece_hashes;
        int64_t piece_length;
    };

    // Reads file content and splits into fixed size chunks, returns SHA1 concatenated hash string
    std::string compute_piece_hashes(const std::string& file_path, int64_t piece_length = 16384);

    // Creates a .torrent dictionary in bencode format
    std::map<std::string, std::shared_ptr<bencode::Bencode>> create_torrent_dict(const TorrentFile& file);

    // Serializes and writes the .torrent file to disk
    void write_torrent_file(const std::string& output_path, const TorrentFile& file);

    // Reads file content into a vector of characters
    std::vector<char> read_file(const std::string &filepath);

    // Computes the SHA1 hash of a vector of characters
    std::string sha1_hash(const std::vector<char> &data);

    // Creates a .torrent dictionary in bencode format and returns the encoded string
    std::string create_torrent(const std::string &input_file, const std::string &announce_url, int piece_length);

}

#endif // TORRENT_CREATOR_HPP
