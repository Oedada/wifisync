#include <cstddef>
#include <iostream>
#include <openssl/evp.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include "headers/hash.hpp"
#include "headers/x25519.hpp"
#include "headers/utils.hpp"

X25519::X25519(){
    gen_pair_keys();
    get_pub_key();
}
void X25519::gen_pair_keys(){
    own_key_pair = nullptr;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
    catch_error(EVP_PKEY_keygen_init(ctx), "Error with init X25519 context");
    catch_error(EVP_PKEY_keygen(ctx, &own_key_pair), "Error with generate X25519 key");
    EVP_PKEY_CTX_free(ctx);
}

void X25519::get_pub_key(){
    if(!own_key_pair) throw std::runtime_error("No key generated");
    size_t len = key_lenght;
    EVP_PKEY_get_raw_public_key(own_key_pair, pub_key, &len);
}

void X25519::set_other_pub_key(unsigned char *other_key){
    other_pub_key_pair = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_X25519,
        nullptr,
        other_key,
        key_lenght
    );
    if (!other_pub_key_pair){
        throw std::runtime_error("Failed to create peer key");
    }
}

void X25519::calculate_secret(unsigned char *other_key){
    if(secret != nullptr){
        std::cerr << secret << "\n";
        throw std::logic_error("Cannot change peer key after secret is derived");
    }
    set_other_pub_key(other_key);
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(own_key_pair, nullptr);
    if (!ctx) {
        throw std::runtime_error("Failed to create derive context");
    }
    catch_error(EVP_PKEY_derive_init(ctx), "Failed to derive init");
    catch_error(EVP_PKEY_derive_set_peer(ctx, other_pub_key_pair), "Failed to set derive peer");
    
    secret_len = key_lenght;
    secret = (unsigned char*)OPENSSL_malloc(secret_len);
    catch_error(EVP_PKEY_derive(ctx, secret, &secret_len), "Failed to derive to count secret");
    EVP_PKEY_CTX_free(ctx);
}

X25519::~X25519() {
    if (own_key_pair) { EVP_PKEY_free(own_key_pair); own_key_pair = nullptr; }
    if (other_pub_key_pair) { EVP_PKEY_free(other_pub_key_pair); other_pub_key_pair = nullptr; }
    if (secret) { OPENSSL_free(secret); secret = nullptr; }
}

bool equal_secrets(unsigned char *s1, unsigned char *s2, size_t len){
    return CRYPTO_memcmp(s1, s2, len) == 0;
}
