#include <ios>
#include <iostream>
#include <fstream>
#include <vector>

int main(){
    std::vector<char> buf(8192);
    std::string line;
    std::ifstream fin("/home/oedada/Projects/apps/Wifisync/wifisync/test_text.txt", std::ios::binary);
    std::ofstream fout("/home/oedada/Projects/apps/Wifisync/wifisync/test_out_text.txt", std::ios::binary);
    while(fin.read(buf.data(), buf.size()) || fin.gcount() > 0){
        std::streamsize n = fin.gcount();
        fout.write(buf.data(), n);
    }
    return 0;
}
