#include <filesystem>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
using json = nlohmann::json;


int main(){
    json dict = {
        {"name", "Alice"},
        {"age", 25},
        {"city", "Berlin"}
    };
    dict.erase("jjj");
    return 0;
}
