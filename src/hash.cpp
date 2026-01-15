#include <cstddef>
#include <openssl/evp.h>
#include <stdexcept>
#include <vector>
#include <string>
#include "headers/hash.hpp"

Hash::Hash() : is_finalized(false) {
    if((mdctx = EVP_MD_CTX_new()) == NULL){
        throw std::runtime_error("Ошибка создания хэша");
    }
    if(EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1){
        throw std::runtime_error("Ошибка инициализации хэша");
    }
}

void Hash::update(const std::vector<unsigned char> &v){
    if (!is_finalized){
        if(EVP_DigestUpdate(mdctx, v.data(), v.size()) != 1){
            throw std::runtime_error("Ошибка обновления хэша");
        }
    } else{
        throw std::runtime_error("Нельзя вызывть обновление хэша после его вычисления");
    }
}

void Hash::calculate(){
    unsigned int hash_size = EVP_MD_size(EVP_sha256());
    hash.resize(hash_size);
    if(EVP_DigestFinal_ex(mdctx, hash.data(), &hash_size) != 1){
        throw std::runtime_error("Ошибка вычисления хэша");
    }
    is_finalized = true;
    hash.resize(hash_size);
}

std::string Hash::to_hex() const{
    if(!is_finalized){
        throw std::runtime_error("Кэш ещё не вычеслен, его нельзя перевести в hex");
    }
    char const hex_symbols[] = "0123456789abcdef";

    std::string hash_string;
    hash_string.reserve(hash.size()*2);
    for(unsigned char byte : hash){
        hash_string.push_back(hex_symbols[byte>>4]);
        hash_string.push_back(hex_symbols[byte&0x0F]);
    }
    return hash_string;

}

Hash::~Hash(){
    if( mdctx != nullptr){
        EVP_MD_CTX_free(mdctx);
    }
}
