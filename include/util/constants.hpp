#pragma once

#include <cstdint>
#include <string>


namespace constants {
// stable
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
inline const std::string JSON_FIELD_NAME_MOD_TYPE = "/mtype";
inline const std::string JSON_FIELD_NAME_OTHER_PATH = "/other_path";
inline const std::string UNIT_TYPE_DIR = "dir";
inline const std::string UNIT_TYPE_EMPTY_DIR = "empty_dir";
inline const std::string UNIT_TYPE_FILE = "file";

/// network
const int SLEEP_TIME = 200000;
const int TIMEOUT_TIME = 3;
const char StaticBroadcastMessage[] = "Wifisync Hello Wifi:";
const char StaticRequestConnect[] = "Wifisync Connect Request:";
const char StaticResponseConnect[] = "Wifisync Connect Response:";
inline const char *TEST_NET_IP = "192.0.2.1";
inline const char *PING_CONNECT_TEST_IP = "8.8.8.8";
inline const char *GLOBAL_BROADCAST_IP = "255.255.255.255";
constexpr int BUFFER_SIZE = 8192;
constexpr uint64_t SOCKET_CHECK_TIMEOUT = 200000;
inline const char *LOCAL_IP_ADDR = "0.0.0.0";
constexpr int CONNECT_RETRIES_COUNT = 5;
constexpr uint64_t MAX_ALLOWED_SIZE = 8388608;
constexpr int SIZE_HEADER_BYTES = 8;
constexpr uint16_t BROADCAST_PORT = 12312;
constexpr uint16_t TCP_PORT = 12345;
constexpr int COUNT_FIND_DEVICE = 10;

// paths
namespace paths {
inline constexpr const std::string_view config = "config.json";
namespace data {
inline constexpr const std::string_view current_snapshot =
    "current_snapshot.json";
inline constexpr const std::string_view last_snapshot = "last_snapshot.json";
inline constexpr const std::string_view difference = "difference.json";
inline constexpr const std::string_view devices = "devices.json";

} // namespace data
}; // namespace paths
}; // namespace constants
