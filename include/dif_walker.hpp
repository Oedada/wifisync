#include "difference.hpp"
#include "transport.hpp"

class DifWalker{
    private:
        json dif;
        void walk_node(const json &node,const std::string &node_name, const fs::path &cur_path, auto&& emit);
        SUnit json_to_sunit(const json &node, const fs::path &cur_path, const std::string &name);

    public:
        DifWalker(json d);
        void walk(auto&& emit);

};
