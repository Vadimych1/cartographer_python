# this script is used for github ci/cd
set -eux -o pipefail

yum update -y

yum install -y cmake ceres-solver-devel eigen3-devel glog-devel ninja-build epel-release boost boost-thread boost-devel
yum insatll -y gflags gflags-devel glog glog-devel gmock gmock-devel eigen3-devel protobuf-devel protobuf-compiler lua-devel
yum install -y atlas-devel unwind-devel absl-devel cairo-devel
