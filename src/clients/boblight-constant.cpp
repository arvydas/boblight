/*
 * boblight
 * Copyright (C) Bob  2009 
 * 
 * boblight is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * boblight is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOBLIGHT_DLOPEN
#include "../lib/libboblight.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
  char* boblight_error = boblight_loadlibrary(NULL);
  if (boblight_error)
  {
    cout << boblight_error << "\n";
    return 1;
  }

  int rgb[3] = {255, 128, 64};

  while(1)
  {
    void* boblight = boblight_init();

    if (!boblight_connect(boblight, NULL, -1, 5000000))
    {
      cout << boblight_geterror(boblight) << "\n";
      boblight_destroy(boblight);
      sleep(10);
      continue;
    }

    boblight_addpixel(boblight, -1, rgb);
    if (!boblight_sendrgb(boblight) || !boblight_setpriority(boblight, 128))
    {
      cout << boblight_geterror(boblight) << "\n";
      boblight_destroy(boblight);
      continue;
    }

    while(1)
    {
      if (!boblight_ping(boblight))
      {
        cout << boblight_geterror(boblight) << "\n";
        boblight_destroy(boblight);
        break;
      }
      sleep(10);
    }
  }
}