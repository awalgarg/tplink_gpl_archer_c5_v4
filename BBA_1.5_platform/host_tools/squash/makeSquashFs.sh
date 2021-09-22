#!/bin/sh

SH_DIR=$(cd $(dirname $0) && pwd)

FS_PATH=$1
OUTPUT=$2
BLOCK_SIZE=$3

echo "building squash FS ..."
"$SH_DIR/mksquashfs4.2" "$FS_PATH" "$OUTPUT" -noappend -always-use-fragments -all-root -comp xz -b "$BLOCK_SIZE"

