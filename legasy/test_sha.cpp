#include <cstddef>
#include <iostream>
#include <openssl/evp.h>
#include <stdexcept>
#include <vector>
#include <string>

class Hash{
    Hash(const Hash&) = delete;
    Hash& operator=(const Hash&) = delete;
    public:
        std::vector<unsigned char> hash;
        Hash(){
            is_finalized = false;
            if((mdctx = EVP_MD_CTX_new()) == NULL){
                throw std::runtime_error("Ошибка создания хэша");
            }
            if(EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1){
                throw std::runtime_error("Ошибка инициализации хэша");
            }
        }

        void update(const std::vector<unsigned char> &v){
            if (!is_finalized){
                if(EVP_DigestUpdate(mdctx, v.data(), v.size()) != 1){
                    throw std::runtime_error("Ошибка обновления хэша");
                }
            } else{
                throw std::runtime_error("Нельзя вызывть обновление хэша после его вычисления");
            }
        }

        void calculate(){
            unsigned int hash_size = EVP_MD_size(EVP_sha256());
            hash.resize(hash_size);
            if(EVP_DigestFinal_ex(mdctx, hash.data(), &hash_size) != 1){
                throw std::runtime_error("Ошибка вычисления хэша");
            }
            is_finalized = true;
            hash.resize(hash_size);
        }

        std::string to_hex() const{
            if(!is_finalized){
                throw std::runtime_error("Кэш ещё не вычеслен, его нельзя перевести в hex");
            }
            char hex_symbols[] = "0123456789abcdef";

            std::string hash_string;
            hash_string.reserve(hash.size()*2);
            for(unsigned char byte : hash){
                hash_string.push_back(hex_symbols[byte>>4]);
                hash_string.push_back(hex_symbols[byte&0x0F]);
            }
            return hash_string;

        }

        ~Hash(){
            if( mdctx != nullptr){
                EVP_MD_CTX_free(mdctx);
            }
        }

        private:
            EVP_MD_CTX *mdctx;
            bool is_finalized;
};

int main(){
    Hash h;
    std::string hello = "hello";
    std::vector<unsigned char> msg(hello.begin(), hello.end());
    h.update(msg);
    h.calculate();
    std::cout << h.to_hex();
}
