#ifndef REIR_FOEDUS_EXECUTOR_HPP_
#define REIR_FOEDUS_EXECUTOR_HPP_

namespace foedus {
namespace proc {
class ProcArguments;
}  // namespace proc
}  // namespace foedus

namespace reir {
class MetaData;
namespace node {
class Insert;
}  // namespace Insert
namespace foedus {
void insert(::foedus::proc::ProcArguments* ptr,
            reir::MetaData& md,
            reir::node::Insert* ast,
            const std::string& key,
            const std::string& value);
}  // namespace foedus
}  // namespace reir

#endif  // REIR_FOEDUS_EXECUTOR_HPP_
