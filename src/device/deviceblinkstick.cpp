/*
 * boblight
 * Copyright (C) tim.helloworld  2013
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

#include "config.h"
#include "deviceblinkstick.h"
#include "util/log.h"
#include "util/misc.h"
#include "util/timeutils.h"

#define BLINKSTICK_VID       0x20A0
#define BLINKSTICK_PID       0x41E5
#define BLINKSTICK_INTERFACE 0
#define BLINKSTICK_TIMEOUT   100

int get_serial_number_bs(libusb_device *dev, const libusb_device_descriptor &descriptor, char *buf, size_t size);

CDeviceBlinkstick::CDeviceBlinkstick(CClientsHandler& clients) : CDeviceUsb(clients)
{
  m_usbcontext    = NULL;
  m_devicehandle  = NULL;
  memset(m_serial, 0, sizeof(m_serial));
}

void CDeviceBlinkstick::Sync()
{
  if (m_allowsync)
    m_timer.Signal();
}

bool CDeviceBlinkstick::SetupDevice()
{
  int error;
  if ((error = libusb_init(&m_usbcontext)) != LIBUSB_SUCCESS)
  {
    LogError("%s: error setting up usb context, error:%i %s", m_name.c_str(), error, UsbErrorName(error));
    m_usbcontext = NULL;
    return false;
  }

  if (m_debug)
    libusb_set_debug(m_usbcontext, 3);

  libusb_device** devicelist;
  ssize_t         nrdevices = libusb_get_device_list(m_usbcontext, &devicelist);

  for (ssize_t i = 0; i < nrdevices; i++)
  {
    libusb_device_descriptor descriptor;
    error = libusb_get_device_descriptor(devicelist[i], &descriptor);
    if (error != LIBUSB_SUCCESS)
    {
      LogError("%s: error getting device descriptor for device %zi, error %i %s", m_name.c_str(), i, error, UsbErrorName(error));
      continue;
    }

    //try to find a usb device with the BlinkStick vendor and product ID
    if (descriptor.idVendor == BLINKSTICK_VID && descriptor.idProduct == BLINKSTICK_PID)
    {
      int busnumber = libusb_get_bus_number(devicelist[i]);
      int deviceaddress = libusb_get_device_address(devicelist[i]);

      char serial[USBDEVICE_SERIAL_SIZE];
      error = get_serial_number_bs(devicelist[i], descriptor, serial, USBDEVICE_SERIAL_SIZE);
      if (error > 0)
        Log("%s: found BlinkStick at bus %d address %d, serial is %s", m_name.c_str(), busnumber, deviceaddress, serial);
      else
        Log("%s: found BlinkStick at bus %d address %d. Couldn't get serial.", m_name.c_str(), busnumber, deviceaddress);

      if (m_devicehandle == NULL &&
          ((strlen(m_serial)>0 && strncmp(m_serial, serial, USBDEVICE_SERIAL_SIZE))
            || ( m_busnumber == -1 || m_deviceaddress == -1 || ( m_busnumber == busnumber && m_deviceaddress == deviceaddress ))))
      {
        libusb_device_handle *devhandle;

        libusb_device *device = devicelist[i];

        error = libusb_open(devicelist[i], &devhandle);
        if (error != LIBUSB_SUCCESS)
        {
          LogError("%s: error opening device, error %i %s", m_name.c_str(), error, UsbErrorName(error));
          return false;
        }

        /*
        if ((error=libusb_detach_kernel_driver(devhandle, BLINKSTICK_INTERFACE)) != LIBUSB_SUCCESS) {
          LogError("%s: error detaching interface %i, error:%i %s", m_name.c_str(), BLINKSTICK_INTERFACE, error, UsbErrorName(error));
          return false;
        }
        */

        /*
        if ((error = libusb_claim_interface(devhandle, BLINKSTICK_INTERFACE)) != LIBUSB_SUCCESS)
        {
          LogError("%s: error claiming interface %i, error:%i %s", m_name.c_str(), BLINKSTICK_INTERFACE, error, UsbErrorName(error));
          return false;
        }

        if ((error = libusb_ref_device(device)) != LIBUSB_SUCCESS)
        {
          LogError("%s: error referencing USB device, error:%i %s", m_name.c_str(), error, UsbErrorName(error));
          return false;
        }
        */

        m_devicehandle = devhandle;

        //Disable internal smoothness implementation
        DisableSmoothness();

        Log("%s: BlinkStick is initialized, bus %d device address %d", m_name.c_str(), busnumber, deviceaddress);
      }
    }
  }

  libusb_free_device_list(devicelist, 1);

  if (m_devicehandle == NULL)
  {
    if(m_busnumber == -1 || m_deviceaddress == -1)
      LogError("%s: no BlinkStick device with vid %04x and pid %04x found", m_name.c_str(), BLINKSTICK_VID, BLINKSTICK_PID);
    else
      LogError("%s: no BlinkStick device with vid %04x and pid %04x found at bus %i, address %i", m_name.c_str(), BLINKSTICK_VID, BLINKSTICK_PID, m_busnumber, m_deviceaddress);

    return false;
  }

  m_timer.SetInterval(m_interval);

  //set all leds to 0

  return true;
}

inline unsigned int Get12bitColor(CChannel *channel, int64_t now)
{
  return Clamp((unsigned int)Round64((double)channel->GetValue(now) * 4095), 0, 4095);
}

bool CDeviceBlinkstick::WriteOutput()
{
  //get the channel values from the clientshandler
  int64_t now = GetTimeUs();
  m_clients.FillChannels(m_channels, now, this);

  m_buf[0]=1;

  unsigned char idx = 1;

  //put the values in the buffer
  for (int i = 0; i < m_channels.size(); i+=3)
  {
    unsigned int r = Get12bitColor(&m_channels[i]  , now); 
    unsigned int g = Get12bitColor(&m_channels[i+1], now); 
    unsigned int b = Get12bitColor(&m_channels[i+2], now); 

    m_buf[idx++] = r & 0xff;
    m_buf[idx++] = g & 0xff;
    m_buf[idx++] = b & 0xff;
  }

  if (idx < BLINKSTICK_REPORT_SIZE)
    memset(m_buf + idx, 0, BLINKSTICK_REPORT_SIZE - idx);

  /*
  int result = libusb_control_transfer(m_devicehandle,
                                       LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS
                                       | LIBUSB_RECIPIENT_INTERFACE,
                                       0x09,
                                       (2 << 8),
                                       0x00,
                                       m_buf, BLINKSTICK_REPORT_SIZE, BLINKSTICK_TIMEOUT);
  */

  int result = libusb_control_transfer(m_devicehandle,
                                       0x20,
                                       0x09,
                                       0x01,
                                       0x00,
                                       m_buf, BLINKSTICK_REPORT_SIZE, BLINKSTICK_TIMEOUT);
  m_timer.Wait();

  return result == BLINKSTICK_REPORT_SIZE;
}

bool CDeviceBlinkstick::DisableSmoothness()
{
  /*
  unsigned char buf[2];
  buf[0]=5; // setsmoothness command
  buf[1]=0; // smoothness value
  int result = libusb_control_transfer(m_devicehandle,
                                             LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS
                                             | LIBUSB_RECIPIENT_INTERFACE,
                                             0x09,
                                             (2 << 8),
                                             0x00,
                                             buf, 2, BLINKSTICK_TIMEOUT);
  return result == 2;
  */

  return true;
}

void CDeviceBlinkstick::CloseDevice()
{
  if (m_devicehandle != NULL)
  {
    libusb_release_interface(m_devicehandle, BLINKSTICK_INTERFACE);
    libusb_attach_kernel_driver(m_devicehandle, BLINKSTICK_INTERFACE);
    libusb_close(m_devicehandle);

    m_devicehandle = NULL;
  }

  if (m_usbcontext)
  {
    libusb_exit(m_usbcontext);
    m_usbcontext = NULL;
  }
}

const char* CDeviceBlinkstick::UsbErrorName(int errcode)
{
#ifdef HAVE_LIBUSB_ERROR_NAME
  return libusb_error_name(errcode);
#else
  return "";
#endif
}


int get_serial_number_bs(libusb_device *dev, const libusb_device_descriptor &descriptor, char *buf, size_t size)
{
  libusb_device_handle *devhandle;

  int error = libusb_open(dev, &devhandle);
  if (error != LIBUSB_SUCCESS)
  {
    return error;
  }

  memset(buf, 0, size);
  error = libusb_get_string_descriptor_ascii(devhandle, descriptor.iSerialNumber, reinterpret_cast<unsigned char *>(buf), size);
  libusb_close(devhandle);
  return error;
}
