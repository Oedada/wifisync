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
    const int SLEEP_TIME = 200000;
    const int TIMEOUT_TIME = 15;
    const char StaticBroadcastMessage[] = "Wifisync Hello Wifi:";
    const char StaticRequestConnect[] = "Wifisync Connect Request:";
    const char StaticResponseConnect[] = "Wifisync Connect Response:";
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
    constexpr int COUNT_FIND_DEVICE = 10;

    //paths
    inline std::string KEY_DIR = "key";
    inline std::string UUID_FILE = "uuid.uuid";
    inline std::string CONFIG_FILE = "config.json";
    inline std::string DEVICES_FILE = "devices.json";
    inline std::string TMP_DIR = "tmp";
    inline std::string DATA_DIR = "data";
    inline std::string LAST_SNAPSHOT_FILENAME = "last_snapshot.json";
    inline std::string CURRENT_SNAPSHOT_FILENAME = "cur_snapshot.json";
    inline std::string DIFFERENCE_FILENAME = "dif.json";
    inline std::string DETECTING_UNITS_FILENAME = "detecting_units.ws";
    inline std::string IGNORING_UNITS_FILENAME = "ignoring_units.ws";
};
