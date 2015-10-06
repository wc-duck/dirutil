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

#include <string.h>
#include <sys/stat.h>

#if defined( _WIN32 )
	#include <direct.h>
	#include <windows.h>
#else
	#include <errno.h>
	#include <unistd.h>
	#include <dirent.h>
#endif

#if defined( _WIN32 )

static dir_error dir_walk_impl( char* path_buffer, size_t path_len, size_t path_buffer_size, WIN32_FIND_DATA* ffd, dir_walk_flags flags, dir_walk_callback callback, void* userdata )
{
	if( path_buffer_size < 3 )
		return DIR_ERROR_PATH_TO_DEEP;

	path_buffer[path_len]   = '/';
	path_buffer[path_len+1] = '*';
	path_buffer[path_len+2] = '\0';

	HANDLE ffh = FindFirstFile( path_buffer, ffd );
	if( ffh == INVALID_HANDLE_VALUE )
		return DIR_ERROR_PATH_DO_NOT_EXIST;

	do
	{
		const char* item_name = ffd->cFileName;
		bool is_dir = ffd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		// TODO: flag to ignore dot-files?
		if( strcmp( item_name, "." ) == 0 || strcmp( item_name, ".." ) == 0 )
			continue;
		size_t item_len = strlen( item_name );
		if( path_buffer_size < item_len + 1 )
			return DIR_ERROR_PATH_TO_DEEP;
		path_buffer[path_len] = '/';
		strcpy( &path_buffer[path_len + 1], item_name );
		if( is_dir )
		{
			if( flags & DIR_WALK_DEPTH_FIRST )
			{
				dir_walk_impl( path_buffer, path_len + item_len + 1, path_buffer_size - item_len - 1, ffd, flags, callback, userdata );
				callback( path_buffer, DIR_ITEM_DIR, userdata );
			}
			else
			{
				callback( path_buffer, DIR_ITEM_DIR, userdata );
				dir_walk_impl( path_buffer, path_len + item_len + 1, path_buffer_size - item_len - 1, ffd, flags, callback, userdata );
			}
		}
		else
			callback( path_buffer, DIR_ITEM_FILE, userdata );
	}
	while( FindNextFile( ffh, ffd ) != 0 );
	path_buffer[path_len] = '\0';

	FindClose( ffh );
	return DIR_ERROR_OK;
}

dir_error dir_walk( const char* path, dir_walk_flags flags, dir_walk_callback callback, void* userdata )
{
	char path_buffer[4096];
	strncpy( path_buffer, path, sizeof( path_buffer ) );
	size_t path_len = strlen( path_buffer );
	WIN32_FIND_DATA ffd;
	return dir_walk_impl( path_buffer, path_len, sizeof( path_buffer ) - path_len, &ffd, flags, callback, userdata );
}

#else

static dir_error dir_walk_impl( char* path_buffer, size_t path_len, size_t path_buffer_size, unsigned int flags, dir_walk_callback callback, void* userdata )
{
	DIR* dir = opendir( path_buffer );
	if( dir == 0x0 )
		return DIR_ERROR_PATH_DO_NOT_EXIST;

	struct dirent* ent;
	while( ( ent = readdir( dir ) ) != 0x0 )
	{
		const char* item_name = ent->d_name;

		// TODO: flag to ignore dot-files?
		if( strcmp( item_name, "." ) == 0 || strcmp( item_name, ".." ) == 0 )
			continue;

		size_t item_len = strlen( item_name );
		if( path_buffer_size < item_len + 1 )
			return DIR_ERROR_PATH_TO_DEEP;

		// TODO: check for overflow!
		path_buffer[path_len] = '/';
		strcpy( &path_buffer[path_len + 1], item_name );

		bool is_dir = ent->d_type == DT_DIR;
		if( ent->d_type == DT_UNKNOWN )
		{
			struct stat s;
			if( stat( path_buffer, &s ) != 0 )
				return DIR_ERROR_FAILED;
			is_dir = S_ISDIR( s.st_mode );
		}

		if( is_dir )
		{
			if( flags & DIR_WALK_DEPTH_FIRST )
			{
				dir_walk_impl( path_buffer, path_len + item_len + 1, path_buffer_size - item_len - 1, flags, callback, userdata );
				callback( path_buffer, DIR_ITEM_DIR, userdata );
			}
			else
			{
				callback( path_buffer, DIR_ITEM_DIR, userdata );
				dir_walk_impl( path_buffer, path_len + item_len + 1, path_buffer_size - item_len - 1, flags, callback, userdata );
			}
		}
		else
			callback( path_buffer, DIR_ITEM_FILE, userdata );

		path_buffer[path_len] = '\0';
	}
	closedir( dir );
	return DIR_ERROR_OK;
}

dir_error dir_walk( const char* path, unsigned int flags, dir_walk_callback callback, void* userdata )
{
	char path_buffer[4096];
	strncpy( path_buffer, path, sizeof( path_buffer ) );
	size_t path_len = strlen( path_buffer );
	return dir_walk_impl( path_buffer, path_len, sizeof( path_buffer ) - path_len, flags, callback, userdata );
}

#endif

dir_error dir_create( const char* path )
{
#if defined( _WIN32 )
	if( CreateDirectory( path, 0x0 ) )
		return DIR_ERROR_OK;
	if( GetLastError() == ERROR_ALREADY_EXISTS )
		return DIR_ERROR_OK;
#else
	if( mkdir( path, 0777 ) == 0 )
		return DIR_ERROR_OK;
	if( errno == EEXIST )
		return DIR_ERROR_OK;
#endif
	return DIR_ERROR_FAILED;
}

static int dir_walk_rmitem( const char* path, dir_item_type type, void* userdata )
{
	switch( type )
	{
		case DIR_ITEM_FILE:
			#if defined( _WIN32 )
				if( !DeleteFile( path ) )
					*((dir_error*)userdata) = DIR_ERROR_FAILED;
			#else
				if( unlink( path ) != 0 )
					*((dir_error*)userdata) = DIR_ERROR_FAILED;
			#endif
			break;
		case DIR_ITEM_DIR:
			if( rmdir( path ) != 0 )
				*((dir_error*)userdata) = DIR_ERROR_FAILED;
			break;
		default:
			break;
	}
	return *((dir_error*)userdata) == DIR_ERROR_OK ? 0 : 1;
}

dir_error dir_rmtree( const char* path )
{
	dir_error res = DIR_ERROR_OK;
	dir_error e = dir_walk( path, DIR_WALK_DEPTH_FIRST, dir_walk_rmitem, &res );
	if( e != DIR_ERROR_OK )
		return e;
	if( res != DIR_ERROR_OK )
		return res;
	return rmdir( path ) == 0 ? DIR_ERROR_OK : DIR_ERROR_FAILED;
}

dir_error dir_mktree( const char* path )
{
	char path_buffer[4096];
	strncpy( path_buffer, path, sizeof( path_buffer ) );

	char* beg = path_buffer;
	while( true )
	{
		char* sep = strchr( beg, '/' );
		if( sep == 0 )
			return dir_create( path_buffer );

		*sep = '\0';
		dir_error err = dir_create( path_buffer );
		if( err != DIR_ERROR_OK )
			return err;
		*sep = '/';
		beg = sep + 1;
	}
	return DIR_ERROR_FAILED;
}

static int dir_glob_match_range( const char* range_start, const char* range_end, char match_char )
{
	int match_return = 1;
	if( *range_start == '!' )
	{
		match_return = 0;
		++range_start;
	}

	while( range_start != range_end + 1 )
	{
		if( range_start[1] == '-' )
		{
			if( range_start[0] <= match_char && match_char <= range_start[2] )
				return match_return;
			range_start += 3;
		}
		else
		{
			if( *range_start == match_char )
				return match_return;
			++range_start;
		}
	}

	return !match_return;
}

static int dir_glob_match_groups( const char* group_start, const char* group_end, const char* match_this )
{
	// ... comma separated ...
	const char* item_start = group_start;
	while( item_start < group_end )
	{
		const char* item_end = item_start;
		while( *item_end != ',' && item_end != group_end + 1 )
			++item_end;

		size_t item_len = (size_t)(item_end - item_start);
		if( strncmp( match_this, item_start, item_len ) == 0 )
			return (int)item_len;

		item_start = item_end + 1;
	}
	return -1;
}

#include <stdio.h>

static dir_glob_result dir_glob_match( const char* glob_pattern, const char* glob_end, const char* path )
{
	const char* unverified = path;

	while( glob_pattern != glob_end )
	{
		switch( *glob_pattern )
		{
			case '\0':
				return DIR_GLOB_NO_MATCH;
			case '*':
			{
				switch( glob_pattern[1] )
				{
					case '\0':
					{
						// try to find a path-separator in unverified since a pattern without a path-separator should only match files.
						if( strchr( unverified, '/' ) )
							return DIR_GLOB_NO_MATCH; // should match dirs...
						return DIR_GLOB_MATCH;
					}

					case '*':
					{
						if( glob_pattern[2] != '/' )
							return DIR_GLOB_INVALID_PATTERN; // invalid?

						for( const char* sub_search = unverified - 1;
							 sub_search;
							 sub_search = strchr( sub_search + 1, '/' ) )
						{
							++sub_search;
							dir_glob_result res = dir_glob_match( glob_pattern + 3, glob_end, sub_search );
							if( res != DIR_GLOB_NO_MATCH )
								return res;
						}
						return DIR_GLOB_NO_MATCH;
					}

					default:
					{
						// search in unverified for char
						const char* next = unverified;
						while( *next && ( *next != glob_pattern[1] ) && ( *next != '/' ) )
							++next;

						switch( *next )
						{
							case '\0':
								return DIR_GLOB_NO_MATCH; // failed, could not find char after *
							case '/':
								if( glob_pattern[1] == '/' )
								{
									unverified = next + 1;
									glob_pattern += 2;
								}
								else
									return DIR_GLOB_NO_MATCH;
								break;
							default:
								unverified = next;
								++glob_pattern;
								break;
						}
					}
					break;

				}
			}
			break;
			case '?':
			{
				if( *unverified == '/' )
					return DIR_GLOB_NO_MATCH;
				++unverified;
				++glob_pattern;
			}
			break;

			case '[':
			{
				const char* range_start = glob_pattern + 1;
				const char* range_end   = strchr( range_start, ']' );
				if( range_end == 0x0 )
					return DIR_GLOB_INVALID_PATTERN; // invalid

				if( !dir_glob_match_range( range_start, range_end - 1, *unverified ) )
					return DIR_GLOB_NO_MATCH;

				glob_pattern = range_end + 1;
				++unverified;
			}
			break;

			case '{':
			{
				const char* group_start = glob_pattern + 1;
				const char* group_end   = group_start;
				while( group_end != glob_end && *group_end != '}' )
					++group_end;

				int match_len = dir_glob_match_groups( group_start, group_end - 1, unverified );
				if( match_len < 0 )
					return DIR_GLOB_NO_MATCH;

				glob_pattern = group_end + 1;
				unverified += match_len;
			}
			break;

			default:
			{
				if( *unverified != *glob_pattern )
					return DIR_GLOB_NO_MATCH;
				++unverified;
				++glob_pattern;
			}
			break;
		}
	}

	return *unverified == '\0' ? DIR_GLOB_MATCH : DIR_GLOB_NO_MATCH;
}

dir_glob_result dir_glob_match( const char* glob_pattern, const char* path )
{
	return dir_glob_match( glob_pattern, glob_pattern + strlen(glob_pattern), path );
}
