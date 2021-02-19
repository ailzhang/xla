#!/bin/bash

set -ex

source ./xla_env

if [ -x "$(command -v nvidia-smi)" ]; then
  export GPU_NUM_DEVICES=2
else
  export XRT_DEVICE_MAP="CPU:0;/job:localservice/replica:0/task:0/device:XLA_CPU:0"
  export XRT_WORKERS="localservice:0;grpc://localhost:40934"
fi

export CMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}

cd /tmp/pytorch/xla

echo "Running Python Tests"
./test/run_tests.sh

echo "Running MNIST Test"
python test/test_train_mnist.py --tidy
if [ -x "$(command -v nvidia-smi)" ]; then
  #python test/test_train_mp_mnist_amp.py --fake_data
  echo "Running AMP on 2 GPU"
  python test/test_train_mp_mnist_amp.py
fi

echo "Running C++ Tests"
pushd test/cpp
./run_tests.sh
popd
