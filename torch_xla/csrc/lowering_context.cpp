#include "lowering_context.h"

#include "tensorflow/compiler/xla/xla_client/debug_macros.h"

namespace torch_xla {
namespace ir {

xla::XlaOp LoweringContext::GetParameter(
    const std::shared_ptr<xla::ComputationClient::Data>& data) {
  auto it = parameters_map_.find(data.get());
  if (it == parameters_map_.end()) {
    xla::XlaOp param =
        xla::Parameter(builder(), parameters_.size(), data->shape(),
                       absl::StrCat("param_", parameters_.size()));
    parameters_.push_back(data);
    it = parameters_map_.emplace(data.get(), param).first;
  }
  return it->second;
}

std::vector<xla::ComputationClient::Data*> LoweringContext::GetParametersData()
    const {
  std::vector<xla::ComputationClient::Data*> parameters;
  for (auto& param : parameters_) {
    parameters.push_back(param.get());
  }
  return parameters;
}

xla::int64 LoweringContext::AddResult(xla::XlaOp op) {
  root_tuple_.push_back(std::move(op));
  return root_tuple_.size() - 1;
}

xla::StatusOr<xla::XlaComputation> LoweringContext::Build() {
  if (!root_tuple_.empty()) {
    auto root = xla::Tuple(builder(), root_tuple_);
    return builder()->Build(root);
  }
  return builder()->Build();
}

xla::StatusOr<xla::XlaComputation> LoweringContext::Build(
    const xla::XlaOp& root) {
  XLA_CHECK(root_tuple_.empty());
  return builder()->Build(root);
}

void LoweringContext::AssignOutputOp(const Output& output, xla::XlaOp op) {
  emitted_outputs_[output] = op;
}

xla::XlaOp LoweringContext::GetOutputOp(const Output& output) const {
  auto it = emitted_outputs_.find(output);
  XLA_CHECK(it != emitted_outputs_.end())
      << "No XLA operation amitted for output: " << output;
  return it->second;
}

}  // namespace ir
}  // namespace torch_xla
