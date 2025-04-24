BUILD_DIR=~/.panda/build
WORKSPACE_DIR=$(pwd)
docker run \
  -e USER="$(id -u)" \
  -u="$(id -u)" \
  -v $WORKSPACE_DIR:$WORKSPACE_DIR \
  -v $BUILD_DIR:$BUILD_DIR \
  -w $WORKSPACE_DIR \
  gcr.io/bazel-public/bazel:latest \
  --output_user_root=$BUILD_DIR \
  build //examples:example1 --disk_cache=$BUILD_DIR
