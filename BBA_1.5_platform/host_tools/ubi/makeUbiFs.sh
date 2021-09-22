#!/bin/sh

SH_DIR=$(cd $(dirname $0) && pwd)

echo "building ubi FS ..."

FS_PATH="$1"
OUTPUT="$2"
PAGE_SIZE="$3"
BLOCK_SIZE="$4"
BLOCK_NUM="$5"
VOL_TYPE="$6"
FS_NAME="$7"
AUTO_RESIZE="$8"

CFG_NAME="ubi.cfg"

##we use lzo2 at current path.
export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH

cd $(dirname "$OUTPUT")/

####-F is needed!!!!!!!
####logic block size = physical block size - 2 * page


"$SH_DIR/mkfs.ubifs" -F -r "$FS_PATH" -o "$OUTPUT.img" -e $(($BLOCK_SIZE-2*$PAGE_SIZE)) -m "$PAGE_SIZE" -c "$BLOCK_NUM"
##use binary tool instead
##mkfs.ubifs -F -r "$FS_PATH" -o "$OUTPUT.img" -e $(($BLOCK_SIZE-2*$PAGE_SIZE)) -m "$PAGE_SIZE" -c "$BLOCK_NUM"

rm -rf "$CFG_NAME"
echo "[ubifs]" >> "$CFG_NAME"
echo "mode=ubi" >> "$CFG_NAME"
echo "image=$(basename $OUTPUT).img" >> "$CFG_NAME"
echo "vol_id=0" >> "$CFG_NAME"
echo "vol_type=$VOL_TYPE" >> "$CFG_NAME"
echo "vol_alignment=1" >> "$CFG_NAME"
echo "vol_name=$FS_NAME" >> "$CFG_NAME"
[ "$AUTO_RESIZE" = "y" ] && echo "vol_flags=autoresize" >> "$CFG_NAME" || echo -n

"$SH_DIR/ubinize" -o "$OUTPUT" -m $PAGE_SIZE -p $BLOCK_SIZE "$CFG_NAME"
##use binary tool instead
##ubinize -o "$OUTPUT" -m $PAGE_SIZE -p $BLOCK_SIZE "$CFG_NAME"

rm -rf "$OUTPUT.img"

rm -rf "$CFG_NAME"
