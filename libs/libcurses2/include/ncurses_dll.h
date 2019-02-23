//
// Created by rebut_p on 22/02/19.
//

#ifndef KERNEL_NCURSES_DLL_H
#define KERNEL_NCURSES_DLL_H

#define NCURSES_PUBLIC_VAR(name) _nc_##name
#define NCURSES_WRAPPED_VAR(type,name) extern type NCURSES_PUBLIC_VAR(name)(void

#define TRACE
#define NCURSES_IMPEXP
#define NCURSES_API
#define NCURSES_EXPORT(type) NCURSES_IMPEXP type NCURSES_API
#define NCURSES_EXPORT_VAR(type) NCURSES_IMPEXP type

#endif //KERNEL_NCURSES_DLL_H
