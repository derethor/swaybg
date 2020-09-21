#include <assert.h>

# include <glob.h>
# include "swaybg.h"

# define LIST_GLOB_FLAGS GLOB_NOSORT

void list_files (const char * path , struct swaybg_state * state )
{

  glob_t  globlist;
  int     c;

  assert ( state!= NULL);
  assert ( path != NULL);

	if (glob( path, LIST_GLOB_FLAGS , NULL, &globlist) == GLOB_NOSPACE || glob( path, GLOB_PERIOD, NULL, &globlist) == GLOB_NOMATCH)
		return ; // (FAILURE);

	if (glob( path, LIST_GLOB_FLAGS , NULL, &globlist) == GLOB_ABORTED)
		return ; // (ERROR);

  c = 0;

	while (globlist.gl_pathv[c]) c ++;


	uint32_t value = (( state->seed * 1103515245) + 12345) & 0x7fffffff;
	state->seed = value;
	const uint32_t e =	(uint32_t) ((  ((float ) (value) ) / 2147483646.0f ) * ( (float) c ));

	printf("%s\n", globlist.gl_pathv[e]);
}

