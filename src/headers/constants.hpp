#pragma once
#include <cstdint>
#include <string>

namespace constants{
    //stable
    inline const std::string project_name = "wifisync";
    inline const std::string author_name = "Oedada";

    // cryptography
    constexpr int HASH_BYTE_LENGTH = 32;

    
    // json file tree
    inline const std::string MODIFIED_UNIT = "M";
    inline const std::string DELETED_UNIT = "D";
    inline const std::string ADDED_UNIT = "A";
    inline const std::string JSON_FIELD_NAME_TYPE = "/type";
    inline const std::string JSON_FIELD_NAME_HASH = "/hash";
    inline const std::string UNIT_TYPE_DIR = "dir";
    inline const std::string UNIT_TYPE_EMPTY_DIR = "empty_dir";
    inline const std::string UNIT_TYPE_FILE = "file";
    
    ///network
    const char MagicMessage[] = "Wifisync Hello Wifi:";
    const char MagicResponse[] = "Wifisync Response Wifi";
    inline const char* TEST_NET_IP = "192.0.2.1";
    inline const char* PING_CONNECT_TEST_IP = "8.8.8.8";
    inline const char* GLOBAL_BROADCAST_IP = "255.255.255.255";
    constexpr int BUFFER_SIZE = 8192;
    constexpr uint64_t SOCKET_CHECK_TIMEOUT = 200000;
    inline const char* LOCAL_IP_ADDR = "0.0.0.0";
    constexpr int CONNECT_RETRIES_COUNT = 5;
    constexpr uint64_t MAX_ALLOWED_SIZE = 8388608;
    constexpr int SIZE_HEADER_BYTES = 8;
    constexpr uint16_t BROADCAST_PORT = 12312;
    constexpr uint16_t TCP_PORT = 12345;

    //paths
    inline std::string KEY_DIR = "key";
    inline std::string UUID_FILE = "uuid.uuid";
    inline std::string CONFIG_FILE = "config.json";
};
