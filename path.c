#include <assert.h>

# include <glob.h>
# include "log.h"
# include "output_config.h"

# define LIST_GLOB_FLAGS GLOB_NOSORT

char * next_image (struct swaybg_output_config * config )
{
  glob_t  globlist;
  int     c;

  assert ( config != NULL);
  assert ( config->path != NULL);

	if (glob( config->path, LIST_GLOB_FLAGS , NULL, &globlist) == GLOB_NOSPACE ||
      glob( config->path, GLOB_PERIOD,      NULL, &globlist) == GLOB_NOMATCH)
  {
    swaybg_log ( LOG_ERROR , "no match for %s" , config->path );
		return NULL; // (FAILURE);
  }

	if (glob( config->path, LIST_GLOB_FLAGS , NULL, &globlist) == GLOB_ABORTED)
  {
    swaybg_log ( LOG_ERROR , "error reading %s" , config->path );
		return NULL ; // (ERROR);
  }

  c = 0;

	while (globlist.gl_pathv[c]) c ++;

	uint32_t value = (( config->seed * 1103515245) + 12345) & 0x7fffffff;
	config->seed = value;
	const uint32_t e =	(uint32_t) ((  ((float ) (value) ) / 2147483646.0f ) * ( (float) c ));

  char * result = strdup ( globlist.gl_pathv[e] );
  globfree(&globlist);

  swaybg_log ( LOG_DEBUG , "NEXT FILE %s", result );

  return result;
}

