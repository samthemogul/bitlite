#include "torrent_parser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>

using namespace bencode;

std::string generate_info_hash(const BencodeValue &torrent_dict)
{
    if (!is_dict(torrent_dict))
        throw std::runtime_error("Top-level Bencode is not a dictionary");

    const auto &dict = as_dict(torrent_dict);

    auto it = dict.find("info");
    if (it == dict.end())
        throw std::runtime_error("'info' dictionary not found");

    std::string bencoded_info = encode(it->second->value);

    // SHA1 hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(bencoded_info.c_str()), bencoded_info.size(), hash);

    return std::string(reinterpret_cast<char *>(hash), SHA_DIGEST_LENGTH);
}

std::shared_ptr<TorrentMetadata> TorrentParser::parse(const std::string &filepath)
{
    auto content = read_file_to_string(filepath);
    auto bencode_value = decode(content);

    if (!std::holds_alternative<std::map<std::string, std::shared_ptr<Bencode>>>(bencode_value))
    {
        throw std::runtime_error("Invalid torrent file format");
    }

    auto dict = std::get<std::map<std::string, std::shared_ptr<Bencode>>>(bencode_value);
    auto metadata = std::make_shared<TorrentMetadata>();

    if (dict.find("announce") != dict.end())
    {
        metadata->announce = std::get<std::string>(dict["announce"]->value);
    }

    auto infoDict = std::get<std::map<std::string, std::shared_ptr<Bencode>>>(dict["info"]->value);

    if (infoDict.find("name") != infoDict.end())
    {
        metadata->name = std::get<std::string>(infoDict["name"]->value);
    }

    if (infoDict.find("piece length") != infoDict.end())
    {
        metadata->pieceLength = std::get<int64_t>(infoDict["piece length"]->value);
    }

    if (infoDict.find("pieces") != infoDict.end())
    {
        metadata->pieces = std::get<std::string>(infoDict["pieces"]->value);
    }

    if (infoDict.find("length") != infoDict.end())
    {
        metadata->length = std::get<int64_t>(infoDict["length"]->value);
    }

    return metadata;
}

std::string TorrentParser::parse_and_calculate_info_hash(const std::string &filepath)
{
    auto content = read_file_to_string(filepath);
    auto bencode_value = decode(content);

    if (!std::holds_alternative<std::map<std::string, std::shared_ptr<Bencode>>>(bencode_value))
    {
        throw std::runtime_error("Invalid torrent file format");
    }

    auto dict = std::get<std::map<std::string, std::shared_ptr<Bencode>>>(bencode_value);

    return generate_info_hash(dict);
}

void TorrentMetadata::print() const
{
    std::cout << "Announce URL: " << announce << "\n"
              << "Name: " << name << "\n"
              << "Piece Length: " << pieceLength << "\n"
              << "Total Length: " << length << "\n"
              << "Pieces (SHA1s combined): " << pieces.size() << " bytes\n";
}
