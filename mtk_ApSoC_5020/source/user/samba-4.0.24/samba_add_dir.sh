#!/bin/sh

SAMBA_FILE=/etc/smb.conf

if [ ! -n "$2" ]; then
	echo "insufficient arguments!"
	echo "Usage: $0 <dir name> <access path> <allow users>"
	echo "Example: $0 temp /temp admin"
	exit 0
fi

echo $2 > /tmp/samba_add_dir.tmp
path=`cut -d'/' -f1-3 /tmp/samba_add_dir.tmp`
vfat_mount=`mount | grep "$path" |grep vfat`
if [ "$vfat_mount" != "" ]; then
	loose_allocate="loose allocate = yes"

fi
rm /tmp/samba_add_dir.tmp

ALLOWUSERS="$3 $4 $5 $6 $7 $8 $9 ${10} ${11}"

echo "
["$1"]
path = "$2"
valid users = "$ALLOWUSERS"
browseable = yes
writable = yes
create mask = 0664
directory mask = 0775
guest ok = yes
"$loose_allocate"

" >> $SAMBA_FILE


echo "[homes]
browseable = yes
guest ok = yes
read only = no
create mask = 0755

" >> $SAMBA_FILE

