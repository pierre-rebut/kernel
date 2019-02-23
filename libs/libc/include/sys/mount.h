//
// Created by rebut_p on 22/02/19.
//

#ifndef _MOUNT_H
#define _MOUNT_H

int mount(const char *fstype, const char *dev, const char *mnt);

int umount(const char *mnt);

#endif //_MOUNT_H
