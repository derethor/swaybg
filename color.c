#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include "log.h"

uint32_t parse_color(const char *color)
{
  assert(color != NULL);

  if (color[0] == '#')
  {
    ++color;
  }

  int len = strlen(color);
  if (len != 6 && len != 8)
  {
    swaybg_log(LOG_DEBUG, "Invalid color %s, defaulting to 0xFFFFFFFF",
               color);
    return 0xFFFFFFFF;
  }
  uint32_t res = (uint32_t)strtoul(color, NULL, 16);
  if (strlen(color) == 6)
  {
    res = (res << 8) | 0xFF;
  }
  return res;
}

bool is_valid_color(const char *color)
{
  assert(color != NULL);
  int len = strlen(color);
  if (len != 7 || color[0] != '#')
  {
    swaybg_log(LOG_ERROR, "%s is not a valid color for swaybg. "
                          "Color should be specified as #rrggbb (no alpha).",
               color);
    return false;
  }

  int i;
  for (i = 1; i < len; ++i)
  {
    if (!isxdigit(color[i]))
    {
      return false;
    }
  }

  return true;
}
