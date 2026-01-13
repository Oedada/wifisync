#include <string>
#include <vector>
#include <iostream>
int main(){
    std::vector<std::string> a = {"/", "home", "oedada", "Projects", "apps", "Wifisync", "wifisync", "data", "train"};
    std::vector<int> b = {1, 2, 3, 4, 5};
    std::vector<std::string> c = {"/", "home", "oedada", "Projects", "apps", "Wifisync", "wifisync", "data", "train", "LuckPerms", "translations"};
    std::cout << std::equal(a.begin(), a.end(), c.begin());
}
