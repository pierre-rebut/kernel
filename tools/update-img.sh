#!/bin/bash

mntpoint=/mnt

sudo mount -o loop $1 $mntpoint
sudo cp $2/k/kernel_epita $mntpoint/boot
sudo cp $2/user/ls/ls $2/user/42sh/42sh $mntpoint/bin
sudo cp $2/user/utils/mount $2/user/utils/umount $2/user/utils/cat $mntpoint/bin
sudo cp $2/user/roms/skate/skate.rom $2/user/roms/perrodlauncher/perrodlauncher.rom $mntpoint/roms
sudo cp $2/user/roms/chichehunter/hunter.rom $2/user/roms/chichepong/pong.rom $mntpoint/roms
sudo cp $2/user/utils/touch $mntpoint/bin
sudo umount $mntpoint
