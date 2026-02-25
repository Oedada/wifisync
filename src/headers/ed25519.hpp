#include <openssl/evp.h>
#include <filesystem>

bool equal_secrets(unsigned char *s1, unsigned char *s2, size_t len);

class Ed25519{
    public:
        const static int key_lenght = 32;
        unsigned char sig[64];  // подпись всегда 64 байта
        EVP_PKEY *pkey = nullptr;
        std::filesystem::path key_dir;
        Ed25519(const std::filesystem::path& kd);
        void sign(const unsigned char* msg, size_t msg_len);
        ~Ed25519();
};

bool check_sig(std::filesystem::path pub_key_path, const unsigned char* msg, size_t msg_len, unsigned char* sig, size_t sig_len);
