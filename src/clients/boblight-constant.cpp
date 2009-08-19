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
#include <string>
#include <vector>

#include "config.h"
#include "util/misc.h"

using namespace std;

void PrintHelpMessage();
bool ParseBoblightFlags(int argc, char *argv[], vector<string>& options, int& priority, string& address, int& port, bool& list, bool& help);
void ListBoblightOptions();

int main(int argc, char *argv[])
{
  char* boblight_error = boblight_loadlibrary(NULL);
  if (boblight_error)
  {
    PrintError(boblight_error);
    return 1;
  }

  bool   list = false;
  bool   help = false;
  int    priority = 128;
  string address;
  int    port = -1;
  vector<string> options;

  if (!ParseBoblightFlags(argc, argv, options, priority, address, port, list, help))
  {
    PrintHelpMessage();
    return 1;
  }

  if (help)
  {
    PrintHelpMessage();
    return 1;
  }
  else if (list)
  {
    ListBoblightOptions();
    return 1;
  }

  return 0;
  
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

void PrintHelpMessage()
{
  cout << "\n";
  cout << "boblight-constant " << VERSION << "\n";
  cout << "\n";
  cout << "Usage: boblight-constant [OPTION] color\n";
  cout << "\n";
  cout << "  color is in RRGGBB hex notation\n";
  cout << "\n";
  cout << "  options:\n";
  cout << "\n";
  cout << "  -p  priority, from 0 to 255\n";
  cout << "  -s  address:[port], set the address and optional port to connect to\n";
  cout << "  -o  add libboblight option line\n";
  cout << "  -l  list libboblight option lines\n";
  cout << "\n";
}

bool ParseBoblightFlags(int argc, char *argv[], vector<string>& options, int& priority, string& address, int& port, bool& list, bool& help)
{
  string option;
  int c;

  opterr = 0; //we don't want to print error messages
  
  while ((c = getopt (argc, argv, "p:s:o:lh")) != -1)
  {
    if (c == 'p')
    {
      option = optarg;
      if (!StrToInt(option, priority) || priority < 0 || priority > 255)
      {
        PrintError("Wrong option " + static_cast<string>(optarg) + " for argument -p");
        return false;
      }
    }
    else if (c == 's')
    {
      option = optarg;
      address = option.substr(0, option.find(':'));

      if (option.find(':') != string::npos)
      {
        option = option.substr(option.find(':') + 1);
        string word;
        if (!StrToInt(option, port) || port < 0 || port > 65535)
        {
          PrintError("Wrong option " + static_cast<string>(optarg) + " for argument -s");
          return false;
        }
      }
    }
    else if (c == 'o')
    {
      options.push_back(optarg);
    }
    else if (c == 'l')
    {
      list = true;
    }
    else if (c == 'h')
    {
      help = true;
    }
    else if (c == '?')
    {
      if (optopt == 'p' || optopt == 's' || optopt == 'o')
      {
        string error = "Option ";
        error += static_cast<char>(optopt);
        error += " requires an argument";
        PrintError(error);
        return false;
      }
    }
  }

  return true;
}

void ListBoblightOptions()
{
  void* boblight = boblight_init();
  int nroptions = boblight_getnroptions(boblight);

  for (int i = 0; i < nroptions; i++)
  {
    cout << boblight_getoptiondescript(boblight, i) << "\n";
  }

  boblight_destroy(boblight);
}