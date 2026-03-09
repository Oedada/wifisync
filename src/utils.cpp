#include <stdexcept>
#include <string>

void catch_error(int return_code,std::string error_message){
    if(return_code != 1){
        throw std::runtime_error(error_message);
    }
}
