/*
 * (C) Copyright 2002-2016 Diomidis Spinellis
 *
 * This file is part of CScout.
 *
 * CScout is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CScout is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CScout.  If not, see <http://www.gnu.org/licenses/>.
 *

/*
 * Generic gcc workarounds.
 * Note that CScout has builtin support for __atribute__.
 */
#ifdef __cplusplus
#define __EXCEPTIONS
#endif

#define __PRETTY_FUNCTION__ "UNKNOWN"
#define asm __asm__
#define __attribute__(x)
#define __builtin_add_overflow(x,y,z) 1
#define __builtin_bswap16(x) (x)
#define __builtin_bswap32(x) (x)
#define __builtin_bswap64(x) (x)
#define __builtin_choose_expr(a,v,c) 1
#define __builtin_constant_p(_x) ((_x), 0)
#define __builtin_expect(_x, _v) ((_v), (_x))
#define __builtin_fabsf(_x) (float)(_x)
#define __builtin_fabsl(_x) (long double)(_x)
#define __builtin_fabs( _x) (double)(_x)
#define __builtin___fprintf_chk(x,y,z,...) 1
#define __builtin_frame_address(_x) (_x, (void *)0)
#define __builtin_inf()     (double)0
#define __builtin_inff()    (float)0
#define __builtin_infl()    (long double)0
#define __builtin___memcpy_chk(x,y,z,w) 1
#define __builtin_memcpy(_d, _s, _n) (_s, _n, d)
#define __builtin___memmove_chk(x,y,z,w) 1
#define __builtin___mempcpy_chk(x,y,z,w) 1
#define __builtin_memset() 1
#define __builtin___memset_chk(x,y,z,w) 1
#define __builtin_mul_overflow(x,y,z) 1
#define __builtin_next_arg(_x) (_x, 0)
#define __builtin_object_size(x,y) 1
#define __builtin_offsetof(_t, _m) (((size_t) &((_t *)0)->_m))
#define __builtin___printf_chk(x,y,...) 1
#define __builtin_return_address(x) 1
#define __builtin___snprintf_chk(x,y,z,w) 1
#define __builtin___sprintf_chk(x,y,z,w,...) 1
#define __builtin_stdarg_start
#define __builtin___stpcpy_chk(x,y,z) 1
#define __builtin___strcat_chk(x,y,z) 1
#define __builtin_strchr(x,y) 1
#define __builtin_strcmp(x,y) 1
#define __builtin___strcpy_chk(x,y,z) 1
#define __builtin_strcspn(x,y) 1
#define __builtin_strlen(x) 1
#define __builtin___strncat_chk(x,y,z,w) 
#define __builtin___strncpy_chk(x,y,z,w) 1
#define __builtin_types_compatible_p(x,y) 1
#define __builtin_unreachable() 1
#define __builtin_va_arg(_ap, _type) (*(_type *)(_ap))
#define __builtin_va_arg_pack() 1
#define __builtin_va_arg_pack_len() 1
#define __builtin_va_copy(_a, _b)
#define __builtin_va_end(_v)
#define __builtin_va_list void *
#define __builtin_va_start(_ap, _arg) ((void)(_ap),(void)(_arg))
#define __builtin_va_start(x,y) 1
#define __builtin___vprintf_chk(x,y,z) 1
#define __builtin___vsnprintf_chk(x,y,z,w,r) 1
#define __builtin___vsprintf_chk(x,y,z,w,r) 1
#define __COUNTER__ 0
#define __extension
#define __extension__
#define __int128 long
#define __restrict
#define typeof __typeof__
