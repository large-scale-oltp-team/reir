
#include <foedus/storage/masstree/masstree_storage.hpp>
#include <foedus/proc/proc_id.hpp>
#include <foedus/engine.hpp>
#include <foedus/xct/xct_manager.hpp>
#include "foedus_executor.hpp"

namespace reir {
namespace foedus {

void insert(::foedus::proc::ProcArguments* arg,
            reir::MetaData& md,
            reir::node::Insert* ast,
            const std::string& key,
            const std::string& value) {
  auto* engine = arg->engine_;
  ::foedus::storage::masstree::MasstreeStorage db(engine, "db");
  auto* xct_manager = engine->get_xct_manager();
  auto* ctx = arg->context_;
  xct_manager->begin_xct(ctx, ::foedus::xct::kSerializable);

  auto ret = db.insert_record(arg->context_,
                              key.data(), key.length(),
                              value.data(), value.length());
  if (ret != ::foedus::kErrorCodeOk) {
    throw std::runtime_error(::foedus::get_error_message(ret));
  } else {
    // success
  }
  ::foedus::Epoch commit_epoch;
  xct_manager->precommit_xct(ctx, &commit_epoch);
}

}  // namespace foedus
}  // namespace reir
