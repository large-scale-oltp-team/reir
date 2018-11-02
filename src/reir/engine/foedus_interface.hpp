#ifndef FOEDUS_INTERFACE_HPP_
#define FOEDUS_INTERFACE_HPP_

#include "util/slice.hpp"

namespace foedus {
namespace proc {
class ProcArguments;
}
namespace storage {
namespace masstree {
class MasstreeCursor;
}
}
}

extern "C" {

bool begin_xct(foedus::proc::ProcArguments *proc);

bool foedus_insert(foedus::proc::ProcArguments* proc,
                   const char* key, uint64_t key_len,
                   const char* value, uint64_t value_len);

void precommit_xct(foedus::proc::ProcArguments *proc);

foedus::storage::masstree::MasstreeCursor* foedus_generate_cursor(
    foedus::proc::ProcArguments* proc,
    const char* from, uint64_t from_len,
    const char* to, uint64_t to_len);

bool foedus_cursor_next(foedus::storage::masstree::MasstreeCursor* cursor);
bool foedus_cursor_is_valid(foedus::storage::masstree::MasstreeCursor* cursor);
void foedus_cursor_copy_key(foedus::storage::masstree::MasstreeCursor* cursor, char* buff);
void foedus_cursor_copy_value(foedus::storage::masstree::MasstreeCursor* cursor, char* buff);
void foedus_cursor_destroy(foedus::storage::masstree::MasstreeCursor* cursor);

void link_test() {
  std::cout << "link test success" << std::endl;
}

}  // extern "C"
#endif
