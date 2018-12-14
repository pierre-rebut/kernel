#!/bin/sh

iso_filename=$1
base_dir=iso

mkdir -p $base_dir/
mkdir -p $base_dir/roms/
mkdir -p $base_dir/boot/grub/

cp k/$2 $base_dir/

find roms -name "*.rom" -exec cp {} $base_dir/roms/ \;

for rom in $(find $base_dir/roms -name "*.rom") ; do
	name=$(basename $rom .rom)
	cat <<EOF
menuentry "k - $name" {
	multiboot /$2 /$name
	module /roms/$name.rom
}
EOF
done > $base_dir/boot/grub/grub.cfg

grub-mkrescue -o $3/$iso_filename $base_dir
