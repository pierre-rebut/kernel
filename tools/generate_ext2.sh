#!/bin/bash

sudo mount -o loop $1 /mnt
sudo cp $2 /mnt/boot
sudo umount /mnt
