
#define BOBLIGHT_DLOPEN_EXTERN
#include "lib/libboblight.h"

const char* test(void* vpboblight)
{
  return boblight_geterror(vpboblight);
}
