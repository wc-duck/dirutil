/*
    A small drop-in library providing some functions related to directories.

    version 0.1, April, 2015

    Copyright (C) 2015- Fredrik Kihlander

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

    Fredrik Kihlander
*/

#include <dirutil/dirutil.h>
#include <stdio.h>

int dir_walk_print( const char* path, dir_item_type type, void* )
{
	printf( "%s %s\n", type == DIR_ITEM_FILE ? "FILE:" : "DIR: ", path );
	return 0;
}

int main( int argc, const char** argv )
{
	return dir_walk( argc > 1 ? argv[1] : ".", DIR_WALK_DEPTH_FIRST, dir_walk_print, 0 ) == DIR_ERROR_OK;
}