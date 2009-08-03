/*
 * boblight
 * Copyright (C) Bob  2009 
 * 
 * boblight is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * boblight is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//these definitions can be expanded to make normal prototypes, or functionpointers and dlsym lines

BOBLIGHT_FUNCTION(void*,       boblight_init,             ());
BOBLIGHT_FUNCTION(void,        boblight_destroy,          (void* vpboblight));

BOBLIGHT_FUNCTION(int,         boblight_connect,          (void* vpboblight, const char* address, int port, int usectimeout));
BOBLIGHT_FUNCTION(int,         boblight_setpriority,      (void* vpboblight, int priority));
BOBLIGHT_FUNCTION(const char*, boblight_geterror,         (void* vpboblight));
BOBLIGHT_FUNCTION(int,         boblight_getnrlights,      (void* vpboblight));
BOBLIGHT_FUNCTION(const char*, boblight_getlightname,     (void* vpboblight, int lightnr));

BOBLIGHT_FUNCTION(int,         boblight_setspeed,         (void* vpboblight, int lightnr, float speed));
BOBLIGHT_FUNCTION(int,         boblight_setinterpolation, (void* vpboblight, int lightnr, int on));
BOBLIGHT_FUNCTION(int,         boblight_setuse,           (void* vpboblight, int lightnr, int on));
BOBLIGHT_FUNCTION(int,         boblight_setvalue,         (void* vpboblight, int lightnr, float value));
BOBLIGHT_FUNCTION(int,         boblight_setsaturation,    (void* vpboblight, int lightnr, float saturation));
BOBLIGHT_FUNCTION(int,         boblight_setvaluerange,    (void* vpboblight, int lightnr, float* valuerange));
BOBLIGHT_FUNCTION(int,         boblight_setthreshold,     (void* vpboblight, int lightnr, int threshold));

BOBLIGHT_FUNCTION(void,        boblight_setscanrange,     (void* vpboblight, int width, int height));

BOBLIGHT_FUNCTION(void,        boblight_addpixel,         (void* vpboblight, int lightnr, int* rgb));
BOBLIGHT_FUNCTION(void,        boblight_addpixelxy,       (void* vpboblight, int x, int y, int* rgb));

BOBLIGHT_FUNCTION(int,         boblight_sendrgb,          (void* vpboblight));
BOBLIGHT_FUNCTION(int,         boblight_ping,             (void* vpboblight));
