#include <assert.h>

# include <glob.h>
# include "log.h"
# include "output_config.h"

# define LIST_GLOB_FLAGS GLOB_NOSORT|GLOB_PERIOD

char * next_image (struct swaybg_output_config * config )
{
  glob_t  globlist;
  int     c;

  assert ( config != NULL);
  assert ( config->path != NULL);

  int r = glob( config->path, LIST_GLOB_FLAGS , NULL, &globlist);

	if ( r == GLOB_NOSPACE || r == GLOB_NOMATCH )
  {
    swaybg_log ( LOG_ERROR , "no match for %s" , config->path );
		return NULL; // (FAILURE);
  }

	if ( r == GLOB_ABORTED)
  {
    swaybg_log ( LOG_ERROR , "error reading %s" , config->path );
		return NULL ; // (ERROR);
  }

  // count all files
  c = 0;
	while (globlist.gl_pathv[c]) c ++;

  // randomly pick one
	uint32_t value = (( config->seed * 1103515245) + 12345) & 0x7fffffff;
	config->seed = value;
	const uint32_t e =	(uint32_t) ((  ((float ) (value) ) / 2147483646.0f ) * ( (float) c ));

  // get next filename
  char * result = strdup ( globlist.gl_pathv[e] );
  globfree(&globlist);

  swaybg_log ( LOG_DEBUG , "NEXT FILE %s", result );

  return result;
}

