#include <stdio.h>
#include <string>
#include <libusb.h>

#include "psvr2_structs.h"

#if 0
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

	printf("Dev (bus %u, device %u): %04X - %04X speed: %s\n", libusb_get_bus_number(dev), libusb_get_device_address(dev),	desc.idVendor, desc.idProduct, speed);

	if(!handle)
	{
		libusb_open(dev, &handle);
	}
		

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
#endif

bool psvr2_usb_xfer_continue(struct libusb_transfer* xfer, const char* type)
{
	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;

	switch(xfer->status)
	{
	case LIBUSB_TRANSFER_OVERFLOW:
		//PSVR2_ERROR(hmd, "%s xfer returned overflow!", type);
		/* Fall through */
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_NO_DEVICE:
		//os_thread_helper_lock(&hmd->usb_thread);
		hmd->usb_active_xfers--;
		//os_thread_helper_signal_locked(&hmd->usb_thread);
		//os_thread_helper_unlock(&hmd->usb_thread);
		//PSVR2_TRACE(hmd, "%s xfer is aborting with status %d", type, xfer->status);
		return false;

	case LIBUSB_TRANSFER_COMPLETED: break;
	}

	return true;
}

static void LIBUSB_CALL status_xfer_cb(struct libusb_transfer* xfer)
{
	//DRV_TRACE_MARKER();

	if(!psvr2_usb_xfer_continue(xfer, "Status"))
	{
		return;
	}

	//timepoint_ns received_ns = os_monotonic_get_ns();

	// handle status packet
	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;
	hmd->data_lock.lock();

	if((size_t)xfer->actual_length >= sizeof(struct status_record_hdr))
	{
		//PSVR2_TRACE(hmd, "Status - %d bytes", xfer->actual_length);
		//PSVR2_TRACE_HEX(hmd, xfer->buffer, xfer->actual_length);
		//process_status_report(hmd, xfer->buffer, xfer->actual_length, received_ns);
	}

	libusb_submit_transfer(xfer);
	hmd->data_lock.unlock();
}

static void LIBUSB_CALL slam_xfer_cb(struct libusb_transfer* xfer)
{
	//DRV_TRACE_MARKER();

	if(!psvr2_usb_xfer_continue(xfer, "SLAM frame"))
	{
		return;
	}

	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;

	if(xfer->actual_length == sizeof(struct slam_usb_record))
	{
		//process_slam_record(hmd, xfer->buffer, xfer->actual_length);
	}

	hmd->data_lock.lock();
	libusb_submit_transfer(xfer);
	hmd->data_lock.unlock();
}

static void LIBUSB_CALL dump_xfer_cb(struct libusb_transfer* xfer)
{
	//DRV_TRACE_MARKER();
	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;

	const char* name = NULL;

	if(xfer == hmd->led_detector_xfer)
	{
		name = "LED Detector";
	}
	else if(xfer == hmd->relocalizer_xfer)
	{
		name = "RP";
	}
	else if(xfer == hmd->vd_xfer)
	{
		name = "VD";
	}
		
	assert(name != NULL);

	if(!psvr2_usb_xfer_continue(xfer, name))
	{
		return;
	}

	//PSVR2_TRACE(hmd, "%s xfer size %u", name, xfer->actual_length);
	//PSVR2_TRACE_HEX(hmd, xfer->buffer, xfer->actual_length);

	hmd->data_lock.lock();
	libusb_submit_transfer(xfer);
	hmd->data_lock.unlock();
}

bool send_psvr2_control(psvr2_hmd* hmd, uint16_t report_id, uint8_t subcmd, uint8_t* pkt_data, uint32_t pkt_len)
{
	struct sie_ctrl_pkt pkt;
	int ret;

	assert(pkt_len <= sizeof(pkt.data));

	pkt.report_id = report_id;
	pkt.subcmd = subcmd;
	pkt.len = pkt_len;
	memcpy(pkt.data, pkt_data, pkt_len);

	ret = libusb_control_transfer(hmd->dev, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_ENDPOINT, 0x9, report_id, 0x0, (unsigned char*)&pkt, pkt_len + 8, 100);

	if(ret < 0)
	{
		//PSVR2_ERROR(hmd, "Failed to send report id %u subcmd %u", report_id, subcmd);
		return false;
	}

	return true;
}

#if SUPPORT_PSVR2_CAMERAS
struct camera_cmd
{
	uint data[2] = { 0 };
};

bool set_camera_mode(psvr2_hmd* hmd, enum psvr2_camera_mode mode)
{
	camera_cmd cmd;

	cmd.data[0] = 0x1;
	cmd.data[1] = mode;

	//PSVR2_DEBUG(hmd, "Setting camera mode to 0x%x", mode);

	return send_psvr2_control(hmd, PSVR2_REPORT_ID_SET_CAMERA_MODE, 0x1, (uint8_t*)(&cmd), sizeof(cmd));
}
#endif

#if SUPPORT_EYE_TRACKING

uint8_t gaze_buf[USB_GAZE_XFER_SIZE] = { 0 };

static void process_gaze_packet(psvr2_hmd* hmd, uint8_t* buf, size_t bytes_read)
{

	psvr2_gaze_state gaze_state;

	if(bytes_read < sizeof(gaze_state))
	{
		//PSVR2_WARN(hmd, "Gaze packet too small: %zu bytes", bytes_read);
		return;
	}

	memcpy(&gaze_state, buf, sizeof(gaze_state));

	if(memcmp(&gaze_state.header, "GS", 2) != 0)
	{
		//PSVR2_WARN(hmd, "Got gaze with bad header %d", gaze_state.header);
		return;
	}

	if(!hmd->et_data.processed_sample_packet)
	{
		hmd->et_data.processed_sample_packet = true;
	}

	uint32_t remote_sample_timestamp_us = gaze_state.gaze_data_.combined_gaze_.timestamp;

	// wrap-around intentional and A-OK, given these are unsigned
	uint32_t remote_sample_timestamp_delta_us = remote_sample_timestamp_us - hmd->et_data.last_remote_report_sample_time_us;

	hmd->et_data.last_remote_report_sample_time_us = remote_sample_timestamp_us;

	timepoint_ns last_timestamp_ns = hmd->et_data.last_remote_report_sample_time_ns;
	int64_t NS_PER_USEC = 1000;
	timepoint_ns timestamp_ns = hmd->et_data.last_remote_report_sample_time_ns + ((int64_t)remote_sample_timestamp_delta_us * NS_PER_USEC);

	hmd->et_data.last_remote_report_sample_time_ns = timestamp_ns;
	hmd->et_data.data_mutex.lock();

#if 0

	for(int i = 0; i < 2; i++)
	{
		pkt_eye_gaze* eye_gaze_data = i == 0 ? &gaze_state.packet_data.left : &gaze_state.packet_data.right;

		xrt_vec3 gaze_point = eye_gaze_data->gaze_point_mm;
		math_vec3_scalar_mul(1.0 / 1000.0, &gaze_point); // to mm
		gaze_point.x *= -1;
		gaze_point.z *= -1;

		xrt_vec3 gaze_direction = eye_gaze_data->gaze_direction;

		// flip to correct coordinate space
		gaze_direction.x *= -1;
		gaze_direction.z *= -1;

		psvr2_et_eye_data* eye_data = &hmd->et_data.eyes[i];

		if(eye_gaze_data->blink_valid && eye_data->blink != eye_data->blink_interp)
		{
			// amount of time needed to blink, this is technically higher what it should be (100ms on the
			// low end for *whole* blink, closed and open, and this value is for reaching one of those
			// extremes), but we want this to feel "smoothed" out for users, since we only get binary blink
			// data from HMD, and apps won't necessarily smooth it out for us, and this number "feels good"
			const timepoint_ns blink_time_ns = U_TIME_1MS_IN_NS * 100LLU;

			// amount of blink movement occurred since last tick
			double blink_delta = (double)(timestamp_ns - last_timestamp_ns) / (double)blink_time_ns;

			// direction interp is moving
			float dir = eye_gaze_data->blink ? 1 : -1;

			eye_data->blink_interp += dir * blink_delta;
			eye_data->blink_interp = CLAMP(eye_data->blink_interp, 0, 1);
		}

		if(eye_gaze_data->gaze_direction_valid)
		{
			m_filter_euro_vec3_run(&eye_data->gaze_direction_filter, timestamp_ns, &gaze_direction, &eye_data->filtered_gaze_direction);
			math_vec3_normalize(&eye_data->filtered_gaze_direction);
		}

		eye_data->blink = eye_gaze_data->blink;
		eye_data->blink_valid = eye_gaze_data->blink_valid;
		eye_data->gaze_direction = gaze_direction;
		eye_data->gaze_direction_valid = eye_gaze_data->gaze_direction_valid;
		eye_data->gaze_point = gaze_point;
		eye_data->gaze_point_valid = eye_gaze_data->gaze_point_mm_valid;
		eye_data->pupil_diameter = eye_gaze_data->pupil_diameter_mm / 1000.0f; // to m
		eye_data->pupil_diameter_valid = eye_gaze_data->pupil_diameter_valid;
		eye_data->unk_float_2_valid = eye_gaze_data->unk_bool_2;
		eye_data->unk_float_2 = eye_gaze_data->unk_float_2;
		eye_data->unk_float_4_valid = eye_gaze_data->unk_bool_3;
		eye_data->unk_float_4 = eye_gaze_data->unk_float_4;
	}

	{
		pkt_gaze_combined* combined = &gaze_state.packet_data.combined;

		xrt_vec3 gaze_direction = combined->normalized_gaze;
		// flip to correct coordinate space
		gaze_direction.x *= -1;
		gaze_direction.z *= -1;

		xrt_vec3 gaze_point = combined->gaze_point_3d;
		math_vec3_scalar_mul(1.0 / 1000.0, &gaze_point); // to mm
		// flip to correct coordinate space
		gaze_point.x *= -1;
		gaze_point.z *= -1;

		psvr2_et_combined_data* eye_data = &hmd->et_data.combined;

		if(combined->normalized_gaze_valid)
		{
			m_filter_euro_vec3_run(&eye_data->gaze_direction_filter, timestamp_ns, &gaze_direction, &eye_data->filtered_gaze_direction);
			math_vec3_normalize(&eye_data->filtered_gaze_direction);
		}

		eye_data->gaze_direction_valid = combined->normalized_gaze_valid;
		eye_data->gaze_direction = gaze_direction;
		eye_data->gaze_point_valid = combined->gaze_point_valid;
		eye_data->gaze_point = gaze_point;
		eye_data->is_valid = combined->is_valid;
		eye_data->unk_float_8_valid = combined->unk_bool_7;
		eye_data->unk_float_8 = combined->unk_float_8;
		eye_data->unk_float3_pair_valid = combined->unk_bool_9;
		eye_data->unk_float_12 = combined->unk_float_12;
		eye_data->unk_float_15 = combined->unk_float_15;
		eye_data->unk_float_18 = combined->unk_float_18;
	}

	hmd->et_data.unk_float_4_valid = gaze_state.packet_data.unk_bool_9;
	hmd->et_data.unk_float_4 = gaze_state.packet_data.unk_float_4;

	hmd->et_data.unk_float_5_valid = gaze_state.packet_data.unk_bool_10;
	hmd->et_data.unk_float_5 = gaze_state.packet_data.unk_float_5;

	hmd->et_data.data_mutex.unlock();

	// update the gaze direction
	float look_x_dir = atan(hmd->et_data.combined.filtered_gaze_direction.x);
	float look_y_dir = atan(hmd->et_data.combined.filtered_gaze_direction.y);

	xrt_space_relation gaze_relation = { 0 };

	math_quat_from_euler_angles(&(xrt_vec3) 
	{
		.x = look_y_dir, .y = -look_x_dir
	},
		& gaze_relation.pose.orientation);

	gaze_relation.pose.position = (xrt_vec3){ 0 };

	if(hmd->et_data.combined.gaze_direction_valid)
	{
		gaze_relation.relation_flags =
			XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT | XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;
	}

	m_relation_history_push(hmd->et_data.gaze_relation_history, &gaze_relation, timestamp_ns);
#endif
}

static void LIBUSB_CALL gaze_xfer_cb(libusb_transfer* xfer)
{
	//DRV_TRACE_MARKER();

	if(!psvr2_usb_xfer_continue(xfer, "Gaze"))
	{
		return;
	}

	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;

#if 0

	if((size_t)xfer->actual_length >= sizeof(pkt_gaze_state))
	{
		//PSVR2_TRACE(hmd, "Gaze - %d bytes", xfer->actual_length);
		//PSVR2_TRACE_HEX(hmd, xfer->buffer, xfer->actual_length);

		process_gaze_packet(hmd, xfer->buffer, xfer->actual_length);
	}
	else
	{
		//PSVR2_TRACE(hmd, "bad gaze - %d bytes", xfer->actual_length);
	}

	libusb_submit_transfer(xfer);
#endif
}

static void* psvr2_eye_tracking_control_thread(void* usrptr)
{
	bool success;
	psvr2_hmd* hmd = (psvr2_hmd*)usrptr;

	const char* thread_name = "PSVR2 Eye Tracking Control";

	
#if 1
	//U_TRACE_SET_THREAD_NAME(thread_name);
	//os_thread_helper_name(&hmd->et_data.eye_tracking_thread, thread_name);

	//os_thread_helper_lock(&hmd->et_data.eye_tracking_thread);

	bool is_thread_running = true; // os_thread_helper_is_running_locked(&hmd->et_data.eye_tracking_thread)

	while(is_thread_running)
	{
		//os_thread_helper_unlock(&hmd->et_data.eye_tracking_thread);

		bool enable = hmd->et_data.want_enabled || hmd->et_data.force_enable;

		enum psvr2_gaze_stream_subcommand subcmd = enable ? PSVR2_GAZE_STREAM_SUBCMD_ENABLE : PSVR2_GAZE_STREAM_SUBCMD_DISABLE;

		// send keepalive for et, lasts some amount of seconds, we send it regularly to keep it on
		success = send_psvr2_control(hmd, PSVR2_REPORT_ID_SET_GAZE_STREAM, subcmd, NULL, 0);

		if(!success)
		{
			//PSVR2_ERROR(hmd, "Failed to send gaze keepalive");
			return NULL;
		}

		hmd->et_data.enabled = enable;

		//os_nanosleep(U_TIME_1S_IN_NS);

		//os_thread_helper_lock(&hmd->et_data.eye_tracking_thread);
	}

	//os_thread_helper_unlock(&hmd->et_data.eye_tracking_thread);
#endif
	return NULL;
}

#if SUPPORT_EYE_TRACKING
void psvr2_free_et_data(psvr2_hmd* hmd)
{
	//u_var_remove_root(&hmd->et_data);

	// stop the ET thread
	//os_thread_helper_stop_and_wait(&hmd->et_data.eye_tracking_thread);

	//m_relation_history_destroy(&hmd->et_data.gaze_relation_history);
}

int psvr2_start_gaze_tracking(psvr2_hmd* hmd)
{
	int res = 0;

	// Gaze endpoint
	hmd->gaze_xfer = libusb_alloc_transfer(0);

	if(hmd->gaze_xfer == NULL)
	{
		//PSVR2_ERROR(hmd, "Could not alloc USB transfer for gaze data");
		return -1;
	}
	
	libusb_fill_bulk_transfer(hmd->gaze_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_GAZE_ENDPOINT, gaze_buf, USB_GAZE_XFER_SIZE, gaze_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->gaze_xfer);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not submit USB transfer for gaze data");
		libusb_free_transfer(hmd->gaze_xfer);
		hmd->gaze_xfer = NULL;
		return -1;
	}
	hmd->usb_active_xfers++;

#if 0
	m_relation_history_create(&hmd->et_data.gaze_relation_history);

	if(hmd->et_data.gaze_relation_history == NULL)
	{
		//PSVR2_ERROR(hmd, "Could not create relation history");
		return -1;
	}
#endif

#if 0
	res = os_thread_helper_init(&hmd->et_data.eye_tracking_thread);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not create thread for eye tracking keepalive");
		return -1;
	}
#endif

#if SUPPORT_SONY_ET_CALIBRATION
	FILE* eye_calib_file = u_file_open_file_in_config_dir_subpath("psvr2", "eye_calibration.bin", "r");

	if(eye_calib_file)
	{
		size_t file_size;
		char* contents = u_file_read_content(eye_calib_file, &file_size);

		if(contents != NULL)
		{
			res = libusb_bulk_transfer(hmd->dev, LIBUSB_ENDPOINT_OUT | PSVR2_GAZE_INTERFACE, (uint8_t*)contents, file_size, NULL, 1000);

			if(res < 0)
			{
				//PSVR2_ERROR(hmd, "Could not send gaze calibration");
				return -1;
			}

			free(contents);
		}

		fclose(eye_calib_file);
	}
#endif

#if 0
	res = os_thread_helper_start(&hmd->et_data.eye_tracking_thread, psvr2_eye_tracking_control_thread, hmd);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not start gaze keepalive thread");
		return -1;
	}
#endif

	//os_mutex_init(&hmd->et_data.data_mutex);

	//m_filter_euro_vec3_init(&hmd->et_data.combined.gaze_direction_filter, M_EURO_FILTER_EYE_TRACKING_FCMIN, M_EURO_FILTER_EYE_TRACKING_FCMIN_D, M_EURO_FILTER_EYE_TRACKING_BETA);

	//u_var_add_root(&hmd->et_data, "PSVR2 Eye Tracker", true);

	//u_var_add_bool(&hmd->et_data, &hmd->et_data.want_enabled, "Eye Tracking Wanted");
	//u_var_add_bool(&hmd->et_data, &hmd->et_data.force_enable, "Force Enable Eye Tracking");
	//u_var_add_bool(&hmd->et_data, &hmd->et_data.enabled, "Force Enable Enabled");

#if 0
	{
		u_var_add_gui_header(&hmd->et_data, NULL, "Eye Tracker Data");
		psvr2_et_data* et_data = &hmd->et_data;

		u_var_add_ro_i64_ns(et_data, &et_data->last_remote_report_sample_time_ns, "Timestamp");
		u_var_add_ro_u32(et_data, &et_data->last_remote_report_sample_time_us, "Raw Timestamp (us)");

		u_var_add_bool(et_data, &et_data->unk_float_4_valid, "unk_float_4 Valid");
		u_var_add_f32(et_data, &et_data->unk_float_4, "unk_float_4");

		u_var_add_bool(et_data, &et_data->unk_float_5_valid, "unk_float_5 Valid");
		u_var_add_f32(et_data, &et_data->unk_float_5, "unk_float_5");
	}
#endif

#if 0
	for(size_t i = 0; i < 2; i++)
	{
		u_var_add_gui_header(&hmd->et_data, NULL, i == 0 ? "Left Eye" : "Right Eye");
		psvr2_et_eye_data* eye = &hmd->et_data.eyes[i];

		m_filter_euro_vec3_init(&eye->gaze_direction_filter, M_EURO_FILTER_EYE_TRACKING_FCMIN, M_EURO_FILTER_EYE_TRACKING_FCMIN_D, M_EURO_FILTER_EYE_TRACKING_BETA);

		u_var_add_bool(&hmd->et_data, &eye->blink_valid, "Blink Valid");
		u_var_add_bool(&hmd->et_data, &eye->blink, "Blink");

		u_var_add_bool(&hmd->et_data, &eye->pupil_diameter_valid, "Pupil Diameter Valid");
		u_var_add_f32(&hmd->et_data, &eye->pupil_diameter, "Pupil Diameter (meters)");

		u_var_add_bool(&hmd->et_data, &eye->gaze_direction_valid, "Gaze Direction Valid");
		u_var_add_vec3_f32(&hmd->et_data, &eye->gaze_direction, "Gaze Direction");
		u_var_add_vec3_f32(&hmd->et_data, &eye->filtered_gaze_direction, "Filtered Gaze Direction");

		u_var_add_bool(&hmd->et_data, &eye->gaze_point_valid, "Gaze Point Valid");
		u_var_add_vec3_f32(&hmd->et_data, &eye->gaze_point, "Gaze Point");

		u_var_add_bool(&hmd->et_data, &eye->unk_float_2_valid, "unk_float_2 Valid");
		u_var_add_ro_vec2_f32(&hmd->et_data, &eye->unk_float_2, "unk_float_2");

		u_var_add_bool(&hmd->et_data, &eye->unk_float_4_valid, "unk_float_4 Valid");
		u_var_add_ro_vec2_f32(&hmd->et_data, &eye->unk_float_4, "unk_float_4");
	}
#endif

#if 0
	{
		u_var_add_gui_header(&hmd->et_data, NULL, "Combined Eye Data");
		psvr2_et_combined_data* combined = &hmd->et_data.combined;

		u_var_add_bool(&hmd->et_data, &combined->gaze_point_valid, "Gaze Direction Valid");
		u_var_add_vec3_f32(&hmd->et_data, &combined->gaze_direction, "Gaze Direction");
		u_var_add_vec3_f32(&hmd->et_data, &combined->filtered_gaze_direction, "Filtered Gaze Direction");

		u_var_add_bool(&hmd->et_data, &combined->gaze_point_valid, "Gaze Point Valid");
		u_var_add_vec3_f32(&hmd->et_data, &combined->gaze_point, "Gaze Point");

		u_var_add_bool(&hmd->et_data, &combined->is_valid, "Valid");

		u_var_add_bool(&hmd->et_data, &combined->unk_float_8_valid, "unk_float_8 Valid");
		u_var_add_f32(&hmd->et_data, &combined->unk_float_8, "unk_float_8");

		u_var_add_bool(&hmd->et_data, &combined->unk_float3_pair_valid, "unk_float3_pair Valid");
		u_var_add_vec3_f32(&hmd->et_data, &combined->unk_float_12, "unk_float_12");
		u_var_add_vec3_f32(&hmd->et_data, &combined->unk_float_15, "unk_float_15");
		u_var_add_vec3_f32(&hmd->et_data, &combined->unk_float_18, "unk_float_18");
	}
#endif

	return 0;
}
#endif // SUPPORT_EYE_TRACKING

#if SUPPORT_FACE_TRACKING
xrt_result_t psvr2_get_face_tracking(xrt_device* xdev,	enum xrt_input_name facial_expression_type, int64_t at_timestamp_ns, xrt_facial_expression_set* out_value)
{
	xrt_result_t result = XRT_SUCCESS;

	psvr2_hmd* hmd = psvr2_hmd(xdev);

	hmd->et_data.data_mutex.lock();

	// @todo: store a history of facial sample data to be able to interpolate/extrapolate to at_timestamp_ns
	//        for now, let's just always use the latest data

	timepoint_ns latest_local_sample_time_ns = hmd->et_data.last_remote_report_sample_time_ns + hmd->hw2mono_vts;

	bool gaze_directions_valid = true;
	float confidence[2];
	float blink[2];
	struct xrt_vec3 gaze_directions[2];

	for(int i = 0; i < 2; i++)
	{
		struct psvr2_et_eye_data* eye = &hmd->et_data.eyes[i];

		confidence[i] = 1.0f;
		blink[i] = (float)eye->blink_interp;
		gaze_directions[i] = eye->filtered_gaze_direction;

		if(!eye->blink_valid)
		{
			confidence[i] *= 0.25f;
		}

		if(!eye->gaze_direction_valid)
		{
			confidence[i] *= 0.66f;

			// if the per-eye gaze dir isn't valid, pull from the combined data
			gaze_directions[i] = hmd->et_data.combined.filtered_gaze_direction;

			// no per eye or combined gaze data :c
			// we'll still set it based on the combined gaze data, since that *more frequently* has
			// more up to date info (since it will still work with only one eye open)
			if(!hmd->et_data.combined.gaze_direction_valid)
			{
				confidence[i] *= 0.66f;
				gaze_directions_valid = false;
			}
		}
	}

	switch(facial_expression_type)
	{
	case XRT_INPUT_ANDROID_FACE_TRACKING:
	{
		if(hmd->et_data.processed_sample_packet)
		{
			out_value->face_expression_set_android = (struct xrt_facial_expression_set_android){
				.state = XRT_FACE_TRACKING_STATE_STOPPED_ANDROID,
				.is_valid = false,
			};

			break;
		}

		out_value->face_expression_set_android = (struct xrt_facial_expression_set_android){
			.parameters =
				{
					[XRT_FACE_PARAMETER_INDICES_EYES_CLOSED_L_ANDROID] = blink[0],
					[XRT_FACE_PARAMETER_INDICES_EYES_CLOSED_R_ANDROID] = blink[1],
					[XRT_FACE_PARAMETER_INDICES_EYES_LOOK_LEFT_L_ANDROID] = MAX(0, -gaze_directions[0].x),
					[XRT_FACE_PARAMETER_INDICES_EYES_LOOK_LEFT_R_ANDROID] = MAX(0, -gaze_directions[1].x),
					[XRT_FACE_PARAMETER_INDICES_EYES_LOOK_RIGHT_L_ANDROID] = MAX(0, gaze_directions[0].x),
					[XRT_FACE_PARAMETER_INDICES_EYES_LOOK_RIGHT_R_ANDROID] = MAX(0, gaze_directions[1].x),
					[XRT_FACE_PARAMETER_INDICES_EYES_LOOK_DOWN_L_ANDROID] = MAX(0, -gaze_directions[0].y),
					[XRT_FACE_PARAMETER_INDICES_EYES_LOOK_DOWN_R_ANDROID] = MAX(0, -gaze_directions[1].y),
					[XRT_FACE_PARAMETER_INDICES_EYES_LOOK_UP_L_ANDROID] = MAX(0, gaze_directions[0].y),
					[XRT_FACE_PARAMETER_INDICES_EYES_LOOK_UP_R_ANDROID] = MAX(0, gaze_directions[1].y),
				},
			.region_confidences =
				{
					[XRT_FACE_CONFIDENCE_REGIONS_LOWER_ANDROID] = 0.0f,
					[XRT_FACE_CONFIDENCE_REGIONS_LEFT_UPPER_ANDROID] = confidence[0],
					[XRT_FACE_CONFIDENCE_REGIONS_RIGHT_UPPER_ANDROID] = confidence[1],
				},
			.state = hmd->et_data.enabled ? XRT_FACE_TRACKING_STATE_TRACKING_ANDROID
										  : XRT_FACE_TRACKING_STATE_STOPPED_ANDROID,
			.sample_time_ns = latest_local_sample_time_ns,
			.is_valid = hmd->et_data.processed_sample_packet && hmd->et_data.enabled,
		};
		break;
	}
	case XRT_INPUT_FB_FACE_TRACKING2_VISUAL:
	{
		if(hmd->et_data.processed_sample_packet)
		{
			out_value->face_expression_set2_fb = (struct xrt_facial_expression_set2_fb){
				.data_source = XRT_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB,
				.is_valid = false,
			};

			break;
		}

		out_value->face_expression_set2_fb = (struct xrt_facial_expression_set2_fb){
			.weights =
				{
					[XRT_FACE_EXPRESSION2_EYES_CLOSED_L_FB] = blink[0],
					[XRT_FACE_EXPRESSION2_EYES_CLOSED_R_FB] = blink[1],
					[XRT_FACE_EXPRESSION2_EYES_LOOK_LEFT_L_FB] = MAX(0, -gaze_directions[0].x),
					[XRT_FACE_EXPRESSION2_EYES_LOOK_LEFT_R_FB] = MAX(0, -gaze_directions[1].x),
					[XRT_FACE_EXPRESSION2_EYES_LOOK_RIGHT_L_FB] = MAX(0, gaze_directions[0].x),
					[XRT_FACE_EXPRESSION2_EYES_LOOK_RIGHT_R_FB] = MAX(0, gaze_directions[1].x),
					[XRT_FACE_EXPRESSION2_EYES_LOOK_DOWN_L_FB] = MAX(0, -gaze_directions[0].y),
					[XRT_FACE_EXPRESSION2_EYES_LOOK_DOWN_R_FB] = MAX(0, -gaze_directions[1].y),
					[XRT_FACE_EXPRESSION2_EYES_LOOK_UP_L_FB] = MAX(0, gaze_directions[0].y),
					[XRT_FACE_EXPRESSION2_EYES_LOOK_UP_R_FB] = MAX(0, gaze_directions[1].y),
				},
			.confidences =
				{
					[XRT_FACE_CONFIDENCE2_LOWER_FACE_FB] = 0.0f,
					[XRT_FACE_CONFIDENCE2_UPPER_FACE_FB] = (confidence[0] + confidence[1]) / 2.0f,
				},
			.data_source = XRT_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB,
			.sample_time_ns = latest_local_sample_time_ns,
			.is_valid = hmd->et_data.processed_sample_packet && hmd->et_data.enabled,
			.is_eye_following_blendshapes_valid = gaze_directions_valid,
		};
		break;
	}
	case XRT_INPUT_HTC_EYE_FACE_TRACKING:
	{
		if(hmd->et_data.processed_sample_packet)
		{
			out_value->eye_expression_set_htc = (struct xrt_facial_eye_expression_set_htc){
				.base =
					{
						.is_active = false,
					},
			};

			break;
		}

		out_value->eye_expression_set_htc = (struct xrt_facial_eye_expression_set_htc){
			.base =
				{
					.is_active = hmd->et_data.processed_sample_packet && hmd->et_data.enabled,
					.sample_time_ns = latest_local_sample_time_ns,
				},
			.expression_weights =
				{
					[XRT_EYE_EXPRESSION_LEFT_BLINK_HTC] = blink[0],
					[XRT_EYE_EXPRESSION_RIGHT_BLINK_HTC] = blink[1],
				},
		};

		break;
	}
	default: result = XRT_ERROR_INPUT_UNSUPPORTED; break;
	}

	hmd->et_data.data_mutex.unlock();

	return result;
}
#endif

#endif

uint8_t status_buf[USB_STATUS_XFER_SIZE] = { 0 };
uint8_t slam_buf[USB_SLAM_XFER_SIZE] = { 0 };
uint8_t led_detector_buf[USB_LD_XFER_SIZE] = { 0 };
uint8_t relocalizer_buf[USB_RP_XFER_SIZE] = { 0 };
uint8_t vd_buf[USB_VD_XFER_SIZE] = { 0 };

#if SUPPORT_PSVR2_CAMERAS
uint8_t recv_buf[NUM_CAM_XFERS][USB_CAM_MODE10_XFER_SIZE] = { 0 };
#endif

bool psvr2_usb_start(psvr2_hmd* hmd)
{
	bool result = false;
	int res = 0;

	//os_thread_helper_lock(&hmd->usb_thread);

	// Status endpoint
	hmd->status_xfer = libusb_alloc_transfer(0);

	if(hmd->status_xfer == NULL)
	{
		//PSVR2_ERROR(hmd, "Could not alloc USB transfer for status reports");
		goto out;
	}

	libusb_fill_interrupt_transfer(hmd->status_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_STATUS_ENDPOINT, status_buf, USB_STATUS_XFER_SIZE, status_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->status_xfer);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not submit USB transfer for status reports");
		goto out;
	}
	hmd->usb_active_xfers++;

#if SUPPORT_PSVR2_CAMERAS
	hmd->camera_enable = true;
	hmd->camera_mode = PSVR2_CAMERA_MODE_10;

	set_camera_mode(hmd, hmd->camera_mode);
	
	for(int i = 0; i < NUM_CAM_XFERS; i++)
	{
		hmd->camera_xfers[i] = libusb_alloc_transfer(0);

		if(hmd->camera_xfers[i] == NULL)
		{
			//PSVR2_ERROR(hmd, "Could not alloc USB transfer %d for camera data", i);
			goto out;
		}

		libusb_fill_bulk_transfer(hmd->camera_xfers[i], hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_CAMERA_ENDPOINT, recv_buf[i], USB_CAM_MODE10_XFER_SIZE, img_xfer_cb, hmd, 0);

		res = libusb_submit_transfer(hmd->camera_xfers[i]);
		if(res < 0)
		{
			//PSVR2_ERROR(hmd, "Could not submit USB transfer %d for camera data", i);
			goto out;
		}
		hmd->usb_active_xfers++;
	}
#endif

	hmd->slam_xfer = libusb_alloc_transfer(0);

	if(hmd->slam_xfer == NULL)
	{
		//PSVR2_ERROR(hmd, "Could not alloc USB transfer for SLAM data");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->slam_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_SLAM_ENDPOINT, slam_buf, USB_SLAM_XFER_SIZE, slam_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->slam_xfer);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not submit USB transfer for SLAM data");
		goto out;
	}
	hmd->usb_active_xfers++;

	// LD endpoint
	hmd->led_detector_xfer = libusb_alloc_transfer(0);

	if(hmd->led_detector_xfer == NULL)
	{
		//PSVR2_ERROR(hmd, "Could not alloc USB transfer for LED Detector data");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->led_detector_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_LD_ENDPOINT, led_detector_buf, USB_LD_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->led_detector_xfer);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not submit USB transfer for LED Detector data");
		goto out;
	}
	hmd->usb_active_xfers++;

	// RP endpoint
	hmd->relocalizer_xfer = libusb_alloc_transfer(0);

	if(hmd->relocalizer_xfer == NULL)
	{
		//PSVR2_ERROR(hmd, "Could not alloc USB transfer for RP data");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->relocalizer_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_RP_ENDPOINT, relocalizer_buf, USB_RP_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->relocalizer_xfer);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not submit USB transfer for RP data");
		goto out;
	}

	hmd->usb_active_xfers++;

	// VD endpoint
	hmd->vd_xfer = libusb_alloc_transfer(0);

	if(hmd->vd_xfer == NULL)
	{
		//PSVR2_ERROR(hmd, "Could not alloc USB transfer for VD data");
		goto out;
	}
	
	libusb_fill_bulk_transfer(hmd->vd_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_VD_ENDPOINT, vd_buf, USB_VD_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->vd_xfer);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not submit USB transfer for VD data");
		goto out;
	}

	hmd->usb_active_xfers++;

#if SUPPORT_EYE_TRACKING
	res = psvr2_start_gaze_tracking(hmd);

	if(res < 0)
	{
		//PSVR2_ERROR(hmd, "Could not start gaze tracking");
		goto out;
	}
#endif

	result = true;

out:
	//os_thread_helper_unlock(&hmd->usb_thread);
	return result;
}

int main(int argc, char* argv[])
{
	const std::string welcome_str = "PSVR 2 Direct Gaze Reader\n\n";
	printf(welcome_str.c_str());

#if 1
	psvr2_hmd hmd = {};
	bool start_ok = psvr2_usb_start(&hmd);

	if(!start_ok)
	{
		return -1;
	}

#else
	const char* device_name = NULL;
	libusb_device** devs = nullptr;
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
#endif

	return 0;
}
