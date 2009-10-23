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

#include "util/log.h"
#include "util/misc.h"
#include "devicesound.h"

CDeviceSound::CDeviceSound(CClientsHandler& clients) : CDevice(clients)
{
}

bool CDeviceSound::SetupDevice()
{
  log("sound device not implemented");
  return false;
}

bool CDeviceSound::WriteOutput()
{
  return false;
}

void CDeviceSound::CloseDevice()
{
  return;
}

