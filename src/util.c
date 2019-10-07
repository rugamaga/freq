#include <stdio.h>

#include "util.h"

void indent(size_t level) {
  for( size_t i = 0; i < level; ++i )
    fprintf(stderr, "  ");
}
