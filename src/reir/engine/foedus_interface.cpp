#include <foedus/proc/proc_id.hpp>
#include <foedus/storage/masstree/masstree_storage.hpp>
#include <foedus/storage/masstree/masstree_cursor.hpp>
#include <foedus/xct/xct_manager.hpp>
#include <foedus/engine.hpp>
#include <iostream>

#include "foedus_interface.hpp"

bool begin_xct(foedus::proc::ProcArguments *proc) {
  std::cout << "FOEDUS transaction started " << std::endl;
  auto* engine = proc->engine_;
  auto* ctx = proc->context_;
  auto* xct_manager = engine->get_xct_manager();
  auto ret = xct_manager->begin_xct(ctx, ::foedus::xct::kSerializable);
  if (ret == ::foedus::kErrorCodeOk) {
    return true;
  } else {
    std::cout << "foedus error:[begin_xct]: "<< ::foedus::get_error_message(ret) << "\n";
    return false;
  }
}

bool foedus_insert(foedus::proc::ProcArguments* proc,
            const char* key, uint64_t key_len,
            const char* value, uint64_t value_len) {
  auto* engine = proc->engine_;
  ::foedus::storage::masstree::MasstreeStorage db(engine, "db");
  auto ret = db.insert_record(proc->context_,
                              key, static_cast<foedus::storage::masstree::KeyLength>(key_len),
                              value, static_cast<foedus::storage::masstree::PayloadLength>(value_len));
  if (ret != ::foedus::kErrorCodeOk) {
    std::cout << "foedus error:[insert]: "<< ::foedus::get_error_message(ret) << "\n";
    return false;
  } else {
    return true;
  }
}

foedus::storage::masstree::MasstreeCursor* foedus_generate_cursor(foedus::proc::ProcArguments* proc,
                            const char* from, uint64_t from_len,
                            const char* to, uint64_t to_len) {
  auto* engine = proc->engine_;
  ::foedus::storage::masstree::MasstreeStorage db(engine, "db");

  auto* cursor = new foedus::storage::masstree::MasstreeCursor(db, proc->context_);
  auto ret = cursor->open(from, static_cast<foedus::storage::masstree::KeyLength>(from_len),
               to, static_cast<foedus::storage::masstree::KeyLength>(to_len));
  if (ret != foedus::kErrorCodeOk) {
    std::cout << "foedus error:[open_cursor]: " << ::foedus::get_error_message(ret) << "\n";
  }
  return cursor;
}

bool foedus_cursor_is_valid(foedus::storage::masstree::MasstreeCursor* cursor) {
  return cursor->is_valid_record();
}

bool foedus_next_cursor(foedus::storage::masstree::MasstreeCursor* c) {
  c->next();
  return c->is_valid_record();
}

bool foedus_cursor_next(foedus::storage::masstree::MasstreeCursor* cursor) {
  cursor->next();
  return cursor->is_valid_record();
}

void foedus_cursor_copy_key(foedus::storage::masstree::MasstreeCursor* cursor, char* buff){
  cursor->copy_combined_key(buff);
}

void foedus_cursor_copy_value(foedus::storage::masstree::MasstreeCursor* cursor, char* buff) {
  const auto* payload = cursor->get_payload();
  size_t len = cursor->get_payload_length();
  memcpy(buff, payload, len);
}

void foedus_cursor_destroy(foedus::storage::masstree::MasstreeCursor* cursor) {
  delete cursor;
}

bool foedus_destroy_cursor(foedus::storage::masstree::MasstreeCursor* c) {
  delete c;
  return true;
}

bool foedus_scan(foedus::proc::ProcArguments* proc,
                 const char* key, uint64_t key_len) {
  return true;
}

bool foedus_update(foedus::proc::ProcArguments* proc,
            const char* key, uint64_t key_len,
            const char* value, uint64_t value_len) {
  auto* engine = proc->engine_;
  ::foedus::storage::masstree::MasstreeStorage db(engine, "db");
  auto ret = db.overwrite_record(proc->context_,
                                 key, key_len,
                                 value,
                                 0,  // zero offset
                                 value_len);
  std::cout << "update invoked" << std::endl;
  if (ret != ::foedus::kErrorCodeOk) {
    throw std::runtime_error(::foedus::get_error_message(ret));
  } else {
    return true;
  }
}

void precommit_xct(foedus::proc::ProcArguments *proc) {
  std::cout << "FOEDUS commit" << std::endl;
  auto* engine = proc->engine_;
  auto* ctx = proc->context_;
  auto* xct_manager = engine->get_xct_manager();
  ::foedus::Epoch commit_epoch;
  auto ret = xct_manager->precommit_xct(ctx, &commit_epoch);
  if (ret != foedus::kErrorCodeOk) {
    std::cout << ret << "\n";
  } else {
    std::cout << "success.\n";
  }

  xct_manager->wait_for_commit(commit_epoch);  // FIXME: eliminate it
}

