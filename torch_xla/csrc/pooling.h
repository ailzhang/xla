#pragma once

#include "tensorflow/compiler/xla/client/xla_builder.h"
#include "torch/csrc/jit/ir.h"

namespace torch_xla {

// Computes max pooling for the given input with the attributes specified in the
// given node.
xla::XlaOp BuildMaxPool2d(const torch::jit::Node* node,
                          const xla::XlaOp& input);

// Computes the gradient for max pooling.
xla::XlaOp BuildMaxPool2dBackward(const torch::jit::Node* node,
                                  const xla::XlaOp& out_backprop,
                                  const xla::XlaOp& input);

// Computes average pooling for the given input with the attributes specified in
// the given node.
xla::XlaOp BuildAvgPool2d(const torch::jit::Node* node,
                          const xla::XlaOp& input);

// Computes the gradient for average pooling.
xla::XlaOp BuildAvgPool2dBackward(const torch::jit::Node* node,
                                  const xla::XlaOp& out_backprop,
                                  const xla::XlaOp& input);

// Computes adaptive average pooling for the given input with the output size
// specified in the given node.
xla::XlaOp BuildAdaptiveAvgPool2d(const torch::jit::Node* node,
                                  const xla::XlaOp& input);

// Computes the gradient for adaptive average pooling.
xla::XlaOp BuildAdaptiveAvgPool2dBackward(const torch::jit::Node* node,
                                          const xla::XlaOp& out_backprop,
                                          const xla::XlaOp& input);

}  // namespace torch_xla
