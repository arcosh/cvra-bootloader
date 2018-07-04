
#
# Include git commit hash and version tag in the bootloader binary
#
GIT_COMMIT_HASH=$(shell git rev-parse --short HEAD)
GIT_VERSION_TAG=$(shell git describe --tags)
$(info Commit hash is $(GIT_COMMIT_HASH))
$(info Version tag is $(GIT_VERSION_TAG))
DEFINES += -DGIT_COMMIT_HASH=\"$(GIT_COMMIT_HASH)\"
DEFINES += -DGIT_VERSION_TAG=\"$(GIT_VERSION_TAG)\"
