[![Build Status](https://travis-ci.org/wc-duck/dirutil.svg?branch=master)](https://travis-ci.org/wc-duck/dirutil)
[![Build status](https://ci.appveyor.com/api/projects/status/gu9luwtutq3lkkob?svg=true)](https://ci.appveyor.com/project/wc-duck/dirutil)

# dirtools
small collection of directory related functions.

# examples

## Print directory recursively.

```c
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
```

## Print files matching glob-pattern.

```c
	#include <dirutil/dirutil.h>
	#include <stdio.h>
	
	int dir_walk_print( const char* path, dir_item_type type, void* )
	{
		if( dir_glob_match( "**/*.txt", path ) == DIR_GLOB_MATCH )
			printf( "%s %s\n", type == DIR_ITEM_FILE ? "FILE:" : "DIR: ", path );
		return 0;
	}
	
	int main( int argc, const char** argv )
	{
		return dir_walk( argc > 1 ? argv[1] : ".", DIR_WALK_DEPTH_FIRST, dir_walk_print, 0 ) == DIR_ERROR_OK;
	}
```
