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

#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <sstream>
#include <vector>

#include "log.h"
#include "mutex.h"
#include "lock.h"
#include "misc.h"

using namespace std;

bool logtostdout = true;
bool printlogtofile = true;

static CMutex   g_logmutex;
static ofstream g_logfile;

//returns hour:minutes:seconds
string GetStrTime()
{
  char buff[100];
  time_t now = time(NULL);

  strftime(buff, 99, "%H:%M:%S", localtime(&now));
  return buff;
}

bool InitLog(string filename, ofstream& logfile)
{
  if (!getenv("HOME"))
  {
    PrintError("$HOME is not set");
    return false;
  }
  
  string directory = static_cast<string>(getenv("HOME")) + "/.boblight/";
  string fullpath = directory + filename;

  //try to make the directory the log goes in
  if (mkdir(directory.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
  {
    //if it already exists we're ok
    if (errno != EEXIST)
    {
      PrintError("unable to make directory " + directory + ":\n" + GetErrno());
      return false;
    }
  }

  //rename the previous logfile
  rename(fullpath.c_str(), (fullpath + ".old").c_str());

  //open the logfile
  logfile.open(fullpath.c_str());
  if (logfile.fail())
  {
    PrintError("unable to open " + fullpath + ":\n" + GetErrno());
    return false;
  }

  log("start of log %s", fullpath.c_str());

  return true;
}

//we only want the function name and the namespace, so we search for '(' and get the string before that
string PruneFunction(string function)
{
  size_t parenpos = function.find('(');
  size_t spacepos = function.rfind(' ', parenpos);

  return function.substr(spacepos + 1, parenpos - spacepos - 1);
}

void SetLogFile(std::string filename)
{
  CLock lock(g_logmutex);

  if (!g_logfile.is_open())
  {
    if (!InitLog(filename, g_logfile))
    {
      printlogtofile = false;
    }
  }
}

void PrintLog(const char* fmt, const char* function, ...)
{
  string                logstr;
  string                funcstr;
  va_list               args;
  int                   nrspaces;

  static int            logbuffsize = 128; //size of the buffer
  static char*          logbuff = reinterpret_cast<char*>(malloc(logbuffsize)); //buffer for vsnprintf
  static vector<string> logstrings;      //we save any log lines here while the log isn't open

  CLock lock(g_logmutex);
  
  va_start(args, function);

  //print to the logbuffer and check if our buffer is large enough
  int neededbuffsize = vsnprintf(logbuff, logbuffsize, fmt, args);
  if (neededbuffsize + 1 > logbuffsize)
  {
    logbuffsize = neededbuffsize + 1;
    logbuff = reinterpret_cast<char*>(realloc(logbuff, logbuffsize)); //resize the buffer to the needed size
    vsnprintf(logbuff, logbuffsize, fmt, args);                       //write to the buffer again
  }
  
  va_end(args);

  logstr = GetStrTime() + " " + logbuff;
  funcstr = PruneFunction(function);

  //we try to place the function name at the right of an 80 char terminal
  nrspaces = 80 - (logstr.size() + funcstr.size() + 3);

  //add the spaces
  if (nrspaces > 0) logstr.insert(logstr.size(), nrspaces, ' ');
  
  logstr += " (" + funcstr + ")\n";


  if (g_logfile.is_open() && printlogtofile)
  {
    //print any saved log lines
    if (!logstrings.empty())
    {
      for (int i = 0; i < logstrings.size(); i++)
      {
        g_logfile << logstrings[i] << flush;
      }
      logstrings.clear();
    }
    //write the string to the logfile
    g_logfile << logstr << flush;
  }
  else if (printlogtofile)
  {
    //save the log line if the log isn't open yet
    logstrings.push_back(logstr);
  }

  //print to stdout when requested
  if (logtostdout) cout << logstr << flush;
}