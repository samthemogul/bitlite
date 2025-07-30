#include "torrent_creator.hpp"
#include "bencode.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <openssl/sha.h>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace torrent
{
    std::vector<char> read_file(const std::string &filepath)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file)
            throw std::runtime_error("Failed to open file: " + filepath);

        return std::vector<char>(std::istreambuf_iterator<char>(file), {});
    }

    std::string sha1_hash(const std::vector<char> &data)
    {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char *>(data.data()), data.size(), hash);
        return std::string(reinterpret_cast<char *>(hash), SHA_DIGEST_LENGTH);
    }

    std::string create_torrent(const std::string &input_file, const std::string &announce_url, int piece_length)
    {
        using namespace bencode;

        // Read file content
        std::vector<char> content = read_file(input_file);
        std::string filename = fs::path(input_file).filename().string();

        // Split file into pieces and hash
        std::string pieces_concat;
        for (size_t i = 0; i < content.size(); i += piece_length)
        {
            size_t len = std::min(piece_length, static_cast<int>(content.size() - i));
            std::vector<char> piece(content.begin() + i, content.begin() + i + len);
            pieces_concat += sha1_hash(piece); // binary concat
        }

        // Build info dictionary
        std::map<std::string, std::shared_ptr<Bencode>> info_dict;
        info_dict["name"] = std::make_shared<Bencode>(Bencode{filename});
        info_dict["length"] = std::make_shared<Bencode>(Bencode{static_cast<int64_t>(content.size())});
        info_dict["piece length"] = std::make_shared<Bencode>(Bencode{static_cast<int64_t>(piece_length)});
        info_dict["pieces"] = std::make_shared<Bencode>(Bencode{pieces_concat});

        // Build top-level dictionary
        std::map<std::string, std::shared_ptr<Bencode>> torrent_dict;
        torrent_dict["announce"] = std::make_shared<Bencode>(Bencode{announce_url});
        torrent_dict["info"] = std::make_shared<Bencode>(Bencode{info_dict});

        // Encode and return the bencoded string
        return encode(torrent_dict);
    }

    std::string compute_piece_hashes(const std::string &file_path, int64_t piece_length)
    {
        std::vector<char> content = read_file(file_path);
        std::string pieces_concat;

        for (size_t i = 0; i < content.size(); i += piece_length)
        {
            size_t len = std::min(piece_length, static_cast<int64_t>(content.size() - i));
            std::vector<char> piece(content.begin() + i, content.begin() + i + len);
            pieces_concat += sha1_hash(piece); // binary concatenation of SHA1 hashes
        }

        return pieces_concat;
    }

    std::map<std::string, std::shared_ptr<bencode::Bencode>> create_torrent_dict(const TorrentFile &file)
    {
        using namespace bencode;

        std::map<std::string, std::shared_ptr<Bencode>> info_dict;
        info_dict["name"] = std::make_shared<Bencode>(Bencode{file.file_name});
        info_dict["length"] = std::make_shared<Bencode>(Bencode{file.file_length});
        info_dict["piece length"] = std::make_shared<Bencode>(Bencode{file.piece_length});
        info_dict["pieces"] = std::make_shared<Bencode>(Bencode{file.piece_hashes});

        std::map<std::string, std::shared_ptr<Bencode>> torrent_dict;
        torrent_dict["announce"] = std::make_shared<Bencode>(Bencode{file.announce});
        torrent_dict["info"] = std::make_shared<Bencode>(Bencode{info_dict});

        return torrent_dict;
    }

    void write_to_file(const std::string &output_path, const std::string &content)
    {
        std::ofstream file(output_path, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Failed to open output file: " + output_path);
        }
        file << content;
        if (!file)
        {
            throw std::runtime_error("Failed to write to output file: " + output_path);
        }
    }

    void write_torrent_file(const std::string &output_path, const TorrentFile &file)
    {
        auto torrent_dict = create_torrent_dict(file);
        std::string encoded = bencode::encode(torrent_dict);
        write_to_file(output_path, encoded);
    }

} // namespace torrent
