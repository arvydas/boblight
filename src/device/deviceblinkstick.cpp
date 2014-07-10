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

#if defined(WIN32)
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#endif

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

#if defined(WIN32)
//Seems to be missing from the declarations in cygwin
//Needed to retrieve BlinkStick's serial number using Windows HID API
extern "C" {
		HIDAPI BOOLEAN NTAPI HidD_GetSerialNumberString (HANDLE, PVOID, ULONG);
}
#endif

bool CDeviceBlinkstick::SetupDevice()
{
#if defined(WIN32)
  //HID implementation for Windows

  m_hidHandle = INVALID_HANDLE_VALUE;

	GUID                                hidGuid;        
	HDEVINFO                            deviceInfoList;
	SP_DEVICE_INTERFACE_DATA            deviceInfo;
	char                                buffer[16324];
	PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceDetails;
	DWORD                               size;
	int                                 i;
	HANDLE                              handle = INVALID_HANDLE_VALUE;
	HIDD_ATTRIBUTES                     deviceAttributes;
	 
	HidD_GetHidGuid(&hidGuid);
	deviceInfoList = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
	deviceInfo.cbSize = sizeof(deviceInfo);
	for(i=0;;i++){
		if(handle != INVALID_HANDLE_VALUE){
			CloseHandle(handle);
			handle = INVALID_HANDLE_VALUE;
		}

		if(!SetupDiEnumDeviceInterfaces(deviceInfoList, 0, &hidGuid, i, &deviceInfo))
		{
			break;  /* no more entries */
		}

		ULONG requiredSize;
		//Get the details with null values to get the required size of the buffer
		SetupDiGetDeviceInterfaceDetail (deviceInfoList,
										 &deviceInfo,
										 NULL, //interfaceDetail,
										 0, //interfaceDetailSize,
										 &requiredSize,
										 0); //infoData))

		deviceDetails = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(requiredSize);
		deviceDetails->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

		//Get the device interface details
		SetupDiGetDeviceInterfaceDetail (deviceInfoList,
												  &deviceInfo,
												  deviceDetails,
												  requiredSize,
												  &requiredSize,
												  NULL);

		/* attempt opening for R/W -- ignore devices which can't be accessed */
		handle = CreateFile(deviceDetails->DevicePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if(handle != INVALID_HANDLE_VALUE){
			deviceAttributes.Size = sizeof(deviceAttributes);
			HidD_GetAttributes(handle, &deviceAttributes);
			if(deviceAttributes.VendorID == BLINKSTICK_VID && deviceAttributes.ProductID == BLINKSTICK_PID)
			{
				wchar_t wserial[USBDEVICE_SERIAL_SIZE];
				char serial[USBDEVICE_SERIAL_SIZE];

				//Serial number is returned as wide char
				if (HidD_GetSerialNumberString(handle, wserial, USBDEVICE_SERIAL_SIZE))
				{
						const wchar_t *wIndirectSerial = wserial;
						mbstate_t       mbstate;

						// Reset to initial shift state
						::memset((void*)&mbstate, 0, sizeof(mbstate));

						//Convert widechar to char
						wcsrtombs(serial, &wIndirectSerial,
																	 USBDEVICE_SERIAL_SIZE, &mbstate);


						Log("%s: found BlinkStick, serial is %s", m_name.c_str(), serial);
				}
				else
				{
						Log("%s: found BlinkStick, but could not retrieve serial.", m_name.c_str());
				}

	      if (strlen(m_serial) == 0 || strlen(m_serial) > 0 && strncmp(m_serial, serial, USBDEVICE_SERIAL_SIZE) == 0)
	      {
		      m_hidHandle = handle;

		      break;  /* we have found the device we are looking for! */
	      }
			}
		}
	}
	SetupDiDestroyDeviceInfoList(deviceInfoList);

	if (m_hidHandle == INVALID_HANDLE_VALUE)
	{
		if (strlen(m_serial) == 0)
		{
			LogError("%s: no BlinkStick device with vid %04x and pid %04x found", m_name.c_str(), BLINKSTICK_VID, BLINKSTICK_PID);
		}
		else
		{
			LogError("%s: no BlinkStick device with vid %04x, pid %04x and serial %s found", m_name.c_str(), BLINKSTICK_VID, BLINKSTICK_PID, m_serial);
		}

		return false;
	}
#else
  //LibUSB implementation for Linux

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

        m_devicehandle = devhandle;

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

#endif

  m_timer.SetInterval(m_interval);

  //set all leds to 0

  return true;
}

inline unsigned int Get8bitColor(CChannel *channel, int64_t now)
{
  return Clamp((unsigned int)Round64((double)channel->GetValue(now) * 255), 0, 255);
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
    unsigned int r = Get8bitColor(&m_channels[i]  , now); 
    unsigned int g = Get8bitColor(&m_channels[i+1], now); 
    unsigned int b = Get8bitColor(&m_channels[i+2], now); 

    if (m_inverse)
    {
      r = 255 - r;
      g = 255 - g;
      b = 255 - b;
    }

    m_buf[idx++] = r & 0xff;
    m_buf[idx++] = g & 0xff;
    m_buf[idx++] = b & 0xff;
  }

  if (idx < BLINKSTICK_REPORT_SIZE)
    memset(m_buf + idx, 0, BLINKSTICK_REPORT_SIZE - idx);

#if defined(WIN32)
  BOOLEAN result = false;

  //Allow 10 attempts to send to BlinkStick before failing
  int attempts = 0;
  while (!result && attempts < 10)
  {
		result = HidD_SetFeature(m_hidHandle, m_buf, 3 + 1);
		if (!result && m_debug)
		  Log("Attempting to resend");
		attempts++;
  }

  m_timer.Wait();

  return result;
#else
  int result = 0;
  int attempts = 0;
  while (result != BLINKSTICK_REPORT_SIZE && attempts < 10)
  {
    result = libusb_control_transfer(m_devicehandle,
                                         0x20,
                                         0x09,
                                         0x01,
                                         0x00,
                                         m_buf, BLINKSTICK_REPORT_SIZE, BLINKSTICK_TIMEOUT);
		if (result != BLINKSTICK_REPORT_SIZE && m_debug)
		  Log("Attempting to resend");
		attempts++;
  }
  m_timer.Wait();

  return result == BLINKSTICK_REPORT_SIZE;
#endif
}

void CDeviceBlinkstick::CloseDevice()
{
#if defined(WIN32)
  if (m_hidHandle != INVALID_HANDLE_VALUE)
  {
	CloseHandle(m_hidHandle);
	m_hidHandle == INVALID_HANDLE_VALUE;
  }
#else
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
#endif
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
