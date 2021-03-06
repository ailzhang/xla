#include "replace_untraced_operators.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/compiler/xla/xla_client/debug_macros.h"

namespace torch_xla {
namespace {

// Returns true if the node contains an attribute and has the expected value.
template <class T>
bool NodeHasExpectedAttribute(const torch::jit::Node* node,
                              const at::Symbol attribute_name,
                              const T& expected) {
  const auto maybe_attribute = node->get<T>(attribute_name);
  return maybe_attribute && *maybe_attribute == expected;
}

// Only allow certain at::aten::_convolution operators to be replaced.
bool CanTraceConvolution(const torch::jit::Node* node) {
  return NodeHasExpectedAttribute(node, at::attr::dilation,
                                  std::vector<int64_t>{1, 1}) &&
         NodeHasExpectedAttribute(node, at::attr::output_padding,
                                  std::vector<int64_t>{0, 0}) &&
         NodeHasExpectedAttribute(node, at::attr::transposed, false) &&
         NodeHasExpectedAttribute(node, at::attr::groups, int64_t(1)) &&
         NodeHasExpectedAttribute(node, at::attr::benchmark, false) &&
         NodeHasExpectedAttribute(node, at::attr::deterministic, false);
}

// When possible, replace at::aten::{_convolution, batch_norm} operators with
// equivalent ones which are part of the operator schema and differentiable.
void ReplaceUntracedOperators(torch::jit::Block* block) {
  for (auto it = block->nodes().begin(), end = block->nodes().end(); it != end;
       ++it) {
    for (auto sub : it->blocks()) {
      ReplaceUntracedOperators(sub);
    }
    switch (it->kind()) {
      case at::aten::_convolution: {
        torch::jit::WithInsertPoint guard(*it);
        auto graph = block->owningGraph();
        auto node = *it;
        if (!CanTraceConvolution(node)) {
          break;
        }
        const auto weight = node->namedInput(at::attr::weight);
        const auto weight_type =
            weight->type()->expect<at::CompleteTensorType>();
        const auto& weight_size = weight_type->sizes();
        const auto kernel_size = graph->insertConstant(
            std::vector<int64_t>{weight_size[2], weight_size[3]});
        const auto stride = graph->insertConstant(
            node->get<std::vector<int64_t>>(at::attr::stride).value());
        const auto padding = graph->insertConstant(
            node->get<std::vector<int64_t>>(at::attr::padding).value());

        auto replacement_node = graph->create(at::aten::thnn_conv2d_forward, 3);

        graph->insertNode(replacement_node);
        replacement_node->addInput(node->namedInput(at::attr::input));
        replacement_node->addInput(weight);
        replacement_node->addInput(kernel_size);
        replacement_node->addInput(node->namedInput(at::attr::bias));
        replacement_node->addInput(stride);
        replacement_node->addInput(padding);

        replacement_node->outputs()[0]->setType(it->outputs()[0]->type());
        TF_VLOG(3) << "Replacing " << **it << " with traceable counterpart";
        it->output()->replaceAllUsesWith(replacement_node->outputs()[0]);
        it.destroyCurrent();
        break;
      }
      case at::aten::batch_norm: {
        torch::jit::WithInsertPoint guard(*it);
        auto graph = block->owningGraph();
        auto node = *it;
        auto replacement_node = graph->create(at::aten::native_batch_norm, 3);

        graph->insertNode(replacement_node);
        const auto node_inputs = node->inputs();
        JIT_ASSERT(node_inputs.size() == 9);
        for (size_t i = 0; i < node_inputs.size() - 1; ++i) {
          replacement_node->addInput(node_inputs[i]);
        }
        replacement_node->outputs()[0]->setType(it->outputs()[0]->type());
        TF_VLOG(3) << "Replacing " << **it << " with traceable counterpart";
        it->output()->replaceAllUsesWith(replacement_node->outputs()[0]);
        it.destroyCurrent();
        break;
      }
      default: {
        break;
      }
    }
  }
}

}  // namespace

void ReplaceUntracedOperators(const std::shared_ptr<torch::jit::Graph>& graph) {
  XLA_VLOG_LINES(4, "Before ReplaceUntracedOperators:\n" + graph->toString());
  ReplaceUntracedOperators(graph->block());
  XLA_VLOG_LINES(4, "After ReplaceUntracedOperators:\n" + graph->toString());
}

}  // namespace torch_xla
