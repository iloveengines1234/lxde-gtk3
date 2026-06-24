/*
 * Copyright (C) 2006 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *               2012 Henry Gebhardt <hsggebhardt@gmail.com>
 *               2014 Andriy Grytsenko <andrej@rep.kiev.ua>
 *
 * This file is a part of LXPanel project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef DBG_H
#define DBG_H

#include <stdio.h>
#include <cairo.h> /* Standard include path managed by pkg-config */

/*
 * Macros ENTER, RET(), and DBG() are for low-level debug. Use them in
 * combination with defining DEBUG to add some messages that normally
 * would never be shown. Usually those messages will spam your console
 * and are just for finding problem, not for production usage.
 */

#ifdef DEBUG

#define ENTER            do { fprintf(stderr, "%s:%s:%-5d: ENTER\n",  __FILE__, __func__, __LINE__); } while(0)
#define RET(...)         do { fprintf(stderr, "%s:%s:%-5d: RETURN\n", __FILE__, __func__, __LINE__); \
                              return __VA_ARGS__; } while(0)
#define DBG(fmt, ...)    fprintf(stderr, "%s:%s:%-5d: " fmt, __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#else

#define ENTER            do { } while(0)
#define RET(...)         return __VA_ARGS__
#define DBG(fmt, ...)    do { } while(0)

#endif

void _check_cairo_status(cairo_t* cr, char const* file, char const* func, int line);
void _check_cairo_surface_status(cairo_surface_t** surf, char const* file, char const* func, int line);

/* Using standard __func__ instead of compiler-specific __FUNCTION__ */
#define check_cairo_status(cr) _check_cairo_status(cr, __FILE__, __func__, __LINE__)
#define check_cairo_surface_status(surf) _check_cairo_surface_status(surf, __FILE__, __func__, __LINE__)

#endif /* DBG_H */