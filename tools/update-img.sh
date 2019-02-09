#!/bin/bash

sudo mount -o loop $1 /mnt
sudo cp $2/k/kernel_epita /mnt/boot
sudo cp $2/user/ls/ls $2/user/42sh/42sh /mnt/bin
sudo cp $2/user/utils/mount $2/user/utils/umount $2/user/utils/cat /mnt/bin
sudo cp $2/user/roms/skate/skate.rom $2/user/roms/perrodlauncher/perrodlauncher.rom /mnt/roms
sudo cp $2/user/roms/chichehunter/hunter.rom $2/user/roms/chichepong/pong.rom /mnt/roms
sudo umount /mnt&
