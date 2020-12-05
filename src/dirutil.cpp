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
static const char* dir_walk_item_name( WIN32_FIND_DATA* ent )
{
	return ent->cFileName;
}

static bool dir_walk_item_is_dir( const char* /*path*/, WIN32_FIND_DATA* ent )
{
	return ent->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}
#else
static const char* dir_walk_item_name( struct dirent* ent )
{
	return ent->d_name;
}

static bool dir_walk_item_is_dir( const char* path, struct dirent* ent )
{
	switch(ent->d_type)
	{
		case DT_DIR:
			return true;
		case DT_UNKNOWN:
		{
			struct stat s;
			if( stat( path, &s ) != 0 )
				return false;
			return S_ISDIR( s.st_mode );
		}
		default:
			return false;
	}
}
#endif

static dir_error dir_walk_impl( size_t            root_len,
								char*             path_buffer,
								size_t            path_len,
								size_t            path_buffer_size,
							#if defined( _WIN32 )
								WIN32_FIND_DATA*  ent,
							#endif
								unsigned int      flags,
								dir_walk_callback callback,
								void*             userdata )
{
#if defined( _WIN32 )
	if( path_buffer_size < 3 )
		return DIR_ERROR_PATH_TO_DEEP;

	path_buffer[path_len]   = '/';
	path_buffer[path_len+1] = '*';
	path_buffer[path_len+2] = '\0';

	HANDLE ffh = FindFirstFile( path_buffer, ent );
	if( ffh == INVALID_HANDLE_VALUE )
		return DIR_ERROR_PATH_DO_NOT_EXIST;

	do
	{
#else
	DIR* dir = opendir( path_buffer );
	if( dir == 0x0 )
		return DIR_ERROR_PATH_DO_NOT_EXIST;

	struct dirent* ent;
	while( ( ent = readdir( dir ) ) != 0x0 )
	{
#endif
		const char* item_name = dir_walk_item_name(ent);

		if( strcmp( item_name, "." ) == 0 || strcmp( item_name, ".." ) == 0 )
			continue;

		size_t item_len = strlen( item_name );
		if( path_buffer_size < item_len + 1 )
			return DIR_ERROR_PATH_TO_DEEP;

		dir_item_type item_type = dir_walk_item_is_dir(path_buffer, ent) ? DIR_ITEM_DIR : DIR_ITEM_FILE;

		if(item_name[0] == '.')
		{
			if(item_type == DIR_ITEM_DIR  && (flags & DIR_WALK_IGNORE_DOT_DIRS) > 0)
				continue;
			if(item_type == DIR_ITEM_FILE && (flags & DIR_WALK_IGNORE_DOT_FILES) > 0)
				continue;
		}

		// TODO: check for overflow!
		path_buffer[path_len] = '/';
		strcpy( &path_buffer[path_len + 1], item_name );

		dir_walk_item item;
		item.path     = path_buffer;
		item.relative = path_buffer + root_len + 1;
		item.name     = item_name;
		item.type     = item_type;
		item.userdata = userdata;

		if( item.type == DIR_ITEM_DIR )
		{
			bool depth_first = (flags & DIR_WALK_DEPTH_FIRST) > 0;

			if( !depth_first )
				callback( &item );

			dir_walk_impl( root_len,
						   path_buffer,
						   path_len + item_len + 1,
						   path_buffer_size - item_len - 1, 
						#if defined( _WIN32 )
						   ent,
						#endif
						   flags,
						   callback,
						   userdata );

			if( depth_first )
				callback( &item );
		}
		else
			callback( &item );
#if defined( _WIN32 )
	}
	while( FindNextFile( ffh, ent ) != 0 );
	FindClose( ffh );
#else
	}
	closedir( dir );
#endif
	path_buffer[path_len] = '\0';
	return DIR_ERROR_OK;
}

dir_error dir_walk( const char* path, unsigned int flags, dir_walk_callback callback, void* userdata )
{
	char path_buffer[4096];
	strncpy( path_buffer, path, sizeof( path_buffer ) );
	size_t path_len = strlen( path_buffer );

	// normalize input path to only strip of trailing / if there is one.
	if(path_buffer[path_len-1] == '/')
	{
		--path_len;
		path_buffer[path_len] = '\0';
	}

#if defined( _WIN32 )
	WIN32_FIND_DATA ffd;
#endif
	return dir_walk_impl( path_len,
						  path_buffer,
						  path_len,
						  sizeof( path_buffer ) - path_len, 
#if defined( _WIN32 )
						  &ffd,
#endif
						  flags,
						  callback,
						  userdata );
}

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

static bool dir_walk_rmfile( const char* path )
{
#if defined( _WIN32 )
	return DeleteFile( path );
#else
	return unlink( path ) == 0;
#endif
}

static int dir_walk_rmitem( const dir_walk_item* item )
{
	dir_error* err = (dir_error*)item->userdata;
	switch( item->type )
	{
		case DIR_ITEM_FILE:
			if(!dir_walk_rmfile(item->path))
				*err = DIR_ERROR_FAILED;
			break;
		case DIR_ITEM_DIR:
			if( rmdir( item->path ) != 0 )
				*err = DIR_ERROR_FAILED;
			break;
		default:
			break;
	}
	return *err == DIR_ERROR_OK ? 0 : 1;
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

	// if path is absolute, we need to start trying to create dirs from the second '/',
	// otherwise we would try to create the dir ""
	if( *beg == '/' )
		++beg;

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
