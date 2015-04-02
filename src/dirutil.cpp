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
		bool is_dir = ent->d_type == DT_DIR;

		// TODO: flag to ignore dot-files?
		if( strcmp( item_name, "." ) == 0 || strcmp( item_name, ".." ) == 0 )
			continue;

		size_t item_len = strlen( item_name );
		if( path_buffer_size < item_len + 1 )
			return DIR_ERROR_PATH_TO_DEEP;

		// TODO: check for overflow!
		path_buffer[path_len] = '/';
		strcpy( &path_buffer[path_len + 1], item_name );
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
#include <stdio.h>
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
				{
					printf("failed to remove file %s\n", path);
					*((dir_error*)userdata) = DIR_ERROR_FAILED;
				}
			#endif
			break;
		case DIR_ITEM_DIR:
			if( rmdir( path ) != 0 )
			{
				printf("failed to remove %s\n", path);
				*((dir_error*)userdata) = DIR_ERROR_FAILED;
			}
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
	if( rmdir( path ) == 0 )
		return DIR_ERROR_OK;
	printf("failed to remove %s\n", path);
	return DIR_ERROR_FAILED;
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
