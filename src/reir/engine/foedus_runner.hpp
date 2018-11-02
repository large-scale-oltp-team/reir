#ifndef REIR_FOEDUS_RUNNER_HPP_
#define REIR_FOEDUS_RUNNER_HPP_

#include <pthread.h>
#include <functional>
#include <memory>
#include <foedus/engine.hpp>
#include <foedus/proc/proc_id.hpp>

namespace reir {

// foedus::ErrorStack trampoline(const foedus::proc::ProcArguments& arg);

class DBInterface;
class FoedusRunner {
 public:
  FoedusRunner();
  void run(std::function<void(DBInterface&)> f);
  ~FoedusRunner();
 private:
  std::shared_ptr<foedus::Engine> engine_;
  std::function<void(DBInterface&)> f_;
  friend foedus::ErrorStack trampoline(const foedus::proc::ProcArguments&);
};

}  // namespace reir

#endif  // REIR_FOEDUS_RUNNER_HPP_
