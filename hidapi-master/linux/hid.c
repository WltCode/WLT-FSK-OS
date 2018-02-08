/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2009

 Copyright 2009, All Rights Reserved.

 At the discretion of the user of this library,
 this software may be licensed under the terms of the
 GNU General Public License v3, a BSD-Style license, or the
 original HIDAPI license as outlined in the LICENSE.txt,
 LICENSE-gpl3.txt, LICENSE-bsd.txt, and LICENSE-orig.txt
 files located at the root of the source distribution.
 These files may also be found in the public source
 code repository located at:
        http://github.com/signal11/hidapi .
********************************************************/

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <poll.h>

/* Linux */
#include <linux/hidraw.h>
#include <linux/version.h>
#include <linux/input.h>
#include <libudev.h>

#include <unistd.h>


#include "hidapi.h"

/* Definitions from linux/hidraw.h. Since these are new, some distros
   may not have header files which contain them. */
#ifndef HIDIOCSFEATURE
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#endif
#ifndef HIDIOCGFEATURE
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

#define TRUE  1
#define FALSE 0



/* USB HID device property names */
const char *device_string_names[] = {
	"manufacturer",
	"product",
	"serial",
};

/* Symbolic names for the properties above */
enum device_string_id {
	DEVICE_STRING_MANUFACTURER,
	DEVICE_STRING_PRODUCT,
	DEVICE_STRING_SERIAL,

	DEVICE_STRING_COUNT,
};

struct hid_device_ {
	int device_handle;
	int blocking;
	int uses_numbered_reports;
};


 hid_device *readcard_handle;
 int Com_Id;
 int Hw_Versions_Number;
 int Souser_Versions_Number;



static __u32 kernel_version = 0;

static __u32 detect_kernel_version(void)
{
	struct utsname name;
	int major, minor, release;
	int ret;

	uname(&name);
	ret = sscanf(name.release, "%d.%d.%d", &major, &minor, &release);
	if (ret == 3) {
		return KERNEL_VERSION(major, minor, release);
	}

	ret = sscanf(name.release, "%d.%d", &major, &minor);
	if (ret == 2) {
		return KERNEL_VERSION(major, minor, 0);
	}

	printf("Couldn't determine kernel version from version string \"%s\"\n", name.release);
	return 0;
}

static hid_device *new_hid_device(void)
{
	hid_device *dev = calloc(1, sizeof(hid_device));
	dev->device_handle = -1;
	dev->blocking = 1;
	dev->uses_numbered_reports = 0;

	return dev;
}


/* The caller must free the returned string with free(). */
static wchar_t *utf8_to_wchar_t(const char *utf8)
{
	wchar_t *ret = NULL;

	if (utf8) {
		size_t wlen = mbstowcs(NULL, utf8, 0);
		if ((size_t) -1 == wlen) {
			return wcsdup(L"");
		}
		ret = calloc(wlen+1, sizeof(wchar_t));
		mbstowcs(ret, utf8, wlen+1);
		ret[wlen] = 0x0000;
	}

	return ret;
}

/* Get an attribute value from a udev_device and return it as a whar_t
   string. The returned string must be freed with free() when done.*/
static wchar_t *copy_udev_string(struct udev_device *dev, const char *udev_name)
{
	return utf8_to_wchar_t(udev_device_get_sysattr_value(dev, udev_name));
}

/* uses_numbered_reports() returns 1 if report_descriptor describes a device
   which contains numbered reports. */
static int uses_numbered_reports(__u8 *report_descriptor, __u32 size) {
	unsigned int i = 0;
	int size_code;
	int data_len, key_size;

	while (i < size) {
		int key = report_descriptor[i];

		/* Check for the Report ID key */
		if (key == 0x85/*Report ID*/) {
			/* This device has a Report ID, which means it uses
			   numbered reports. */
			return 1;
		}

		//printf("key: %02hhx\n", key);

		if ((key & 0xf0) == 0xf0) {
			/* This is a Long Item. The next byte contains the
			   length of the data section (value) for this key.
			   See the HID specification, version 1.11, section
			   6.2.2.3, titled "Long Items." */
			if (i+1 < size)
				data_len = report_descriptor[i+1];
			else
				data_len = 0; /* malformed report */
			key_size = 3;
		}
		else {
			/* This is a Short Item. The bottom two bits of the
			   key contain the size code for the data section
			   (value) for this key.  Refer to the HID
			   specification, version 1.11, section 6.2.2.2,
			   titled "Short Items." */
			size_code = key & 0x3;
			switch (size_code) {
			case 0:
			case 1:
			case 2:
				data_len = size_code;
				break;
			case 3:
				data_len = 4;
				break;
			default:
				/* Can't ever happen since size_code is & 0x3 */
				data_len = 0;
				break;
			};
			key_size = 1;
		}

		/* Skip over this key and it's associated data */
		i += data_len + key_size;
	}

	/* Didn't find a Report ID key. Device doesn't use numbered reports. */
	return 0;
}

/*
 * The caller is responsible for free()ing the (newly-allocated) character
 * strings pointed to by serial_number_utf8 and product_name_utf8 after use.
 */
static int
parse_uevent_info(const char *uevent, int *bus_type,
	unsigned short *vendor_id, unsigned short *product_id,
	char **serial_number_utf8, char **product_name_utf8)
{
	char *tmp = strdup(uevent);
	char *saveptr = NULL;
	char *line;
	char *key;
	char *value;

	int found_id = 0;
	int found_serial = 0;
	int found_name = 0;

	line = strtok_r(tmp, "\n", &saveptr);
	while (line != NULL) {
		/* line: "KEY=value" */
		key = line;
		value = strchr(line, '=');
		if (!value) {
			goto next_line;
		}
		*value = '\0';
		value++;

		if (strcmp(key, "HID_ID") == 0) {
			/**
			 *        type vendor   product
			 * HID_ID=0003:000005AC:00008242
			 **/
			int ret = sscanf(value, "%x:%hx:%hx", bus_type, vendor_id, product_id);
			if (ret == 3) {
				found_id = 1;
			}
		} else if (strcmp(key, "HID_NAME") == 0) {
			/* The caller has to free the product name */
			*product_name_utf8 = strdup(value);
			found_name = 1;
		} else if (strcmp(key, "HID_UNIQ") == 0) {
			/* The caller has to free the serial number */
			*serial_number_utf8 = strdup(value);
			found_serial = 1;
		}

next_line:
		line = strtok_r(NULL, "\n", &saveptr);
	}

	free(tmp);
	return (found_id && found_name && found_serial);
}


static int get_device_string(hid_device *dev, enum device_string_id key, wchar_t *string, size_t maxlen)
{
	struct udev *udev;
	struct udev_device *udev_dev, *parent, *hid_dev;
	struct stat s;
	int ret = -1;
        char *serial_number_utf8 = NULL;
        char *product_name_utf8 = NULL;

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return -1;
	}

	/* Get the dev_t (major/minor numbers) from the file handle. */
	ret = fstat(dev->device_handle, &s);
	if (-1 == ret)
		return ret;
	/* Open a udev device from the dev_t. 'c' means character device. */
	udev_dev = udev_device_new_from_devnum(udev, 'c', s.st_rdev);
	if (udev_dev) {
		hid_dev = udev_device_get_parent_with_subsystem_devtype(
			udev_dev,
			"hid",
			NULL);
		if (hid_dev) {
			unsigned short dev_vid;
			unsigned short dev_pid;
			int bus_type;
			size_t retm;

			ret = parse_uevent_info(
			           udev_device_get_sysattr_value(hid_dev, "uevent"),
			           &bus_type,
			           &dev_vid,
			           &dev_pid,
			           &serial_number_utf8,
			           &product_name_utf8);

			if (bus_type == BUS_BLUETOOTH) {
				switch (key) {
					case DEVICE_STRING_MANUFACTURER:
						wcsncpy(string, L"", maxlen);
						ret = 0;
						break;
					case DEVICE_STRING_PRODUCT:
						retm = mbstowcs(string, product_name_utf8, maxlen);
						ret = (retm == (size_t)-1)? -1: 0;
						break;
					case DEVICE_STRING_SERIAL:
						retm = mbstowcs(string, serial_number_utf8, maxlen);
						ret = (retm == (size_t)-1)? -1: 0;
						break;
					case DEVICE_STRING_COUNT:
					default:
						ret = -1;
						break;
				}
			}
			else {
				/* This is a USB device. Find its parent USB Device node. */
				parent = udev_device_get_parent_with_subsystem_devtype(
					   udev_dev,
					   "usb",
					   "usb_device");
				if (parent) {
					const char *str;
					const char *key_str = NULL;

					if (key >= 0 && key < DEVICE_STRING_COUNT) {
						key_str = device_string_names[key];
					} else {
						ret = -1;
						goto end;
					}

					str = udev_device_get_sysattr_value(parent, key_str);
					if (str) {
						/* Convert the string from UTF-8 to wchar_t */
						retm = mbstowcs(string, str, maxlen);
						ret = (retm == (size_t)-1)? -1: 0;
						goto end;
					}
				}
			}
		}
	}

end:
        free(serial_number_utf8);
        free(product_name_utf8);

	udev_device_unref(udev_dev);
	/* parent and hid_dev don't need to be (and can't be) unref'd.
	   I'm not sure why, but they'll throw double-free() errors. */
	udev_unref(udev);

	return ret;
}

int HID_API_EXPORT hid_init(void)
{
	const char *locale;

	/* Set the locale if it's not set. */
	locale = setlocale(LC_CTYPE, NULL);
	if (!locale)
		setlocale(LC_CTYPE, "");

	kernel_version = detect_kernel_version();

	return 0;
}

int HID_API_EXPORT hid_exit(void)
{
	/* Nothing to do for this in the Linux/hidraw implementation. */
	return 0;
}


struct hid_device_info  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;

	struct hid_device_info *root = NULL; /* return object */
	struct hid_device_info *cur_dev = NULL;
	struct hid_device_info *prev_dev = NULL; /* previous device */

	hid_init();

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return NULL;
	}

	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	/* For each item, see if it matches the vid/pid, and if so
	   create a udev_device record for it */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *sysfs_path;
		const char *dev_path;
		const char *str;
		struct udev_device *raw_dev; /* The device's hidraw udev node. */
		struct udev_device *hid_dev; /* The device's HID udev node. */
		struct udev_device *usb_dev; /* The device's USB udev node. */
		struct udev_device *intf_dev; /* The device's interface (in the USB sense). */
		unsigned short dev_vid;
		unsigned short dev_pid;
		char *serial_number_utf8 = NULL;
		char *product_name_utf8 = NULL;
		int bus_type;
		int result;

		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		sysfs_path = udev_list_entry_get_name(dev_list_entry);
		raw_dev = udev_device_new_from_syspath(udev, sysfs_path);
		dev_path = udev_device_get_devnode(raw_dev);

		hid_dev = udev_device_get_parent_with_subsystem_devtype(
			raw_dev,
			"hid",
			NULL);

		if (!hid_dev) {
			/* Unable to find parent hid device. */
			goto next;
		}

		result = parse_uevent_info(
			udev_device_get_sysattr_value(hid_dev, "uevent"),
			&bus_type,
			&dev_vid,
			&dev_pid,
			&serial_number_utf8,
			&product_name_utf8);

		if (!result) {
			/* parse_uevent_info() failed for at least one field. */
			goto next;
		}

		if (bus_type != BUS_USB && bus_type != BUS_BLUETOOTH) {
			/* We only know how to handle USB and BT devices. */
			goto next;
		}

		/* Check the VID/PID against the arguments */
		if ((vendor_id == 0x0 || vendor_id == dev_vid) &&
		    (product_id == 0x0 || product_id == dev_pid)) {
			struct hid_device_info *tmp;

			/* VID/PID match. Create the record. */
			tmp = malloc(sizeof(struct hid_device_info));
			if (cur_dev) {
				cur_dev->next = tmp;
			}
			else {
				root = tmp;
			}
			prev_dev = cur_dev;
			cur_dev = tmp;

			/* Fill out the record */
			cur_dev->next = NULL;
			cur_dev->path = dev_path? strdup(dev_path): NULL;

			/* VID/PID */
			cur_dev->vendor_id = dev_vid;
			cur_dev->product_id = dev_pid;

			/* Serial Number */
			cur_dev->serial_number = utf8_to_wchar_t(serial_number_utf8);

			/* Release Number */
			cur_dev->release_number = 0x0;

			/* Interface Number */
			cur_dev->interface_number = -1;

			switch (bus_type) {
				case BUS_USB:
					/* The device pointed to by raw_dev contains information about
					   the hidraw device. In order to get information about the
					   USB device, get the parent device with the
					   subsystem/devtype pair of "usb"/"usb_device". This will
					   be several levels up the tree, but the function will find
					   it. */
					usb_dev = udev_device_get_parent_with_subsystem_devtype(
							raw_dev,
							"usb",
							"usb_device");

					if (!usb_dev) {
						/* Free this device */
						free(cur_dev->serial_number);
						free(cur_dev->path);
						free(cur_dev);

						/* Take it off the device list. */
						if (prev_dev) {
							prev_dev->next = NULL;
							cur_dev = prev_dev;
						}
						else {
							cur_dev = root = NULL;
						}

						goto next;
					}

					/* Manufacturer and Product strings */
					cur_dev->manufacturer_string = copy_udev_string(usb_dev, device_string_names[DEVICE_STRING_MANUFACTURER]);
					cur_dev->product_string = copy_udev_string(usb_dev, device_string_names[DEVICE_STRING_PRODUCT]);

					/* Release Number */
					str = udev_device_get_sysattr_value(usb_dev, "bcdDevice");
					cur_dev->release_number = (str)? strtol(str, NULL, 16): 0x0;

					/* Get a handle to the interface's udev node. */
					intf_dev = udev_device_get_parent_with_subsystem_devtype(
							raw_dev,
							"usb",
							"usb_interface");
					if (intf_dev) {
						str = udev_device_get_sysattr_value(intf_dev, "bInterfaceNumber");
						cur_dev->interface_number = (str)? strtol(str, NULL, 16): -1;
					}

					break;

				case BUS_BLUETOOTH:
					/* Manufacturer and Product strings */
					cur_dev->manufacturer_string = wcsdup(L"");
					cur_dev->product_string = utf8_to_wchar_t(product_name_utf8);

					break;

				default:
					/* Unknown device type - this should never happen, as we
					 * check for USB and Bluetooth devices above */
					break;
			}
		}

	next:
		free(serial_number_utf8);
		free(product_name_utf8);
		udev_device_unref(raw_dev);
		/* hid_dev, usb_dev and intf_dev don't need to be (and can't be)
		   unref()d.  It will cause a double-free() error.  I'm not
		   sure why.  */
	}
	/* Free the enumerator and udev objects. */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return root;
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device_info *devs)
{
	struct hid_device_info *d = devs;
	while (d) {
		struct hid_device_info *next = d->next;
		free(d->path);
		free(d->serial_number);
		free(d->manufacturer_string);
		free(d->product_string);
		free(d);
		d = next;
	}
}

hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number)
{
	struct hid_device_info *devs, *cur_dev;
	const char *path_to_open = NULL;
	hid_device *handle = NULL;

	devs = hid_enumerate(vendor_id, product_id);
	cur_dev = devs;
	while (cur_dev) {
		if (cur_dev->vendor_id == vendor_id &&
		    cur_dev->product_id == product_id) {
			if (serial_number) {
				if (wcscmp(serial_number, cur_dev->serial_number) == 0) {
					path_to_open = cur_dev->path;
					break;
				}
			}
			else {
				path_to_open = cur_dev->path;
				break;
			}
		}
		cur_dev = cur_dev->next;
	}

	if (path_to_open) {
		/* Open the device */
		handle = hid_open_path(path_to_open);
	}

	hid_free_enumeration(devs);

	return handle;
}

hid_device * HID_API_EXPORT hid_open_path(const char *path)
{
	hid_device *dev = NULL;

	hid_init();

	dev = new_hid_device();

	/* OPEN HERE */
	dev->device_handle = open(path, O_RDWR);

	/* If we have a good handle, return it. */
	if (dev->device_handle > 0) {

		/* Get the report descriptor */
		int res, desc_size = 0;
		struct hidraw_report_descriptor rpt_desc;

		memset(&rpt_desc, 0x0, sizeof(rpt_desc));

		/* Get Report Descriptor Size */
		res = ioctl(dev->device_handle, HIDIOCGRDESCSIZE, &desc_size);
		if (res < 0)
			perror("HIDIOCGRDESCSIZE");


		/* Get Report Descriptor */
		rpt_desc.size = desc_size;
		res = ioctl(dev->device_handle, HIDIOCGRDESC, &rpt_desc);
		if (res < 0) {
			perror("HIDIOCGRDESC");
		} else {
			/* Determine if this device uses numbered reports. */
			dev->uses_numbered_reports =
				uses_numbered_reports(rpt_desc.value,
				                      rpt_desc.size);
		}

		return dev;
	}
	else {
		/* Unable to open any devices. */
		free(dev);
		return NULL;
	}
}


int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
	int bytes_written;

	bytes_written = write(dev->device_handle, data, length);

	return bytes_written;
}


int HID_API_EXPORT hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds)
{
	int bytes_read;

	if (milliseconds >= 0) {
		/* Milliseconds is either 0 (non-blocking) or > 0 (contains
		   a valid timeout). In both cases we want to call poll()
		   and wait for data to arrive.  Don't rely on non-blocking
		   operation (O_NONBLOCK) since some kernels don't seem to
		   properly report device disconnection through read() when
		   in non-blocking mode.  */
		int ret;
		struct pollfd fds;

		fds.fd = dev->device_handle;
		fds.events = POLLIN;
		fds.revents = 0;
		ret = poll(&fds, 1, milliseconds);
		if (ret == -1 || ret == 0) {
			/* Error or timeout */
			return ret;
		}
		else {
			/* Check for errors on the file descriptor. This will
			   indicate a device disconnection. */
			if (fds.revents & (POLLERR | POLLHUP | POLLNVAL))
				return -1;
		}
	}

	bytes_read = read(dev->device_handle, data, length);
	if (bytes_read < 0 && (errno == EAGAIN || errno == EINPROGRESS))
		bytes_read = 0;

	if (bytes_read >= 0 &&
	    kernel_version != 0 &&
	    kernel_version < KERNEL_VERSION(2,6,34) &&
	    dev->uses_numbered_reports) {
		/* Work around a kernel bug. Chop off the first byte. */
		memmove(data, data+1, bytes_read);
		bytes_read--;
	}

	return bytes_read;
}

int HID_API_EXPORT hid_read(hid_device *dev, unsigned char *data, size_t length)
{
	return hid_read_timeout(dev, data, length, (dev->blocking)? -1: 0);
}

int HID_API_EXPORT hid_set_nonblocking(hid_device *dev, int nonblock)
{
	/* Do all non-blocking in userspace using poll(), since it looks
	   like there's a bug in the kernel in some versions where
	   read() will not return -1 on disconnection of the USB device */

	dev->blocking = !nonblock;
	return 0; /* Success */
}


int HID_API_EXPORT hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
	int res;

	res = ioctl(dev->device_handle, HIDIOCSFEATURE(length), data);
	if (res < 0)
		perror("ioctl (SFEATURE)");

	return res;
}

int HID_API_EXPORT hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
	int res;

	res = ioctl(dev->device_handle, HIDIOCGFEATURE(length), data);
	if (res < 0)
		perror("ioctl (GFEATURE)");


	return res;
}


void HID_API_EXPORT hid_close(hid_device *dev)
{
	if (!dev)
		return;
	close(dev->device_handle);
	free(dev);
}


/////////////////////////////////////////////////////////////////////////////////////////////////


int HID_API_EXPORT HID_API_CALL FindDevCnt(int*GetDevCnt)
{

	struct hid_device_info *devs, *cur_dev;
	int dev_cnt = 0;
	if (hid_init())
		return 0;

	devs = hid_enumerate(0x0, 0x0);
	cur_dev = devs;	
	while (cur_dev) 
	{
		//printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
		//printf("\n");
		//printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
		//printf("  Product:      %ls\n", cur_dev->product_string);
		//printf("  Release:      %hx\n", cur_dev->release_number);
		//printf("  Interface:    %d\n",  cur_dev->interface_number);
		//printf("\n");
		if ((cur_dev->vendor_id == 0x0483) &&
            (cur_dev->product_id == 0x5750))
        {
			dev_cnt++;
		}
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);
	*GetDevCnt = dev_cnt;
	return 1;
}








int HID_API_EXPORT HID_API_CALL OpenComport(unsigned long  port, unsigned long baudrate)
{
	struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	int dev_cnt = 0;
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];


	handle = readcard_handle;

	if (hid_init())
		return 0;

	devs = hid_enumerate(0x0, 0x0);
	cur_dev = devs;	
	while (cur_dev) {
		printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
		printf("\n");
		printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
		printf("  Product:      %ls\n", cur_dev->product_string);
		printf("  Release:      %hx\n", cur_dev->release_number);
		printf("  Interface:    %d\n",  cur_dev->interface_number);
		printf("\n");

		if ((cur_dev->vendor_id == 0x0483) &&
            (cur_dev->product_id == 0x5750))
		{
			dev_cnt++;
			if(dev_cnt == port)
			{
				// Open the device using the VID, PID,
				// and optionally the Serial number.
				////handle = hid_open(0x4d8, 0x3f, L"12345");
				//handle = hid_open(0x0483, 0x5750, NULL);
				handle = hid_open(0x0483, 0x5750, cur_dev->serial_number);
				if (!handle) {
					//printf("unable to open device\n");
					Com_Id = 0;
 					return 0;
				}
				readcard_handle = handle;
				Com_Id = dev_cnt;
				break;
			}
		}



		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);

	
	usleep(50000);


	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);



	if(bRet == TRUE)
	{
		memset(&wBuffer[0],0,32);
		wBuffer[0] = 0x8B;
		wBuffer[1] = 0xB8;
		wBuffer[2] = 0x00;
		wBuffer[3] = 0x00;

		usleep(20000);


		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}
			
		usleep(20000);
handle = readcard_handle;
		memset(pInputReport,0,256);

		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);  




		if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{

			Hw_Versions_Number = pInputReport[4];
			Souser_Versions_Number = pInputReport[5];
		}
		else
		{
			Hw_Versions_Number = 0x00;
			Souser_Versions_Number = 0x00;
		}

	}

	
		
	/*


	if(bRet == TRUE)
	{
		memset(&wBuffer[0],0,32);
		wBuffer[0] = 0x42;
		wBuffer[1] = 0x45;
		wBuffer[2] = 0x01;

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			mdelay(20);
		}

		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}
			
		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			mdelay(20);
		}

		memset(pInputReport,0,256);
		res = hid_read(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
							
		}

		memcpy(&pInputReport[0],&buf[0],32);  
	}
		
	*/

	return 1;
}


int HID_API_EXPORT HID_API_CALL ControlBuzzer(unsigned char pEmptimes)
{
	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];

	handle = readcard_handle;

	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);
	wBuffer[0] = 0x42;
	wBuffer[1] = 0x45;
	wBuffer[2] = pEmptimes;

	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(50000);
	}


	memset(buf,0,sizeof(buf));
	buf[0] = 0;
	memcpy(&buf[1],&wBuffer[0],32);  
	res = hid_write(handle, buf, 33);
	if (res < 0) {
		//printf("Unable to write()\n");
		//printf("Error: %ls\n", hid_error(handle));
	}
	else
	{
		//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
	}

	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(20000);
	}
handle = readcard_handle;
	memset(pInputReport,0,256);


	res =0;
	while(res < 32)
	{
		res = hid_read(handle,  &buf[0], 33);
		if(res == 0)
		{
			//printf("waiting...\n");
		}
		else if (res < 32) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
					
		}
	}



	memcpy(&pInputReport[0],&buf[0],32);  
	
	if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
	{
		bRet = FALSE;	
	}
	else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
	{
		bRet = TRUE;
		return bRet;
	}
	else
	{
		bRet = FALSE;
	}

	return bRet;
}







int HID_API_EXPORT HID_API_CALL ReadSerialNo(unsigned char* sn)
{

	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];
	unsigned char  card_idnum;

	handle = readcard_handle;

			

	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);
	wBuffer[0] = 0x52;
	wBuffer[1] = 0x53;

	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(50000);
	}

	memset(buf,0,sizeof(buf));
	buf[0] = 0;
	memcpy(&buf[1],&wBuffer[0],32);  
	res = hid_write(handle, buf, 33);
	if (res < 0) {
		//printf("Unable to write()\n");
		//printf("Error: %ls\n", hid_error(handle));
	}
	else
	{
		//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
	}
			
	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(20000);
	}
handle = readcard_handle;
	memset(pInputReport,0,256);


	res =0;
	while(res < 32)
	{
		res = hid_read(handle,  &buf[0], 33);
		if(res == 0)
		{
			//printf("waiting...\n");
		}
		else if (res < 32) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
					
		}
	}


	memcpy(&pInputReport[0],&buf[0],32);  



	if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
	{
		//AfxMessageBox("\C7\EB\B7\C5\C8뿨Ƭ\A3\A1");
		bRet = FALSE;
		return 0x55;
				
	}
	else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
	{
		memcpy(&rBuffer[0*16],pInputReport,32);  

		
		card_idnum = rBuffer[7];
		rBuffer[7] = rBuffer[4];
		rBuffer[4] = card_idnum;
		card_idnum = rBuffer[6];
		rBuffer[6] = rBuffer[5];
		rBuffer[5] = card_idnum;


  


		sn[0] = rBuffer[4];
		sn[1] = rBuffer[5];
		sn[2] = rBuffer[6];
		sn[3] = rBuffer[7];
		bRet = TRUE;


			


		return 1;

	}
	else
	{
		bRet = FALSE;
	}



	return 0;
}



int HID_API_EXPORT HID_API_CALL ReadBlockData(unsigned char *sn,    //	Ӳ\BF\A8̖
unsigned char blkNo,           //	Ҫ\B6\C1ȡ\B5Ŀ\E9\BA\C5
unsigned char keyType,         //	\C3ܴa\C0\E0\D0\CD(KeyA,KeyB)
unsigned char *keyValue,       //	\C3ܴa\C4\DA\C8\DD
unsigned char *pdata)
{

	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];
	unsigned char  card_idnum;

	handle = readcard_handle;



	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);
	wBuffer[0] = 0x52;
	wBuffer[1] = 0x53;

	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(50000);
	}


	memset(buf,0,sizeof(buf));
	buf[0] = 0;
	memcpy(&buf[1],&wBuffer[0],32);  
	res = hid_write(handle, buf, 33);
	if (res < 0) {
		//printf("Unable to write()\n");
		//printf("Error: %ls\n", hid_error(handle));
	}
	else
	{
		//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
	}
			
	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(20000);
	}

	memset(pInputReport,0,256);


	res =0;
	while(res < 32)
	{
		res = hid_read(handle,  &buf[0], 33);
		if(res == 0)
		{
			//printf("waiting...\n");
		}
		else if (res < 32) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
					
		}
	}


	memcpy(&pInputReport[0],&buf[0],32);  

	if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
	{
			//AfxMessageBox("\C7\EB\B7\C5\C8뿨Ƭ\A3\A1");
			bRet = FALSE;
			return 0x55;
				
	}
	else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
	{
		memcpy(&rBuffer[0*16],pInputReport,32);  

		
		card_idnum = rBuffer[7];
		rBuffer[7] = rBuffer[4];
		rBuffer[4] = card_idnum;
		card_idnum = rBuffer[6];
		rBuffer[6] = rBuffer[5];
		rBuffer[5] = card_idnum;


			
		if((rBuffer[4]==sn[0])&&(rBuffer[5]==sn[1])&&(rBuffer[6]==sn[2])&&(rBuffer[7]==sn[3]))
		{
			bRet = TRUE;
		}
		else
		{
			bRet = FALSE;
		}
			


	}
	else
	{
		bRet = FALSE;
	}

			
	if(bRet == TRUE)
	{
		
		bRet = FALSE;
		memset(&wBuffer[0],0,32);


			
		if(keyType == 0x60)
		{
			wBuffer[0] = 0x52;
			wBuffer[1] = 0x44;
		}
		else if(keyType == 0x61)
		{
			wBuffer[0] = 0x52;
			wBuffer[1] = 0x46;
		}
			


		wBuffer[2] = blkNo;

		wBuffer[3] = keyValue[0];
		wBuffer[4] = keyValue[1];
		wBuffer[5] = keyValue[2];
		wBuffer[6] = keyValue[3];
		wBuffer[7] = keyValue[4];
		wBuffer[8] = keyValue[5];

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}
				
		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}
				
		memset(pInputReport,0,256);


		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);  


		if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
		{
			//AfxMessageBox("\B6\C1\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;
		}
		else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{

			memcpy(&pdata[0],&pInputReport[4],16);  
			bRet = TRUE;



			return 1;
		}
		else
		{
			//AfxMessageBox("\B6\C1\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;

		}
	}



	

	return 0;
}



int HID_API_EXPORT HID_API_CALL WriteBlockData(unsigned char *sn,             //	Ӳ\BF\A8\BA\C5
unsigned char blkNo,           //	Ҫ\B6\C1ȡ\B5Ŀ\E9\BA\C5
unsigned char keyType,         //	\C3ܴa\C0\E0\D0\CD(KeyA,KeyB)
unsigned char *keyValue,       //	\C3ܴa\C4\DA\C8\DD
unsigned char *pdata)
{



	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];
	unsigned char  card_idnum;
	int i;
	handle = readcard_handle;




	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);
	wBuffer[0] = 0x52;
	wBuffer[1] = 0x53;

	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(50000);
	}

	memset(buf,0,sizeof(buf));
	buf[0] = 0;
	memcpy(&buf[1],&wBuffer[0],32);  
	res = hid_write(handle, buf, 33);
	if (res < 0) {
		//printf("Unable to write()\n");
		//printf("Error: %ls\n", hid_error(handle));
	}
	else
	{
		//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
	}
			
	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(20000);
	}

	memset(pInputReport,0,256);


	res =0;
	while(res < 32)
	{
		res = hid_read(handle,  &buf[0], 33);
		if(res == 0)
		{
			//printf("waiting...\n");
		}
		else if (res < 32) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
					
		}
	}


	memcpy(&pInputReport[0],&buf[0],32);  

	if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
	{
		//AfxMessageBox("\C7\EB\B7\C5\C8뿨Ƭ\A3\A1");
		bRet = FALSE;
		return 0x55;
				
	}
	else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
	{
		memcpy(&rBuffer[0*16],pInputReport,32);  

	
		card_idnum = rBuffer[7];
		rBuffer[7] = rBuffer[4];
		rBuffer[4] = card_idnum;
		card_idnum = rBuffer[6];
		rBuffer[6] = rBuffer[5];
		rBuffer[5] = card_idnum;


  

		if((rBuffer[4]==sn[0])&&(rBuffer[5]==sn[1])&&(rBuffer[6]==sn[2])&&(rBuffer[7]==sn[3]))
		{
			bRet = TRUE;
		}
		else
		{
			bRet = FALSE;
		}
				

	}
	else
	{
		bRet = FALSE;
	}

			
		
	if(bRet == TRUE)
	{

		bRet = FALSE;
		memset(&wBuffer[0],0,32);


		if(keyType == 0x60)
		{
			wBuffer[0] = 0x57;
			wBuffer[1] = 0x52;
		}
		else if(keyType == 0x61)
		{
			wBuffer[0] = 0x57;
			wBuffer[1] = 0x54;
		}

		wBuffer[2] = blkNo;

		wBuffer[3] = keyValue[0];
		wBuffer[4] = keyValue[1];
		wBuffer[5] = keyValue[2];
		wBuffer[6] = keyValue[3];
		wBuffer[7] = keyValue[4];
		wBuffer[8] = keyValue[5];

				
		for(i=9;i<16+9;i++)
		{
			wBuffer[i] = pdata[i-9];
		}

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}

		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}
				
		memset(pInputReport,0,256);


		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);   


		if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
		{
			//AfxMessageBox("д\C8\EB\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;
		}
		else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{
			bRet = TRUE;

			return 1;
		}
		else
		{
			//AfxMessageBox("д\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;

		}


	}

	return 0;
}



int HID_API_EXPORT HID_API_CALL ReadSectorData(unsigned char *sn,    //	Ӳ\BF\A8\BA\C5
unsigned char secNo,           //	Ҫ\B6\C1ȡ\B5\C4\C9\C8\C7\F8\BA\C5
unsigned char keyType,         //	\C3ܴa\C0\E0\D0\CD(KeyA,KeyB)
unsigned char *keyValue,       //	\C3ܴa\C4\DA\C8\DD
unsigned char *pdata)
{


	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];
	unsigned char  card_idnum;

	handle = readcard_handle;


	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);
	wBuffer[0] = 0x52;
	wBuffer[1] = 0x53;

	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(50000);
	}


	memset(buf,0,sizeof(buf));
	buf[0] = 0;
	memcpy(&buf[1],&wBuffer[0],32);  
	res = hid_write(handle, buf, 33);
	if (res < 0) {
		//printf("Unable to write()\n");
		//printf("Error: %ls\n", hid_error(handle));
	}
	else
	{
		//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
	}
			
	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(20000);
	}

	memset(pInputReport,0,256);


	res =0;
	while(res < 32)
	{
		res = hid_read(handle,  &buf[0], 33);
		if(res == 0)
		{
			//printf("waiting...\n");
		}
		else if (res < 32) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
					
		}
	}


	memcpy(&pInputReport[0],&buf[0],32);  

	if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
	{
			//AfxMessageBox("\C7\EB\B7\C5\C8뿨Ƭ\A3\A1");
			bRet = FALSE;
			return 0x55;
				
	}
	else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
	{
		memcpy(&rBuffer[0*16],pInputReport,32);  

	
		card_idnum = rBuffer[7];
		rBuffer[7] = rBuffer[4];
		rBuffer[4] = card_idnum;
		card_idnum = rBuffer[6];
		rBuffer[6] = rBuffer[5];
		rBuffer[5] = card_idnum;

		if((rBuffer[4]==sn[0])&&(rBuffer[5]==sn[1])&&(rBuffer[6]==sn[2])&&(rBuffer[7]==sn[3]))
		{
			bRet = TRUE;
		}
		else
		{
			bRet = FALSE;
		}

	}
	else
	{
		bRet = FALSE;
	}

			
	if(bRet == TRUE)
	{
		
		bRet = FALSE;
		memset(&wBuffer[0],0,32);



		if(keyType == 0x60)
		{
			wBuffer[0] = 0x52;
			wBuffer[1] = 0x44;
		}
		else if(keyType == 0x61)
		{
			wBuffer[0] = 0x52;
			wBuffer[1] = 0x46;
		}



		wBuffer[2] = secNo*4 + 0 ;

		wBuffer[3] = keyValue[0];
		wBuffer[4] = keyValue[1];
		wBuffer[5] = keyValue[2];
		wBuffer[6] = keyValue[3];
		wBuffer[7] = keyValue[4];
		wBuffer[8] = keyValue[5];

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}
				
		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}
				
		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}

		memset(pInputReport,0,256);


		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);  


		if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
		{
			//AfxMessageBox("\B6\C1\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;
		}
		else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{

			memcpy(&pdata[16*0],&pInputReport[4],16);  
			bRet = TRUE;
			//return TRUE;
		}
		else
		{
			//AfxMessageBox("\B6\C1\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;

		}
	}

		
	if(bRet == TRUE)
	{
		
		bRet = FALSE;
		memset(&wBuffer[0],0,32);



		if(keyType == 0x60)
		{
			wBuffer[0] = 0x52;
			wBuffer[1] = 0x44;
		}
		else if(keyType == 0x61)
		{
			wBuffer[0] = 0x52;
			wBuffer[1] = 0x46;
		}



		wBuffer[2] = secNo*4 + 1 ;

		wBuffer[3] = keyValue[0];
		wBuffer[4] = keyValue[1];
		wBuffer[5] = keyValue[2];
		wBuffer[6] = keyValue[3];
		wBuffer[7] = keyValue[4];
		wBuffer[8] = keyValue[5];

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}
				
		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}
				
		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}

		memset(pInputReport,0,256);


		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);   


		if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
		{
			//AfxMessageBox("\B6\C1\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;
		}
		else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{

			memcpy(&pdata[16*1],&pInputReport[4],16);  
			bRet = TRUE;
			//return TRUE;
		}
		else
		{
			//AfxMessageBox("\B6\C1\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;

		}
	}


		
	if(bRet == TRUE)
	{
		
		bRet = FALSE;
		memset(&wBuffer[0],0,32);



		if(keyType == 0x60)
		{
			wBuffer[0] = 0x52;
			wBuffer[1] = 0x44;
		}
		else if(keyType == 0x61)
		{
			wBuffer[0] = 0x52;
			wBuffer[1] = 0x46;
		}



		wBuffer[2] = secNo*4 + 2 ;

		wBuffer[3] = keyValue[0];
		wBuffer[4] = keyValue[1];
		wBuffer[5] = keyValue[2];
		wBuffer[6] = keyValue[3];
		wBuffer[7] = keyValue[4];
		wBuffer[8] = keyValue[5];

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}
				
		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}
				
		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}

		memset(pInputReport,0,256);


		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);  


		if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
		{
			//AfxMessageBox("\B6\C1\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;
		}
		else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{

			memcpy(&pdata[16*2],&pInputReport[4],16);  
			bRet = TRUE;
			//return TRUE;

			return 1;

		}
		else
		{
			//AfxMessageBox("\B6\C1\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;

		}
	}


	return 0;
}



int HID_API_EXPORT HID_API_CALL WriteSectorData(unsigned char *sn,             //	Ӳ\BF\A8\BA\C5
unsigned char secNo,           //	Ҫ\B6\C1ȡ\B5\C4\C9\C8\C7\F8\BA\C5
unsigned char keyType,         //	\C3ܴa\C0\E0\D0\CD(KeyA,KeyB)
unsigned char *keyValue,       //	\C3ܴa\C4\DA\C8\DD
unsigned char *pdata)
{
	
	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];
	unsigned char  card_idnum;
	int i;
	handle = readcard_handle;
	
	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);
	wBuffer[0] = 0x52;
	wBuffer[1] = 0x53;

	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(50000);
	}

	memset(buf,0,sizeof(buf));
	buf[0] = 0;
	memcpy(&buf[1],&wBuffer[0],32);  
	res = hid_write(handle, buf, 33);
	if (res < 0) {
		//printf("Unable to write()\n");
		//printf("Error: %ls\n", hid_error(handle));
	}
	else
	{
		//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
	}
			
	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(20000);
	}

	memset(pInputReport,0,256);



	res =0;
	while(res < 32)
	{
		res = hid_read(handle,  &buf[0], 33);
		if(res == 0)
		{
			//printf("waiting...\n");
		}
		else if (res < 32) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
					
		}
	}


	memcpy(&pInputReport[0],&buf[0],32);  

	if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
	{
		//AfxMessageBox("\C7\EB\B7\C5\C8뿨Ƭ\A3\A1");
		bRet = FALSE;
		return 0x55;
				
	}
	else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
	{
		memcpy(&rBuffer[0*16],pInputReport,32);  

	
		card_idnum = rBuffer[7];
		rBuffer[7] = rBuffer[4];
		rBuffer[4] = card_idnum;
		card_idnum = rBuffer[6];
		rBuffer[6] = rBuffer[5];
		rBuffer[5] = card_idnum;


  

		if((rBuffer[4]==sn[0])&&(rBuffer[5]==sn[1])&&(rBuffer[6]==sn[2])&&(rBuffer[7]==sn[3]))
		{
			bRet = TRUE;
		}
		else
		{
			bRet = FALSE;
		}
				

	}
	else
	{
		bRet = FALSE;
	}

			
		
	if(bRet == TRUE)
	{

		bRet = FALSE;
		memset(&wBuffer[0],0,32);


		if(keyType == 0x60)
		{
			wBuffer[0] = 0x57;
			wBuffer[1] = 0x52;
		}
		else if(keyType == 0x61)
		{
			wBuffer[0] = 0x57;
			wBuffer[1] = 0x54;
		}

		wBuffer[2] = secNo*4 + 0;

		wBuffer[3] = keyValue[0];
		wBuffer[4] = keyValue[1];
		wBuffer[5] = keyValue[2];
		wBuffer[6] = keyValue[3];
		wBuffer[7] = keyValue[4];
		wBuffer[8] = keyValue[5];

				
		for(i=9;i<16+9;i++)
		{
			wBuffer[i] = pdata[i-9+16*0];
		}

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}

		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}
				
		memset(pInputReport,0,256);


		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);  


		if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
		{
			//AfxMessageBox("д\C8\EB\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;
		}
		else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{
			bRet = TRUE;
			//return TRUE;
		}
		else
		{
			//AfxMessageBox("д\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;

		}


	}



		
	if(bRet == TRUE)
	{

		bRet = FALSE;
		memset(&wBuffer[0],0,32);


		if(keyType == 0x60)
		{
			wBuffer[0] = 0x57;
			wBuffer[1] = 0x52;
		}
		else if(keyType == 0x61)
		{
			wBuffer[0] = 0x57;
			wBuffer[1] = 0x54;
		}

		wBuffer[2] = secNo*4 + 1;

		wBuffer[3] = keyValue[0];
		wBuffer[4] = keyValue[1];
		wBuffer[5] = keyValue[2];
		wBuffer[6] = keyValue[3];
		wBuffer[7] = keyValue[4];
		wBuffer[8] = keyValue[5];

				
		for(i=9;i<16+9;i++)
		{
			wBuffer[i] = pdata[i-9+16*1];
		}

		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}

		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}
				
		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}

		memset(pInputReport,0,256);


		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);  


		if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
		{
			//AfxMessageBox("д\C8\EB\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;
		}
		else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{
			bRet = TRUE;
			//return TRUE;
		}
		else
		{
			//AfxMessageBox("д\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;

		}


	}



		
	if(bRet == TRUE)
	{

		bRet = FALSE;
		memset(&wBuffer[0],0,32);


		if(keyType == 0x60)
		{
			wBuffer[0] = 0x57;
			wBuffer[1] = 0x52;
		}
		else if(keyType == 0x61)
		{
			wBuffer[0] = 0x57;
			wBuffer[1] = 0x54;
		}

		wBuffer[2] = secNo*4 + 2;

		wBuffer[3] = keyValue[0];
		wBuffer[4] = keyValue[1];
		wBuffer[5] = keyValue[2];
		wBuffer[6] = keyValue[3];
		wBuffer[7] = keyValue[4];
		wBuffer[8] = keyValue[5];

				
		for(i=9;i<16+9;i++)
		{
			wBuffer[i] = pdata[i-9+16*2];
		}
		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{

		}
		else
		{
			usleep(20000);
		}

		memset(buf,0,sizeof(buf));
		buf[0] = 0;
		memcpy(&buf[1],&wBuffer[0],32);  
		res = hid_write(handle, buf, 33);
		if (res < 0) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
		}
				
		if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
		{
		}
		else
		{
			usleep(20000);
		}

		memset(pInputReport,0,256);


		res =0;
		while(res < 32)
		{
			res = hid_read(handle,  &buf[0], 33);
			if(res == 0)
			{
				//printf("waiting...\n");
			}
			else if (res < 32) {
				//printf("Unable to write()\n");
				//printf("Error: %ls\n", hid_error(handle));
			}
			else
			{
				//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
				//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
				bRet = TRUE;
					
			}
		}


		memcpy(&pInputReport[0],&buf[0],32);  


		if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
		{
			//AfxMessageBox("д\C8\EB\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;
		}
		else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
		{
			bRet = TRUE;
			//return TRUE;

			return 1;
		}
		else
		{
			//AfxMessageBox("д\B9\DC\C0\ED\BF\A8ʧ\B0ܣ\A1");
			bRet = FALSE;

		}


	}


	return 0;
}


int HID_API_EXPORT HID_API_CALL ReadVersion(unsigned char* ver)
{
	*ver = 1;
	return TRUE;
}


void HID_API_EXPORT HID_API_CALL CloseComport()
{
	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;

	Com_Id = 0;
	handle = readcard_handle;
	hid_close(handle);
	/* Free static HIDAPI objects. */
	hid_exit();


}






int HID_API_EXPORT HID_API_CALL ReadFlashData(
unsigned long address,           //	Ҫ\B6\C1ȡ\B5Ŀ\E9\BA\C5
unsigned short datasize,         //	\C3ܴa\C0\E0\D0\CD(KeyA,KeyB)
unsigned char *pdata)
{

	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];


	handle = readcard_handle;
	



	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);
	wBuffer[0] = 0x5A;
	wBuffer[1] = 0xA5;
	wBuffer[2] = (address>>0)&0xff;
	wBuffer[3] = (address>>8)&0xff;
	wBuffer[4] = (address>>16)&0xff;
	wBuffer[5] = (unsigned char)datasize;


	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(50000);
	}

	memset(buf,0,sizeof(buf));
	buf[0] = 0;
	memcpy(&buf[1],&wBuffer[0],32);  
	res = hid_write(handle, buf, 33);
	if (res < 0) {
		//printf("Unable to write()\n");
		//printf("Error: %ls\n", hid_error(handle));
	}
	else
	{
		//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
	}
			
	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{

	}
	else
	{
		usleep(20000);
	}

	memset(pInputReport,0,256);


	res =0;
	while(res < 32)
	{
		res = hid_read(handle,  &buf[0], 33);
		if(res == 0)
		{
			//printf("waiting...\n");
		}
		else if (res < 32) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
					
		}
	}


	memcpy(&pInputReport[0],&buf[0],32);   

	if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
	{
			//AfxMessageBox("\C7\EB\B7\C5\C8뿨Ƭ\A3\A1");
			bRet = FALSE;
			return 0;
				
	}
	else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
	{
		memcpy(&pdata[0],&pInputReport[4],datasize);  
		bRet = TRUE;
		return 1;
	}
	else
	{
		bRet = FALSE;
		return 0;
	}
	
	return 0;

}



int HID_API_EXPORT HID_API_CALL WriteFlashData(
unsigned long address,           //	Ҫ\B6\C1ȡ\B5Ŀ\E9\BA\C5
unsigned short datasize,         //	\C3ܴa\C0\E0\D0\CD(KeyA,KeyB)
unsigned char *pdata)
{

	//struct hid_device_info *devs, *cur_dev;
	hid_device *handle;
	
	unsigned char buf[256];
	int res;
	int bRet = TRUE;
	unsigned char pInputReport[256];
	unsigned char wBuffer[32];
	unsigned char rBuffer[32*5];



	handle = readcard_handle;
	

	memset(&rBuffer[0],0,32*5);
	memset(&wBuffer[0],0,32);
	wBuffer[0] = 0x6A;
	wBuffer[1] = 0xA6;
	wBuffer[2] = (address>>0)&0xff;
	wBuffer[3] = (address>>8)&0xff;
	wBuffer[4] = (address>>16)&0xff;
	wBuffer[5] = (unsigned char)datasize;

	memcpy(&wBuffer[6],&pdata[0],datasize);  

	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(50000);
	}

	memset(buf,0,sizeof(buf));
	buf[0] = 0;
	memcpy(&buf[1],&wBuffer[0],32);  
	res = hid_write(handle, buf, 33);
	if (res < 0) {
		//printf("Unable to write()\n");
		//printf("Error: %ls\n", hid_error(handle));
	}
	else
	{
		//AfxMessageBox("\B7\A2\CBͳɹ\A6\A3\A1");
	}
			
	if((Hw_Versions_Number == 0x01)&&(Souser_Versions_Number == 0x01))
	{
	}
	else
	{
		usleep(20000);
	}

	memset(pInputReport,0,256);


	res =0;
	while(res < 32)
	{
		res = hid_read(handle,  &buf[0], 33);
		if(res == 0)
		{
			//printf("waiting...\n");
		}
		else if (res < 32) {
			//printf("Unable to write()\n");
			//printf("Error: %ls\n", hid_error(handle));
		}
		else
		{
			//AfxMessageBox("\BD\D3\CAճɹ\A6\A3\A1");
			//AfxMessageBox("\B6\C1ȡ\BF\A8\BAųɹ\A6\A3\A1");
			bRet = TRUE;
					
		}
	}


	memcpy(&pInputReport[0],&buf[0],32);  

	if((bRet == TRUE)&&(pInputReport[2] == 0x45)&&(pInputReport[3] == 0x52))
	{
		//AfxMessageBox("\C7\EB\B7\C5\C8뿨Ƭ\A3\A1");
		bRet = FALSE;
		return 0;
				
	}
	else if((bRet == TRUE)&&(pInputReport[2] == 0x4f)&&(pInputReport[3] == 0x4b))
	{
		bRet = TRUE;
		return 1;
	}
	else
	{
		bRet = FALSE;
		return 0;
	}

	return 0;

}




/////////////////////////////////////////////////////////////////////////////////////











int HID_API_EXPORT_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_device_string(dev, DEVICE_STRING_MANUFACTURER, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_device_string(dev, DEVICE_STRING_PRODUCT, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_device_string(dev, DEVICE_STRING_SERIAL, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_indexed_string(hid_device *dev, int string_index, wchar_t *string, size_t maxlen)
{
	return -1;
}


HID_API_EXPORT const wchar_t * HID_API_CALL  hid_error(hid_device *dev)
{
	return NULL;
}
