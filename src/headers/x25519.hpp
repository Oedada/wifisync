#include <openssl/evp.h>

bool equal_secrets(unsigned char *s1, unsigned char *s2, size_t len);

class X25519{
    public:
        const static int key_lenght = 32;
        unsigned char *secret = nullptr;
        unsigned char pub_key[key_lenght];
        size_t secret_len;
        // X25519(const X25519&) = delete;
        // X25519& operator=(const X25519&) = delete;
        X25519();
        void calculate_secret(unsigned char *other_key);
        ~X25519();
    private:
        void get_pub_key();
        void gen_pair_keys();
        void set_other_pub_key(unsigned char *other_key);
        EVP_PKEY *own_key_pair = nullptr;
        EVP_PKEY *other_pub_key_pair = nullptr;
};
