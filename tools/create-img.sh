#!/bin/bash

ismounted=0

img_mnt="/mnt"
img_filename="test.img"
img_size="1G"
file_exist="y"
grub_timeout=5
os_app_name="A:/bin/mysh"
kernel_name="kernel_epita"

function readline() {
    read -p "$1" tmpValue
    if [[ ! -z "$tmpValue" ]]; then
	    eval "${2}=$tmpValue"
	fi
}

function doExit() {
    echo "An error occurred" >&2
    if [ "$ismounted" -eq 1 ]; then
	    echo "Umount $img_mnt"
	    sudo umount "$img_mnt"
    fi
    exit 1
}

echo "### Create new k img ###"
readline "Filename ($img_filename): " img_filename
readline "Size ($img_size): " img_size
readline "Kernel name ($kernel_name): " kernel_name
readline "Os starting app ($os_app_name)" os_app_name
readline "Mount point ($img_mnt): " img_mnt
readline "Grub timeout ($grub_timeout): " grub_timeout

if [ -e "$img_filename" ]; then
    readline "$img_filename: file already exist, replace (Y/n): " file_exist
    if [ "$file_exist" = "y" ]; then
	rm -f "$img_filename"
    else
	echo "Abort" >& 2
	exit
    fi
fi

echo

qemu-img create -f raw "$img_filename" "$img_size" > /dev/null && echo "Img created"
if [ $? -ne 0 ]; then
    doExit
fi

mkfs.ext2 "$img_filename" -r 0 -b 1024 > /dev/null && echo "Img formatted"

sudo mount -o loop "$img_filename" "$img_mnt" && echo "Img mounted"
if [ $? -ne 0 ]; then
    doExit
fi

ismounted=1

sudo mkdir -p "$img_mnt/boot/grub" "$img_mnt/home" "$img_mnt/bin" && echo "Directory created"
if [ $? -ne 0 ]; then
    doExit
fi

cat <<EOF > /tmp/testfilegrub && sudo mv /tmp/testfilegrub "$img_mnt/boot/grub/grub.cfg" && echo "Grub config created"
default="0"
timeout=$grub_timeout

menuentry "k - os" {
	  multiboot /boot/$kernel_name $os_app_name
}
EOF

if [ $? -ne 0 ]; then
    doExit
fi
    
sudo grub-install --force --no-floppy --root-directory="$img_mnt" /dev/loop0 > /dev/null && echo "Grub installed"
if [ $? -ne 0 ]; then
    doExit
fi

sudo umount "$img_mnt" && echo "Img umounted"
if [ $? -ne 0 ]; then
    doExit
fi

echo "### New K image created successfully ###"
