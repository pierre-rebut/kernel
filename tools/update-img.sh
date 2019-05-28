#!/bin/bash

[ "$UID" -eq 0 ] || exec sudo "$0" "$@"

ismounted=0
mntpoint=/mnt

function doExit() {
    echo "An error occurred" >&2
    if [ "$ismounted" -eq 1 ]; then
	    echo "Umount $mntpoint"
	    umount "$mntpoint"
    fi
    exit 1
}

mount -o loop "$1" "$mntpoint" && echo "Mount $1 on $mntpoint"
if [ $? -ne 0 ]; then
    doExit
fi

echo "Copy bin app on $mntpoint/bin"
cp "$2/k/kernel_epita" "$mntpoint/boot"
cp "$2/user/42sh/42sh" "$mntpoint/bin"
cp "$2/user/utils/mount" "$2/user/utils/umount" "$2/user/utils/cat" "$mntpoint/bin"
cp "$2/user/utils/touch" "$mntpoint/bin"
cp "$2/user/cp/cp" "$mntpoint/bin"
cp "$2/user/utils/wc" "$mntpoint/bin"
cp "$2/user/ls2/ls" "$mntpoint/bin"
cp "$2/user/nano/nano" "$mntpoint/bin"
cp "$2/user/utils/link" "$2/user/utils/mkdir" "$2/user/utils/chmod" "$mntpoint/bin"
cp "$2/user/utils/rm" "$2/user/utils/test_fork" "$mntpoint/bin"

echo "Copy roms on $mntpoint/roms"
cp "$2/user/roms/skate/skate.rom" "$2/user/roms/perrodlauncher/perrodlauncher.rom" "$mntpoint/roms"
cp "$2/user/roms/chichehunter/hunter.rom" "$2/user/roms/chichepong/pong.rom" "$mntpoint/roms"

umount "$mntpoint" && echo "Umount $mntpoint"
