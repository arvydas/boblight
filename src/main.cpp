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

#include <iostream>
#include <string>
#include <signal.h>
#include <unistd.h>

#include "util/log.h"
#include "util/tcpsocket.h"
#include "util/messagequeue.h"
#include "util/timer.h"
#include "connectionhandler.h"
#include "client.h"
#include "configuration.h"
#include "device/device.h"
#include "config.h"

#define DEFAULTCONF "/etc/boblight.conf"

using namespace std;

volatile bool stop = false;

void PrintFlags(int argc, char *argv[]);
void SignalHandler(int signum);
void PrintHelpMessage();
bool ParseFlags(int argc, char *argv[], bool& help, string& configfile, bool& fork);

int main (int argc, char *argv[])
{
  //read flags
  string configfile;
  bool   help;
  bool   bfork;

  if (!ParseFlags(argc, argv, help, configfile, bfork) || help)
  {
    PrintHelpMessage();
    return 1;
  }

  if (bfork)
  {
    if (fork())
      return 0;
  }
  
  //init our logfile
  logtostdout = true;
  SetLogFile("boblightd.log");
  log("boblight revision: %s", REVISION);
  PrintFlags(argc, argv);

  //set up signal handlers
  signal(SIGTERM, SignalHandler);
  signal(SIGINT, SignalHandler);

  CConfig             config;  //class for loading and parsing config
  vector<CDevice*>    devices; //where we store devices
  vector<CAsyncTimer> timers;  //timer pool, devices with the same interval run off the same async timer
  vector<CLight>      lights;  //lights pool
  CClientsHandler     clients(lights);
  CConnectionHandler  connectionhandler(clients);

  //load and parse config
  if (!config.LoadConfigFromFile(configfile))
    return 1;
  if (!config.CheckConfig())
    return 1;
  if (!config.BuildConfig(connectionhandler, clients, devices, timers, lights))
    return 1;

  //start the connection handler
  connectionhandler.StartThread();

  //start the clients handler
  clients.StartThread();
  
  //start the timers
  log("Starting timers");
  for (int i = 0; i < timers.size(); i++)
    timers[i].StartTimer();

  //start the devices
  log("Starting devices");
  for (int i = 0; i < devices.size(); i++)
    devices[i]->StartThread();

  //keep spinning while running
  while(!stop)
  {
    sleep(1);
  }

  //signal that the devices should stop
  log("Signaling devices to stop");
  for (int i = 0; i < devices.size(); i++)
    devices[i]->AsyncStopThread();

  //signal connection handler to stop
  log("Signaling connection handler to stop");
  connectionhandler.AsyncStopThread();

  //signal clientshandler to stop
  log("Signaling clientshandler to stop");
  clients.AsyncStopThread();

  //signal timers to stop
  log("Signaling timers to stop");
  for (int i = 0; i < timers.size(); i++)
    timers[i].AsyncStopThread();

  //stop the devices
  log("Waiting for devices to stop");
  for (int i = 0; i < devices.size(); i++)
    devices[i]->StopThread();

  //stop the timers
  log("Waiting for timers to stop");
  for (int i = 0; i < timers.size(); i++)
    timers[i].StopThread();

  //stop the connection handler
  log("Waiting for connection handler to stop");
  connectionhandler.StopThread();

  //stop the clients handler
  log("Waiting for clients handler to stop");
  clients.StopThread();

  log("exiting");
  
  return 0;
}

void PrintFlags(int argc, char *argv[])
{
  string flags = "starting";
  
  for (int i = 0; i < argc; i++)
  {
    flags += " ";
    flags += argv[i];
  }

  log("%s", flags.c_str());
}

void SignalHandler(int signum)
{
  if (signum == SIGTERM)
  {
    log("caught SIGTERM");
    stop = true;
  }
  else if (signum == SIGINT)
  {
    log("caught SIGINT");
    stop = true;
  }
  else
  {
    log("caught %i", signum);
  }
}

void PrintHelpMessage()
{
  cout << "\n";
  cout << "boblightd revision:" << REVISION << "\n";
  cout << "\n";
  cout << "Usage: boblightd [OPTION]\n";
  cout << "\n";
  cout << "  options:\n";
  cout << "\n";
  cout << "  -c  set the config file, default is " << DEFAULTCONF << "\n";
  cout << "  -f  fork\n";
  cout << "\n";
}

bool ParseFlags(int argc, char *argv[], bool& help, string& configfile, bool& fork)
{
  help = false;
  fork = false;
  configfile = DEFAULTCONF;

  opterr = 0; //no getopt errors to stdout, we bitch ourselves
  int c;

  while ((c = getopt (argc, argv, "c:hf")) != -1)
  {
    if (c == 'c')
    {
      configfile = optarg;
    }
    else if (c == 'h')
    {
      help = true;
      return true;
    }
    else if (c == 'f')
    {
      fork = true;
    }
    else if (c == '?')
    {
      if (optopt == 'c')
      {
        PrintError("Option " + ToString((char)optopt) + " requires an argument");
      }
      else
      {
        PrintError("Unknown option " + ToString((char)optopt));
      }
      return false;
    }
  }

  return true;
}
