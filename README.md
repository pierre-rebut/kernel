K
=

dependancies for building
--------------------------

* gcc-multilib
* grub2 (for grub-mkrescue)
* libisoburn
* find

launch k
--------
	* Use tools/create-img.sh to genreate a k.img virtual hard drive
	* Create a build dir && use cmake .. && make
	* Use tools/generate_ext2.sh to cpy kernel_epita and app onto k.img
	$ qemu-system-x86_64 -hda format=raw,k.img -serial stdio

TODO
----

Check trello k - os
