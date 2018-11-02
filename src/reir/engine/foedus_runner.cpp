#include <iostream>
#include "foedus_runner.hpp"
#include <numa.h>

#include <foedus/proc/proc_manager.hpp>
#include <foedus/proc/proc_id.hpp>
#include <foedus/engine_options.hpp>
#include <foedus/thread/thread_pool.hpp>
#include <foedus/storage/storage_manager_pimpl.hpp>
#include <foedus/storage/masstree/masstree_metadata.hpp>

#include "reir/exec/db_interface.hpp"

namespace reir {

foedus::ErrorStack trampoline(const foedus::proc::ProcArguments& arg) {
  FoedusRunner* ptr;
  std::memcpy(&ptr, arg.input_buffer_, sizeof(FoedusRunner*));
  FoedusInterface d(&arg);
  ptr->f_(d);
  return foedus::kRetOk;
}

FoedusRunner::FoedusRunner() {
  // FIXME: too hard coded, make them parameter
  foedus::EngineOptions options;
  options.debugging_.debug_log_min_threshold_ =
    foedus::debugging::DebuggingOptions::kDebugLogError;
  //foedus::debugging::DebuggingOptions::kDebugLogInfo;
  options.memory_.use_numa_alloc_ = true;
  options.memory_.page_pool_size_mb_per_node_ = 32;
  options.memory_.private_page_pool_initial_grab_ = 8;

  std::string path = "./";
  if (path[path.size() - 1] != '/') {
    path += "/";
  }
  const std::string snapshot_folder_path_pattern = path + "snapshot/node_$NODE$";
  options.snapshot_.folder_path_pattern_ = snapshot_folder_path_pattern.c_str();
  const std::string log_folder_path_pattern = path + "log/node_$NODE$/logger_$LOGGER$";
  options.log_.folder_path_pattern_ = log_folder_path_pattern.c_str();

  int threads = 1;
  const int cpus = numa_num_task_cpus();
  const int use_nodes = (threads - 1) / cpus + 1;
  const int threads_per_node = (threads + (use_nodes - 1)) / use_nodes;

  options.thread_.group_count_ = (uint16_t)use_nodes;
  options.thread_.thread_count_per_group_ = (foedus::thread::ThreadLocalOrdinal)threads_per_node;

  options.log_.log_buffer_kb_ = 512;
  options.log_.flush_at_shutdown_ = true;

  options.cache_.snapshot_cache_size_mb_per_node_ = 2;
  options.cache_.private_snapshot_cache_initial_grab_ = 16;

  options.xct_.max_write_set_size_ = 4096;

  options.snapshot_.log_mapper_io_buffer_mb_ = 2;
  options.snapshot_.log_reducer_buffer_mb_ = 2;
  options.snapshot_.log_reducer_dump_io_buffer_mb_ = 4;
  options.snapshot_.snapshot_writer_page_pool_size_mb_ = 4;
  options.snapshot_.snapshot_writer_intermediate_pool_size_mb_ = 2;
  options.storage_.max_storages_ = 128;

  engine_ = std::make_shared<foedus::Engine>(options);
  std::cout << "engine initialized" << std::endl;

  engine_
    ->get_proc_manager()
    ->pre_register("func",
                   trampoline);
  COERCE_ERROR(engine_->initialize());
  foedus::storage::masstree::MasstreeMetadata meta("db");
  if (!engine_->get_storage_manager()->get_pimpl()->exists("db")) {
    foedus::Epoch create_epoch;
    engine_->get_storage_manager()
        ->create_storage(&meta, &create_epoch);
  } else {
    std::cout << "massstree already exist" << std::endl;
  }
}

void FoedusRunner::run(std::function<void(DBInterface&)> f) {
  f_ = f;
  void* that = this;
  engine_->get_thread_pool()
    ->impersonate_synchronous("func", &that, sizeof(FoedusRunner*));
}

FoedusRunner::~FoedusRunner() {
  engine_->uninitialize();
  std::cout << "foedus engine successfully uninitialized.\n";
}

}  // namespace reir
