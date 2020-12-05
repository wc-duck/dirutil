[![Build Status](https://travis-ci.org/wc-duck/dirutil.svg?branch=master)](https://travis-ci.org/wc-duck/dirutil)
[![Build status](https://ci.appveyor.com/api/projects/status/gu9luwtutq3lkkob?svg=true)](https://ci.appveyor.com/project/wc-duck/dirutil)

# dirtools
small collection of directory related functions.

# examples

## Print directory recursively.

```c
	#include <dirutil/dirutil.h>
	#include <stdio.h>
	
	int dir_walk_print( const dir_walk_item* item )
	{
		printf( "%s %s\n", item->type == DIR_ITEM_FILE ? "FILE:" : "DIR: ", item->path );
		return 0;
	}
	
	int main( int argc, const char** argv )
	{
		return dir_walk( argc > 1 ? argv[1] : ".", DIR_WALK_DEPTH_FIRST, dir_walk_print, 0 ) == DIR_ERROR_OK;
	}
```

```c++
	#include <dirutil/dirutil.h>
	#include <stdio.h>

	// if compiled as c++ there is a helper-function to call dir_walk as well.
	int main( int argc, const char** argv )
	{
		return dir_walk( argc > 1 ? argv[1] : ".", DIR_WALK_DEPTH_FIRST,
			[](const dir_walk_item* item) {
				printf( "%s %s\n", type == DIR_ITEM_FILE ? "FILE:" : "DIR: ", path );
				return 1;
			}]) == DIR_ERROR_OK;
	}
```

## Print files matching glob-pattern.

```c
	#include <dirutil/dirutil.h>
	#include <stdio.h>
	
	int dir_walk_print( const dir_walk_item* item )
	{
		if( dir_glob_match( "**/*.txt", item->path ) == DIR_GLOB_MATCH )
			printf( "%s %s\n", item->type == DIR_ITEM_FILE ? "FILE:" : "DIR: ", item->path );
		return 0;
	}
	
	int main( int argc, const char** argv )
	{
		return dir_walk( argc > 1 ? argv[1] : ".", DIR_WALK_DEPTH_FIRST, dir_walk_print, 0 ) == DIR_ERROR_OK;
	}
```
