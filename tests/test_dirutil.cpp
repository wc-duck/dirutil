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

#include "greatest.h"
#include <dirutil/dirutil.h>

#include <stdio.h>
#include <stdint.h>
#if defined( _WIN32 )
#  include <windows.h>
#else
#  include <sys/stat.h>
#endif

static bool path_exists( const char* path )
{
#if defined( _WIN32 )
	return GetFileAttributes( path ) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat st;
	return stat( path, &st ) == 0;
#endif
}

static bool filedump( const char* path, const uint8_t* data, size_t data_size )
{
	FILE* f = fopen( path, "wb" );
	if( f == 0x0 )
		return false;

	bool success = fwrite( data, 1, data_size, f ) == data_size;
	fclose( f );
	return success;
}

TEST create_remove_tree()
{
	ASSERT( path_exists( "local" ) );
	ASSERT( !path_exists( "local/apa" ) );

	dir_error err;
	err = dir_mktree( "local/apa/bepa/cepa");
	ASSERT_EQ( DIR_ERROR_OK, err );

	ASSERT( path_exists( "local" ) );
	ASSERT( path_exists( "local/apa" ) );
	ASSERT( path_exists( "local/apa/bepa" ) );
	ASSERT( path_exists( "local/apa/bepa/cepa" ) );

	err = dir_rmtree( "local/apa" );
	ASSERT_EQ( DIR_ERROR_OK, err );
	ASSERT( path_exists( "local" ) );
	ASSERT( !path_exists( "local/apa" ) );
	return 0;
}

TEST create_remove_tree_slash()
{
	ASSERT( path_exists( "local" ) );
	ASSERT( !path_exists( "local/apa" ) );

	dir_error err;
	err = dir_mktree( "local/apa/bepa/cepa");
	ASSERT_EQ( DIR_ERROR_OK, err );

	ASSERT( path_exists( "local" ) );
	ASSERT( path_exists( "local/apa" ) );
	ASSERT( path_exists( "local/apa/bepa" ) );
	ASSERT( path_exists( "local/apa/bepa/cepa" ) );

	err = dir_rmtree( "local/apa/" );
	ASSERT_EQ( DIR_ERROR_OK, err );
	ASSERT( path_exists( "local" ) );
	ASSERT( !path_exists( "local/apa" ) );
	return 0;
}


TEST create_remove_tree_with_files()
{
	ASSERT( path_exists( "local" ) );
	ASSERT( !path_exists( "local/apa" ) );

	dir_error err;
	err = dir_mktree( "local/apa/bepa/cepa");
	ASSERT_EQ( DIR_ERROR_OK, err );

	filedump( "local/apa/file.txt", (uint8_t*)"abc", 4 );
	filedump( "local/apa/bepa/file.txt", (uint8_t*)"abc", 4 );
	filedump( "local/apa/bepa/cepa/file.txt", (uint8_t*)"abc", 4 );

	ASSERT( path_exists( "local" ) );
	ASSERT( path_exists( "local/apa" ) );
	ASSERT( path_exists( "local/apa/bepa" ) );
	ASSERT( path_exists( "local/apa/bepa/cepa" ) );

	err = dir_rmtree( "local/apa" );
	ASSERT_EQ( DIR_ERROR_OK, err );
	ASSERT( path_exists( "local" ) );
	ASSERT( !path_exists( "local/apa" ) );
	return 0;
}


GREATEST_SUITE( dirutil )
{
	RUN_TEST( create_remove_tree );
	RUN_TEST( create_remove_tree_slash );
	RUN_TEST( create_remove_tree_with_files );
}

GREATEST_MAIN_DEFS();

int main( int argc, char **argv )
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE( dirutil );
    GREATEST_MAIN_END();
}
