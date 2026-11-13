#include "core/dif_work.hpp"
#include "fs/environment.hpp"
#include "util/init.hpp"
#include "util/utils.hpp"
#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <string>
#include <sys/prctl.h>
#include <unistd.h>

using json = nlohmann::json;

int main() {
    init();
    if (is_dryrun()) {
        logmsg("dry run");
        DifWork dw("f655377f-0c5b-411f-9b2c-17d90097ca08");
        dw.shift_snapshots();
        dw.calculate_dif();
    }
    return 0;
}
