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

#ifndef FILE_DIR_H_INCLUDED
#define FILE_DIR_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

enum dir_error
{
	DIR_ERROR_OK,

	DIR_ERROR_FAILED,
	DIR_ERROR_PATH_TO_DEEP,
	DIR_ERROR_PATH_IS_FILE,
	DIR_ERROR_PATH_DO_NOT_EXIST
};

enum dir_walk_flags
{
	DIR_WALK_NO_FLAGS    = 0,
	DIR_WALK_DEPTH_FIRST = 1 << 1
};

enum dir_item_type
{
	DIR_ITEM_FILE,
	DIR_ITEM_DIR,
	DIR_ITEM_UNHANDLED
};

enum dir_glob_result
{
	DIR_GLOB_MATCH,
	DIR_GLOB_NO_MATCH,
	DIR_GLOB_INVALID_PATTERN
};

/**
 * Item passed to callback used with dir_walk()
 */
struct dir_walk_item
{
   /**
    * path to file relative CWD, normalized to always use '/'.
    */
   const char* path;

   /**
    * path relative 'root' passed to dir_walk(), normalized to always use '/'.
    */
   const char* relative;

   /**
    * item name, such as filename or dirname.
    */
   const char* name;

   /**
    * item type!
    */
   dir_item_type type;

   /**
    * userdata passed to dir_walk().
    */
   void* userdata;
};

/**
 * Callback called for each item with dir_walk.
 * @param path full path to current item with input path to dir_walk() as a base.
 * @param type item type, file or dir.
 * @param userdata passed to dir_walk.
 */
typedef int ( *dir_walk_callback )( const dir_walk_item* item );

/**
 * Create directory.
 * @param path dir to create
 */
dir_error dir_create( const char* path );

/**
 * Remove directory recursively
 * @param path dir to remove
 *
 * @note this is not an atomic operation and if it fails it might leave the directory partly removed.
 */
dir_error dir_rmtree( const char* path );

/**
 * Create all non-existing directories in path.
 * @param path dirs to create
 */
dir_error dir_mktree( const char* path );

/**
 * Call callback once for each item in the directory and, depending on flags, it's sub-directories.
 * @param root path to walk.
 * @param flags controlling the walk.
 * @param callback called for each item in walk.
 * @param userdata passed to callback.
 */
dir_error dir_walk( const char* root, unsigned int flags, dir_walk_callback callback, void* userdata );

/**
 * Matches an unix style glob-pattern, with added support for ** from ant, vs a path.
 *
 * Rules:
 * ?  - match one char except dir-separator.
 * *  - match any amount of chars ( including the empty string ) except dir-separator.
 * ** - match zero or more path-segments.
 * [] - match one of the chars the brackets except dir-separator, - can be used to specify a range.
 *      example:
 *      	[abx], match an a, b or x ( lower case )
 *      	[0-9], match any of the chars 0,1,2,3,4,5,6,7,8,9
 * {} - match any of the ,-separated strings within the brackets.
 *      example:
 *          {.txt,.doc}
 *
 * @note {} currently do not support sub-expressions of the other types. This could be added if there
 *       is any need for it.
 *
 * For more information see http://man7.org/linux/man-pages/man7/glob.7.html
 *
 * @param glob_pattern is an glob pattern.
 * @param path path to match.
 * @return DIR_GLOB_MATCH on match, DIR_GLOB_NO_MATCH on mismatch, otherwise error-code.
 */
dir_glob_result dir_glob_match( const char* glob_pattern, const char* path );

#ifdef __cplusplus
}
#endif  // __cplusplus

#if __cplusplus >= 201103L

/**
 * Call functor once for each item in the directory and, depending on flags, it's sub-directories.
 * @param root path to walk.
 * @param flags controlling the walk.
 * @param functor to call per item.
 */
template <typename FUNC>
inline dir_error dir_walk( const char* root, unsigned int flags, FUNC&& functor)
{
   return dir_walk(root, flags, 
      [](const dir_walk_item* item) {
         return (*(FUNC*)item->userdata)(item);
      }, &functor);
}

#endif

#endif // FILE_DIR_H_INCLUDED
