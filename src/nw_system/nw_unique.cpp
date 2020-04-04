#include <mutex>
#include <stdint.h>

#include <nw_system/export.h>
#include <nw_system/nw_unique.h>
#include <atomic>
std::atomic<uint64_t> unique_id{1};
NW_PUBLIC uint64_t nw_get_unique_id(){
return unique_id++;
}
