#include <stdio.h>
#include <string>
#include "libusb.h"

#include "psvr2_structs.h"

int verbose = 0;

static void print_endpoint_comp(const struct libusb_ss_endpoint_companion_descriptor* ep_comp)
{
	printf("      USB 3.0 Endpoint Companion:\n");
	printf("        bMaxBurst:           %u\n", ep_comp->bMaxBurst);
	printf("        bmAttributes:        %02xh\n", ep_comp->bmAttributes);
	printf("        wBytesPerInterval:   %u\n", ep_comp->wBytesPerInterval);
}

static void print_endpoint(const struct libusb_endpoint_descriptor* endpoint)
{
	int i, ret;

	printf("      Endpoint:\n");
	printf("        bEndpointAddress:    %02xh\n", endpoint->bEndpointAddress);
	printf("        bmAttributes:        %02xh\n", endpoint->bmAttributes);
	printf("        wMaxPacketSize:      %u\n", endpoint->wMaxPacketSize);
	printf("        bInterval:           %u\n", endpoint->bInterval);
	printf("        bRefresh:            %u\n", endpoint->bRefresh);
	printf("        bSynchAddress:       %u\n", endpoint->bSynchAddress);

	for(i = 0; i < endpoint->extra_length;)
	{
		if(LIBUSB_DT_SS_ENDPOINT_COMPANION == endpoint->extra[i + 1])
		{
			struct libusb_ss_endpoint_companion_descriptor* ep_comp;

			ret = libusb_get_ss_endpoint_companion_descriptor(NULL, endpoint, &ep_comp);
			if(LIBUSB_SUCCESS != ret)
				continue;

			print_endpoint_comp(ep_comp);

			libusb_free_ss_endpoint_companion_descriptor(ep_comp);
		}

		i += endpoint->extra[i];
	}
}

static void print_altsetting(const struct libusb_interface_descriptor* interface)
{
	uint8_t i;

	printf("    Interface:\n");
	printf("      bInterfaceNumber:      %u\n", interface->bInterfaceNumber);
	printf("      bAlternateSetting:     %u\n", interface->bAlternateSetting);
	printf("      bNumEndpoints:         %u\n", interface->bNumEndpoints);
	printf("      bInterfaceClass:       %u\n", interface->bInterfaceClass);
	printf("      bInterfaceSubClass:    %u\n", interface->bInterfaceSubClass);
	printf("      bInterfaceProtocol:    %u\n", interface->bInterfaceProtocol);
	printf("      iInterface:            %u\n", interface->iInterface);

	for(i = 0; i < interface->bNumEndpoints; i++)
		print_endpoint(&interface->endpoint[i]);
}

static void print_2_0_ext_cap(struct libusb_usb_2_0_extension_descriptor* usb_2_0_ext_cap)
{
	printf("    USB 2.0 Extension Capabilities:\n");
	printf("      bDevCapabilityType:    %u\n", usb_2_0_ext_cap->bDevCapabilityType);
	printf("      bmAttributes:          %08xh\n", usb_2_0_ext_cap->bmAttributes);
}

static void print_ss_usb_cap(struct libusb_ss_usb_device_capability_descriptor* ss_usb_cap)
{
	printf("    USB 3.0 Capabilities:\n");
	printf("      bDevCapabilityType:    %u\n", ss_usb_cap->bDevCapabilityType);
	printf("      bmAttributes:          %02xh\n", ss_usb_cap->bmAttributes);
	printf("      wSpeedSupported:       %u\n", ss_usb_cap->wSpeedSupported);
	printf("      bFunctionalitySupport: %u\n", ss_usb_cap->bFunctionalitySupport);
	printf("      bU1devExitLat:         %u\n", ss_usb_cap->bU1DevExitLat);
	printf("      bU2devExitLat:         %u\n", ss_usb_cap->bU2DevExitLat);
}

static void print_bos(libusb_device_handle* handle)
{
	struct libusb_bos_descriptor* bos;
	uint8_t i;
	int ret;

	ret = libusb_get_bos_descriptor(handle, &bos);
	if(ret < 0)
		return;

	printf("  Binary Object Store (BOS):\n");
	printf("    wTotalLength:            %u\n", bos->wTotalLength);
	printf("    bNumDeviceCaps:          %u\n", bos->bNumDeviceCaps);

	for(i = 0; i < bos->bNumDeviceCaps; i++)
	{
		struct libusb_bos_dev_capability_descriptor* dev_cap = bos->dev_capability[i];

		if(dev_cap->bDevCapabilityType == LIBUSB_BT_USB_2_0_EXTENSION)
		{
			struct libusb_usb_2_0_extension_descriptor* usb_2_0_extension;

			ret = libusb_get_usb_2_0_extension_descriptor(NULL, dev_cap, &usb_2_0_extension);
			if(ret < 0)
				return;

			print_2_0_ext_cap(usb_2_0_extension);
			libusb_free_usb_2_0_extension_descriptor(usb_2_0_extension);
		}
		else if(dev_cap->bDevCapabilityType == LIBUSB_BT_SS_USB_DEVICE_CAPABILITY)
		{
			struct libusb_ss_usb_device_capability_descriptor* ss_dev_cap;

			ret = libusb_get_ss_usb_device_capability_descriptor(NULL, dev_cap, &ss_dev_cap);
			if(ret < 0)
				return;

			print_ss_usb_cap(ss_dev_cap);
			libusb_free_ss_usb_device_capability_descriptor(ss_dev_cap);
		}
	}

	libusb_free_bos_descriptor(bos);
}

static void print_interface(const struct libusb_interface* interface)
{
	int i;

	for(i = 0; i < interface->num_altsetting; i++)
		print_altsetting(&interface->altsetting[i]);
}

static void print_configuration(struct libusb_config_descriptor* config)
{
	uint8_t i;

	printf("  Configuration:\n");
	printf("    wTotalLength:            %u\n", config->wTotalLength);
	printf("    bNumInterfaces:          %u\n", config->bNumInterfaces);
	printf("    bConfigurationValue:     %u\n", config->bConfigurationValue);
	printf("    iConfiguration:          %u\n", config->iConfiguration);
	printf("    bmAttributes:            %02xh\n", config->bmAttributes);
	printf("    MaxPower:                %u\n", config->MaxPower);

	for(i = 0; i < config->bNumInterfaces; i++)
		print_interface(&config->interface[i]);
}

static void print_device(libusb_device* dev, libusb_device_handle* handle)
{
	struct libusb_device_descriptor desc;
	unsigned char string[256];
	const char* speed;
	int ret;
	uint8_t i;

	switch(libusb_get_device_speed(dev))
	{
	case LIBUSB_SPEED_LOW:		speed = "1.5M"; break;
	case LIBUSB_SPEED_FULL:		speed = "12M"; break;
	case LIBUSB_SPEED_HIGH:		speed = "480M"; break;
	case LIBUSB_SPEED_SUPER:	speed = "5G"; break;
	case LIBUSB_SPEED_SUPER_PLUS:	speed = "10G"; break;
	case LIBUSB_SPEED_SUPER_PLUS_X2:	speed = "20G"; break;
	default:			speed = "Unknown";
	}

	ret = libusb_get_device_descriptor(dev, &desc);
	if(ret < 0)
	{
		fprintf(stderr, "failed to get device descriptor");
		return;
	}

	printf("Dev (bus %u, device %u): %04X - %04X speed: %s\n",
		libusb_get_bus_number(dev), libusb_get_device_address(dev),
		desc.idVendor, desc.idProduct, speed);

	if(!handle)
		libusb_open(dev, &handle);

	if(handle)
	{
		if(desc.iManufacturer)
		{
			ret = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, string, sizeof(string));
			if(ret > 0)
				printf("  Manufacturer:              %s\n", (char*)string);
		}

		if(desc.iProduct)
		{
			ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct, string, sizeof(string));
			if(ret > 0)
				printf("  Product:                   %s\n", (char*)string);
		}

		if(desc.iSerialNumber && verbose)
		{
			ret = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, string, sizeof(string));
			if(ret > 0)
				printf("  Serial Number:             %s\n", (char*)string);
		}
	}

	if(verbose)
	{
		for(i = 0; i < desc.bNumConfigurations; i++)
		{
			struct libusb_config_descriptor* config;

			ret = libusb_get_config_descriptor(dev, i, &config);
			if(LIBUSB_SUCCESS != ret)
			{
				printf("  Couldn't retrieve descriptors\n");
				continue;
			}

			print_configuration(config);

			libusb_free_config_descriptor(config);
		}

		if(handle && desc.bcdUSB >= 0x0201)
			print_bos(handle);
	}

	if(handle)
		libusb_close(handle);
}

static int test_wrapped_device(const char* device_name)
{
	(void)device_name;
	printf("Testing wrapped devices is not supported on your platform\n");
	return 1;
}

bool psvr2_usb_start(psvr2_hmd* hmd)
{
	bool result = false;
	int res;

	os_thread_helper_lock(&hmd->usb_thread);

	/* Status endpoint */
	hmd->status_xfer = libusb_alloc_transfer(0);
	if(hmd->status_xfer == NULL)
	{
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for status reports");
		goto out;
	}
	uint8_t* status_buf = malloc(USB_STATUS_XFER_SIZE);
	libusb_fill_interrupt_transfer(hmd->status_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_STATUS_ENDPOINT,
		status_buf, USB_STATUS_XFER_SIZE, status_xfer_cb, hmd, 0);
	hmd->status_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->status_xfer);
	if(res < 0)
	{
		PSVR2_ERROR(hmd, "Could not submit USB transfer for status reports");
		goto out;
	}
	hmd->usb_active_xfers++;

	/* Camera data */
	hmd->camera_enable = true;
	hmd->camera_mode = PSVR2_CAMERA_MODE_10;
	set_camera_mode(hmd, hmd->camera_mode);

	for(int i = 0; i < NUM_CAM_XFERS; i++)
	{
		hmd->camera_xfers[i] = libusb_alloc_transfer(0);
		if(hmd->camera_xfers[i] == NULL)
		{
			PSVR2_ERROR(hmd, "Could not alloc USB transfer %d for camera data", i);
			goto out;
		}

		uint8_t* recv_buf = malloc(USB_CAM_MODE10_XFER_SIZE);

		libusb_fill_bulk_transfer(hmd->camera_xfers[i], hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_CAMERA_ENDPOINT,
			recv_buf, USB_CAM_MODE10_XFER_SIZE, img_xfer_cb, hmd, 0);
		hmd->camera_xfers[i]->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

		res = libusb_submit_transfer(hmd->camera_xfers[i]);
		if(res < 0)
		{
			PSVR2_ERROR(hmd, "Could not submit USB transfer %d for camera data", i);
			goto out;
		}
		hmd->usb_active_xfers++;
	}

	/* SLAM endpoint */
	hmd->slam_xfer = libusb_alloc_transfer(0);
	if(hmd->slam_xfer == NULL)
	{
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for SLAM data");
		goto out;
	}
	uint8_t* slam_buf = malloc(USB_SLAM_XFER_SIZE);
	libusb_fill_bulk_transfer(hmd->slam_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_SLAM_ENDPOINT, slam_buf,
		USB_SLAM_XFER_SIZE, slam_xfer_cb, hmd, 0);
	hmd->slam_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->slam_xfer);
	if(res < 0)
	{
		PSVR2_ERROR(hmd, "Could not submit USB transfer for SLAM data");
		goto out;
	}
	hmd->usb_active_xfers++;

	/* LD endpoint */
	hmd->led_detector_xfer = libusb_alloc_transfer(0);
	if(hmd->led_detector_xfer == NULL)
	{
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for LED Detector data");
		goto out;
	}
	uint8_t* led_detector_buf = malloc(USB_LD_XFER_SIZE);
	libusb_fill_bulk_transfer(hmd->led_detector_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_LD_ENDPOINT,
		led_detector_buf, USB_LD_XFER_SIZE, dump_xfer_cb, hmd, 0);
	hmd->led_detector_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->led_detector_xfer);
	if(res < 0)
	{
		PSVR2_ERROR(hmd, "Could not submit USB transfer for LED Detector data");
		goto out;
	}
	hmd->usb_active_xfers++;

	/* RP endpoint */
	hmd->relocalizer_xfer = libusb_alloc_transfer(0);
	if(hmd->relocalizer_xfer == NULL)
	{
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for RP data");
		goto out;
	}
	uint8_t* relocalizer_buf = malloc(USB_RP_XFER_SIZE);
	libusb_fill_bulk_transfer(hmd->relocalizer_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_RP_ENDPOINT,
		relocalizer_buf, USB_RP_XFER_SIZE, dump_xfer_cb, hmd, 0);
	hmd->relocalizer_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->relocalizer_xfer);
	if(res < 0)
	{
		PSVR2_ERROR(hmd, "Could not submit USB transfer for RP data");
		goto out;
	}
	hmd->usb_active_xfers++;

	/* VD endpoint */
	hmd->vd_xfer = libusb_alloc_transfer(0);
	if(hmd->vd_xfer == NULL)
	{
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for VD data");
		goto out;
	}
	uint8_t* vd_buf = malloc(USB_VD_XFER_SIZE);
	libusb_fill_bulk_transfer(hmd->vd_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_VD_ENDPOINT, vd_buf,
		USB_VD_XFER_SIZE, dump_xfer_cb, hmd, 0);
	hmd->vd_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->vd_xfer);
	if(res < 0)
	{
		PSVR2_ERROR(hmd, "Could not submit USB transfer for VD data");
		goto out;
	}
	hmd->usb_active_xfers++;

	res = psvr2_start_gaze_tracking(hmd);
	if(res < 0)
	{
		PSVR2_ERROR(hmd, "Could not start gaze tracking");
		goto out;
	}

	result = true;

out:
	os_thread_helper_unlock(&hmd->usb_thread);
	return result;
}


int main(int argc, char* argv[])
{
	const std::string welcome_str = "PSVR 2 Direct Gaze Reader\n\n";
	printf(welcome_str.c_str());

	const char* device_name = NULL;
	libusb_device** devs;
	ssize_t cnt;
	int r, i;

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-v"))
		{
			verbose = 1;
		}
		else if(!strcmp(argv[i], "-d") && (i + 1) < argc)
		{
			i++;
			device_name = argv[i];
		}
		else
		{
			printf("Usage %s [-v] [-d </dev/bus/usb/...>]\n", argv[0]);
			printf("Note use -d to test libusb_wrap_sys_device()\n");
			return 1;
		}
	}

	r = libusb_init_context(/*ctx=*/NULL, /*options=*/NULL, /*num_options=*/0);
	if(r < 0)
		return r;

	if(device_name)
	{
		r = test_wrapped_device(device_name);
	}
	else
	{
		cnt = libusb_get_device_list(NULL, &devs);
		if(cnt < 0)
		{
			libusb_exit(NULL);
			return 1;
		}

		for(i = 0; devs[i]; i++)
			print_device(devs[i], NULL);

		libusb_free_device_list(devs, 1);
	}

	libusb_exit(NULL);

	return r;
}
