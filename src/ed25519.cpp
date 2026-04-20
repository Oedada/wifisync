#include <cstddef>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdexcept>
#include <filesystem>
#include "ed25519.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

Ed25519::Ed25519(const fs::path& kd) : key_dir(kd) {
    if(!fs::exists(key_dir)){
        fs::create_directory(key_dir);
    }
    // Проверка, существует ли файл ключа
    if(!fs::exists(key_dir / "ed25519_priv.pem")){
        // Создаём контекст для генерации ключа
        EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
        if (!pctx){
            throw std::runtime_error("Error creating context for Ed25519"); 
        }
        // Инициализируем
        catch_error(EVP_PKEY_keygen_init(pctx), "Keygen init error for Ed25519");
        // Генерируем ключ
        catch_error(EVP_PKEY_keygen(pctx, &pkey), "Keygen init error for Ed25519");
        // Сохраняем приватный ключ в PEM
        FILE *f = fopen((key_dir / "ed25519_priv.pem").c_str(), "wb");
        if(!f) throw std::runtime_error("Cannot open file to write key");
        PEM_write_PrivateKey(f, pkey, nullptr, nullptr, 0, nullptr, nullptr);
        fclose(f);
        // Очищаем контекст и объект пары ключей
        EVP_PKEY_CTX_free(pctx);
        // Сохраняем публичный ключ в PEM
        f = fopen((key_dir / "ed25519_pub.pem").c_str(), "wb");
        if(!f) throw std::runtime_error("Cannot open file to write key");
        PEM_write_PUBKEY(f, pkey);
        fclose(f);
    }
    // если не существет генерим и записываем в файл
    else{
        FILE* f = fopen((key_dir / "ed25519_priv.pem").c_str(), "rb");
        if(!f) throw std::runtime_error("Cannot open file to read key");
        pkey = PEM_read_PrivateKey(f, nullptr, nullptr, nullptr);
        fclose(f);
        if(!pkey) throw std::runtime_error("Failed to read private key");
    }
}

void Ed25519::sign(const unsigned char* msg, size_t msg_len){
    size_t siglen = sizeof(sig);

    // создам контекст
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    // инициализация
    catch_error(EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey), "Failed initialize sign");
    // подпись
    catch_error(EVP_DigestSign(ctx, sig, &siglen, msg, msg_len),"Failed sign message");

    EVP_MD_CTX_free(ctx);
}

Ed25519::~Ed25519(){
    EVP_PKEY_free(pkey);
}

bool check_sig(fs::path pub_key_path, const unsigned char* msg, size_t msg_len, unsigned char* sig, size_t sig_len){
    // Загружаем публичный ключ
    FILE* f = fopen(pub_key_path.c_str(), "rb");
    if(!f) throw std::runtime_error("Cannot open file to read key");
    EVP_PKEY* pkey = PEM_read_PUBKEY(f, nullptr, nullptr, nullptr);
    fclose(f);
    if(!pkey) throw std::runtime_error("Failed to read public key");
    // Проверяем размер подписанного сообщения
    size_t siglen = sig_len;
    if(sig_len != 64){
        throw std::runtime_error("Signed message len should be 64");
    }
    // Создаём контекст
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    // Инициализируем
    EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey);
    // Проверяем подпись
    int ret = EVP_DigestVerify(ctx, sig, siglen, msg, msg_len);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ret>0;
}
