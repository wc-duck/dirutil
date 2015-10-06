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

TEST dir_glob_match_simple()
{
	// TODO: split in multiple tests

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "apa.txt", "apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "bpa.txt", "apa.txt" ) );

	return 0;
}

TEST dir_glob_match_star()
{
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "*.txt",   "apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "*.txt",   "apa.who" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a*a.txt", "apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a*a.txt", "bpa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a*a.txt", "apb.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a*.txt",  "apb.txtb" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "*.h",     "src/bloo.cpp" ) );

	return 0;
}

TEST dir_glob_match_simple_dir()
{
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "p1/*.txt", "p1/apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "p1/*.txt", "apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "p1/*.txt", "p1" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "p1/*.txt", "p" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "p*",  "p1" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "p*",  "p1/" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "p*",  "p1/apa.txt" ) ); // should not match across directories.
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "p*/", "p1/" ) );

	return 0;
}

TEST dir_glob_match_single_char()
{
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a?a", "apa" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a?a", "apb" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a?a/apa", "apa/apa" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a?a/apa", "apb/apa" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a?a/", "apa/" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a?a/", "apa" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a?/", "ap/" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a?/", "ap" ) );

	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "ap?a", "ap/a" ) ); // should not match dir-separator.

	return 0;
}

TEST dir_glob_match_multi_dir()
{
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "**/*.txt",   "src/apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "**/*.cpp",   "./src/apa.cpp" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "**/apa.txt", "apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "**/apa.txt", "a/apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "**/apa.txt", "a/b/apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "**/apa.txt", "a/b/c/apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "**/apa.txt", "a/apa.taxt" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a/**/apa.txt", "a/b/apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a/**/apa.txt", "b/a/apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a/**/apa.txt", "a/b/c/apa.txt" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a/**/b/**/apa.txt", "a/b/apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a/**/b/**/apa.txt", "a/c/d/b/a/apa.txt" ) );
	return 0;
}

TEST dir_glob_match_range()
{
	// TODO: support [0-9] etc
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[pb]a.txt", "apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[pb]a.txt", "aba.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[pb]a.txt", "aca.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[pb]a.txt", "apba.txt" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[a-d]a.txt", "aba.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[a-d]a.txt", "aca.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[a-d]a.txt", "ada.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[a-d]a.txt", "afa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[a-d]a.txt", "aBa.txt" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[0-9]a.txt", "a0a.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[0-9]a.txt", "a3a.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[0-9]a.txt", "a9a.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[0-9]a.txt", "apa.txt" ) );
	return 0;
}

TEST dir_glob_match_negative_range()
{
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[!pb]a.txt", "apa.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[!pb]a.txt", "aba.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[!pb]a.txt", "aca.txt" ) );

	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[!a-d]a.txt", "aba.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[!a-d]a.txt", "aca.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[!a-d]a.txt", "ada.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[!a-d]a.txt", "afa.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[!a-d]a.txt", "aBa.txt" ) );

	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[!0-9]a.txt", "a0a.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[!0-9]a.txt", "a3a.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a[!0-9]a.txt", "a9a.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a[!0-9]a.txt", "apa.txt" ) );
	return 0;
}

TEST dir_glob_match_ecaped_chars()
{
	return 0;
}

TEST dir_glob_match_brackets()
{
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a{.f1,.f2}", "a.f1" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a{.f1,.f2}", "a.f2" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a{.f1,.f2}", "a.f3" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "{a1,a2}.txt", "a1.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "{a1,a2}.txt", "a2.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "{a1,a2}.txt", "a3.txt" ) );

	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a{a1,a2}.txt", "aa1.txt" ) );
	ASSERT_EQ( DIR_GLOB_MATCH,    dir_glob_match( "a{a1,a2}.txt", "aa2.txt" ) );
	ASSERT_EQ( DIR_GLOB_NO_MATCH, dir_glob_match( "a{a1,a2}.txt", "ba1.txt" ) );

	// TODO: test sub *,?,[]
	return 0;
}

TEST dir_glob_match_invalid_pattern()
{
	// TODO: add tests for and handle invalid patterns!
	return 0;
}

GREATEST_SUITE( dirutil )
{
	RUN_TEST( create_remove_tree );
	RUN_TEST( create_remove_tree_slash );
	RUN_TEST( create_remove_tree_with_files );
}

GREATEST_SUITE( glob )
{
	RUN_TEST( dir_glob_match_simple );
	RUN_TEST( dir_glob_match_star );
	RUN_TEST( dir_glob_match_simple_dir );
	RUN_TEST( dir_glob_match_single_char );
	RUN_TEST( dir_glob_match_multi_dir );
	RUN_TEST( dir_glob_match_range );
	RUN_TEST( dir_glob_match_negative_range );
	RUN_TEST( dir_glob_match_ecaped_chars );
	RUN_TEST( dir_glob_match_brackets );
	RUN_TEST( dir_glob_match_invalid_pattern );
}

GREATEST_MAIN_DEFS();

int main( int argc, char **argv )
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE( dirutil );
    RUN_SUITE( glob );
    GREATEST_MAIN_END();
}
