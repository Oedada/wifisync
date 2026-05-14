#include "difference.hpp"
#include "transport.hpp"

class DifWalker{
    private:
        json dif;
        void walk_node(const json &node,const std::string &node_name, const fs::path &cur_path, const std::function<void(SUnit)>& emit);
        
    public:
        static SUnit json_to_sunit(const json &node, const fs::path &cur_path, const std::string &name);
        DifWalker(json d);
        void walk(const std::function<void(SUnit)>& emit);

};
