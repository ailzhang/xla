#!/bin/bash
set -ex
VERB=
RMBUILD=1
LOGFILE=/tmp/pytorch_cpp_test.log

while getopts 'VLK' OPTION
do
  case $OPTION in
    V)
      VERB="VERBOSE=1"
      ;;
    L)
      LOGFILE=
      ;;
    K)
      RMBUILD=0
      ;;
  esac
done
shift $(($OPTIND - 1))

rm -rf build
mkdir build 2>/dev/null
pushd build
cmake .. \
    -DPYTHON_INCLUDE_DIR=$(python -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())")\
    -DPYTHON_LIBRARY=$(python -c "import distutils.sysconfig as sysconfig; print(sysconfig.get_config_var('LIBDIR') + '/' + sysconfig.get_config_var('LDLIBRARY'))")
make $VERB
if [ "$LOGFILE" != "" ]; then
  ./test_ptxla 2>$LOGFILE
else
  ./test_ptxla
fi
popd
if [ $RMBUILD -eq 1 ]; then
  rm -rf build
fi
