#include <cstdint>
#include <stdexcept>
#include <string>

void catch_error(int return_code,std::string error_message){
    if(return_code != 1){
        throw std::runtime_error(error_message);
    }
}

void toBytes(uint64_t x, unsigned char* out_b){
    for(int i = 0; i < 8; i++){
        out_b[7-i] = (x >> i*8) & 0xFF;
    }
}

uint64_t fromBytes64(unsigned char* in_b){
    uint64_t x = 0;
    for(int i = 0; i < 8; i++){
        x |= (uint64_t)in_b[7-i] << i*8;
    }
    return x;
}

void toBytes(uint16_t x, unsigned char* out_b){
    for(int i = 0; i < 2; i++){
        out_b[1-i] = (x >> i*2) & 0xFF;
    }
}

uint16_t fromBytes16(unsigned char* in_b){
    uint16_t x = 0;
    for(int i = 0; i < 2; i++){
        x |= (uint16_t)in_b[1-i] << i*2;
    }
    return x;
}
