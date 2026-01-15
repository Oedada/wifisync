#include <openssl/evp.h>
#include <vector>
#include <string>

class Hash{
    public:
        std::vector<unsigned char> hash;
        Hash(const Hash&) = delete;
        Hash& operator=(const Hash&) = delete;
        Hash();
        void update(const std::vector<unsigned char> &v);
        void calculate();
        std::string to_hex() const;
        ~Hash();
    private:
        EVP_MD_CTX *mdctx;
        bool is_finalized;
};
