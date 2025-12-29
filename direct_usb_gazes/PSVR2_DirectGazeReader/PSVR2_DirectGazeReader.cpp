#include <stdio.h>
#include <string>
#include <libusb.h>
#include <algorithm>

#include "psvr2_structs.h"

using namespace std::chrono_literals;

vec3 convert_m_to_mm(const vec3& input_m)
{
	vec3 output_mm = input_m;
	output_mm.x *= METERS_TO_MM;
	output_mm.y *= METERS_TO_MM;
	output_mm.z *= METERS_TO_MM;
	return output_mm;
}

vec3 convert_mm_to_m(const vec3& input_mm)
{
	vec3 output_meters = input_mm;
	output_meters.x *= MM_TO_METERS;
	output_meters.y *= MM_TO_METERS;
	output_meters.z *= MM_TO_METERS;
	return output_meters;
}

vec3 convert_psvr2_direction_to_openxr(const vec3& psvr2_direction)
{
	vec3 openxr_direction = psvr2_direction;
	openxr_direction.x *= -1.0f;
	openxr_direction.z *= -1.0f;
	return openxr_direction;
}

float square(const float input)
{
	return input * input;
}

vec3 safe_normalize(const vec3& input)
{
	vec3 normalized = input;
	float norm_sq = square(input.x) + square(input.y) + square(input.z);
	float norm = sqrt(norm_sq);
	float reciprocal = (norm > 0.0f) ? 1.0f / norm : 1.0f;

	normalized.x *= reciprocal;
	normalized.y *= reciprocal;
	normalized.z *= reciprocal;

	return normalized;
}


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
		printf("%s xfer returned overflow!", type);
		// Fall through
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_NO_DEVICE:
		//os_thread_helper_lock(&hmd->usb_thread);
		hmd->usb_active_xfers--;
		//os_thread_helper_signal_locked(&hmd->usb_thread);
		//os_thread_helper_unlock(&hmd->usb_thread);
		printf("%s xfer is aborting with status %d", type, xfer->status);
		return false;

	case LIBUSB_TRANSFER_COMPLETED: break;
	}

	return true;
}

static void LIBUSB_CALL status_xfer_cb(struct libusb_transfer* xfer)
{
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
		printf("Status - %d bytes", xfer->actual_length);
		//printf(xfer->buffer, xfer->actual_length);
		//process_status_report(hmd, xfer->buffer, xfer->actual_length, received_ns);
	}

	libusb_submit_transfer(xfer);
	hmd->data_lock.unlock();
}

static void LIBUSB_CALL slam_xfer_cb(struct libusb_transfer* xfer)
{
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

	printf("%s xfer size %u", name, xfer->actual_length);
	//printf(xfer->buffer, xfer->actual_length);

	hmd->data_lock.lock();
	libusb_submit_transfer(xfer);
	hmd->data_lock.unlock();
}

bool send_psvr2_control(psvr2_hmd* hmd, uint16_t report_id, uint8_t subcmd, uint8_t* pkt_data, uint32_t pkt_len)
{
	sie_ctrl_pkt pkt = {};

	assert(pkt_len <= sizeof(pkt.data));

	pkt.report_id = report_id;
	pkt.subcmd = subcmd;
	pkt.len = pkt_len;
	memcpy(pkt.data, pkt_data, pkt_len);

	int ret = libusb_control_transfer(hmd->dev, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_ENDPOINT, 0x9, report_id, 0x0, (unsigned char*)&pkt, pkt_len + 8, 100);

	if(ret < 0)
	{
		printf("Failed to send report id %u subcmd %u", report_id, subcmd);
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

	printf("Setting camera mode to 0x%x", mode);

	return send_psvr2_control(hmd, PSVR2_REPORT_ID_SET_CAMERA_MODE, 0x1, (uint8_t*)(&cmd), sizeof(cmd));
}
#endif

#if SUPPORT_EYE_TRACKING

uint8_t gaze_buf[USB_GAZE_XFER_SIZE] = { 0 };

static void process_gaze_packet(psvr2_hmd* hmd, uint8_t* buf, size_t bytes_read)
{
	psvr2_gaze_state input_gaze_state = {};

	if(bytes_read < sizeof(input_gaze_state))
	{
		printf("Gaze packet too small: %zu bytes", bytes_read);
		return;
	}

	memcpy(&input_gaze_state, buf, sizeof(input_gaze_state));

	if(memcmp(&input_gaze_state.header, "GS", 2) != 0)
	{
		printf("Got gaze with bad header %d", input_gaze_state.header);
		return;
	}

	if(!hmd->openxr_eye_tracking_data_.processed_sample_packet)
	{
		hmd->openxr_eye_tracking_data_.processed_sample_packet = true;
	}

	uint32_t remote_sample_timestamp_us = input_gaze_state.gaze_data_.combined_gaze_.timestamp;

	// wrap-around intentional and A-OK, given these are unsigned
	uint32_t remote_sample_timestamp_delta_us = remote_sample_timestamp_us - hmd->openxr_eye_tracking_data_.last_remote_report_sample_time_us;

	hmd->openxr_eye_tracking_data_.last_remote_report_sample_time_us = remote_sample_timestamp_us;

	timepoint_ns last_timestamp_ns = hmd->openxr_eye_tracking_data_.last_remote_report_sample_time_ns;
	int64_t NS_PER_USEC = 1000;
	timepoint_ns timestamp_ns = hmd->openxr_eye_tracking_data_.last_remote_report_sample_time_ns + ((int64_t)remote_sample_timestamp_delta_us * NS_PER_USEC);

	hmd->openxr_eye_tracking_data_.last_remote_report_sample_time_ns = timestamp_ns;
	hmd->openxr_eye_tracking_data_.data_mutex.lock();

	for(int eye = LEFT; eye < NUM_EYES; eye++)
	{
		const psvr2_per_eye_gaze& psvr2_per_eye_gaze_data = (eye == LEFT) ? input_gaze_state.gaze_data_.left_gaze_ : input_gaze_state.gaze_data_.right_gaze_;
		openxr_per_eye_gaze& openxr_per_eye_gaze = hmd->openxr_eye_tracking_data_.openxr_gazes_[eye];

		const vec3 gaze_point_m = convert_m_to_mm(psvr2_per_eye_gaze_data.gaze_point_mm);
		const vec3 gaze_point_openxr_m = convert_psvr2_direction_to_openxr(gaze_point_m);
		const vec3 gaze_direction_openxr = convert_psvr2_direction_to_openxr(psvr2_per_eye_gaze_data.gaze_direction);

#if SUPPORT_LERPED_BLINK_STATES
		if(openxr_per_eye_gaze.is_blink_state_valid && (openxr_per_eye_gaze.blink != openxr_per_eye_gaze.blink_interp))
		{
			const timepoint_ns blink_time_ns = U_TIME_1MS_IN_NS * 100LLU;

			// amount of blink movement occurred since last tick
			double blink_delta = (double)(timestamp_ns - last_timestamp_ns) / (double)blink_time_ns;

			// direction interp is moving
			float dir = gaze.blink_state ? 1 : -1;

			openxr_per_eye_gaze.blink_interp += dir * blink_delta;
			openxr_per_eye_gaze.blink_interp = std::clamp(openxr_per_eye_gaze.blink_interp, 0, 1);
		}
#endif

#if SUPPORT_FILTERED_GAZE_DIRECTIONS
		if(psvr2_per_eye_gaze_data.is_gaze_direction_valid)
		{

			m_filter_euro_vec3_run(&eye_data->gaze_direction_filter, timestamp_ns, &gaze_direction, &eye_data->filtered_gaze_direction);
			math_vec3_normalize(&eye_data->filtered_gaze_direction);
		}
#endif

		openxr_per_eye_gaze.is_blink_state_valid = psvr2_per_eye_gaze_data.is_blink_state_valid;
		openxr_per_eye_gaze.blink_state = psvr2_per_eye_gaze_data.blink_state;
		
		openxr_per_eye_gaze.is_gaze_direction_valid = psvr2_per_eye_gaze_data.is_gaze_direction_valid;
		openxr_per_eye_gaze.gaze_direction = gaze_direction_openxr;

		openxr_per_eye_gaze.is_gaze_point_valid = psvr2_per_eye_gaze_data.is_gaze_point_valid;
		openxr_per_eye_gaze.gaze_point_m = gaze_point_openxr_m;
		
		openxr_per_eye_gaze.is_pupil_diameter_valid = psvr2_per_eye_gaze_data.is_pupil_diameter_valid;
		openxr_per_eye_gaze.pupil_diameter_m = psvr2_per_eye_gaze_data.pupil_diameter_mm * MM_TO_METERS;
	}

	{
		const psvr2_combined_gaze& input_combined_gaze = input_gaze_state.gaze_data_.combined_gaze_;
		openxr_combined_gaze& openxr_combined_gaze = hmd->openxr_eye_tracking_data_.openxr_combined_gaze_;

		const vec3 normalized_gaze_direction_openxr = convert_psvr2_direction_to_openxr(input_combined_gaze.normalized_gaze_direction);
		const vec3 gaze_point_m = convert_mm_to_m(input_combined_gaze.gaze_point_mm);
		const vec3 gaze_point_openxr_m = convert_psvr2_direction_to_openxr(gaze_point_m);

#if SUPPORT_FILTERED_GAZE_DIRECTIONS
		if(combined->normalized_gaze_valid)
		{
			m_filter_euro_vec3_run(&eye_data->gaze_direction_filter, timestamp_ns, &gaze_direction, &eye_data->filtered_gaze_direction);
			math_vec3_normalize(&eye_data->filtered_gaze_direction);
		}
#endif

		uint is_gaze_point_valid = 0;
		vec3 gaze_point;

		uint is_normalized_gaze_direction_valid = 0;
		vec3 normalized_gaze_direction;

		uint is_valid = 0;
		uint timestamp = 0;

		openxr_combined_gaze.is_normalized_gaze_direction_valid = input_combined_gaze.is_normalized_gaze_direction_valid;
		openxr_combined_gaze.normalized_gaze_direction = normalized_gaze_direction_openxr;
		openxr_combined_gaze.is_gaze_point_valid = input_combined_gaze.is_gaze_point_valid;
		openxr_combined_gaze.gaze_point_m = gaze_point_openxr_m;
		openxr_combined_gaze.is_valid = input_combined_gaze.is_valid;
	}

	hmd->openxr_eye_tracking_data_.data_mutex.unlock();

#if 0
	// update the gaze direction
	float look_x_dir = atan(hmd->openxr_eye_tracking_data_.combined_gaze_.filtered_gaze_direction.x);
	float look_y_dir = atan(hmd->openxr_eye_tracking_data_.combined_gaze_.filtered_gaze_direction.y);

	xrt_space_relation gaze_relation = { 0 };

	math_quat_from_euler_angles(&(xrt_vec3) 
	{
		.x = look_y_dir, .y = -look_x_dir
	},
		& gaze_relation.pose.orientation);

	gaze_relation.pose.position = (xrt_vec3){ 0 };

	if(hmd->openxr_eye_tracking_data_.combined.gaze_direction_valid)
	{
		gaze_relation.relation_flags = XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT | XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;
	}

	//m_relation_history_push(hmd->openxr_eye_tracking_data_.gaze_relation_history, &gaze_relation, timestamp_ns);
#endif
}

static void LIBUSB_CALL gaze_xfer_cb(libusb_transfer* xfer)
{
	if(!psvr2_usb_xfer_continue(xfer, "Gaze"))
	{
		return;
	}

	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;

	if((size_t)xfer->actual_length >= sizeof(psvr2_gaze_state))
	{
		printf("Gaze - %d bytes", xfer->actual_length);
		//printf(xfer->buffer, xfer->actual_length);

		process_gaze_packet(hmd, xfer->buffer, xfer->actual_length);
	}
	else
	{
		printf("bad gaze - %d bytes", xfer->actual_length);
	}

	libusb_submit_transfer(xfer);
}

bool keep_running_eye_tracking_control_thread = true;

static void* psvr2_eye_tracking_control_thread(void* usrptr)
{
	bool success;
	psvr2_hmd* hmd = (psvr2_hmd*)usrptr;

	while(keep_running_eye_tracking_control_thread)
	{
		bool enable = hmd->openxr_eye_tracking_data_.want_enabled;

		enum psvr2_gaze_stream_subcommand subcmd = enable ? PSVR2_GAZE_STREAM_SUBCMD_ENABLE : PSVR2_GAZE_STREAM_SUBCMD_DISABLE;

		success = send_psvr2_control(hmd, PSVR2_REPORT_ID_SET_GAZE_STREAM, subcmd, NULL, 0);

		if(!success)
		{
			printf("Failed to send gaze keepalive");
			return NULL;
		}

		hmd->openxr_eye_tracking_data_.enabled = enable;

		std::this_thread::sleep_for(1s);
	}

	return NULL;
}

void stop_gaze_keepalive_thread(psvr2_hmd* hmd)
{
	keep_running_eye_tracking_control_thread = false;
	hmd->usb_thread.join();
}

int psvr2_start_gaze_tracking(psvr2_hmd* hmd)
{
	int res = 0;

	// Gaze endpoint
	hmd->gaze_xfer = libusb_alloc_transfer(0);

	if(hmd->gaze_xfer == NULL)
	{
		printf("Could not alloc USB transfer for gaze data");
		return -1;
	}
	
	libusb_fill_bulk_transfer(hmd->gaze_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_GAZE_ENDPOINT, gaze_buf, USB_GAZE_XFER_SIZE, gaze_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->gaze_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for gaze data");
		libusb_free_transfer(hmd->gaze_xfer);
		hmd->gaze_xfer = NULL;
		return -1;
	}
	hmd->usb_active_xfers++;

#if 0
	m_relation_history_create(&hmd->openxr_eye_tracking_data_.gaze_relation_history);

	if(hmd->openxr_eye_tracking_data_.gaze_relation_history == NULL)
	{
		printf("Could not create relation history");
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
				printf("Could not send gaze calibration");
				return -1;
			}

			free(contents);
		}

		fclose(eye_calib_file);
	}
#endif

	// Start eye-tracking enable/disable/keepalive thread
	hmd->openxr_eye_tracking_data_.eye_tracking_thread = std::thread(psvr2_eye_tracking_control_thread, hmd);

	//m_filter_euro_vec3_init(&hmd->openxr_eye_tracking_data_.combined.gaze_direction_filter, M_EURO_FILTER_EYE_TRACKING_FCMIN, M_EURO_FILTER_EYE_TRACKING_FCMIN_D, M_EURO_FILTER_EYE_TRACKING_BETA);

	//u_var_add_root(&hmd->openxr_eye_tracking_data_, "PSVR2 Eye Tracker", true);

	//u_var_add_bool(&hmd->openxr_eye_tracking_data_, &hmd->openxr_eye_tracking_data_.want_enabled, "Eye Tracking Wanted");
	//u_var_add_bool(&hmd->openxr_eye_tracking_data_, &hmd->openxr_eye_tracking_data_.force_enable, "Force Enable Eye Tracking");
	//u_var_add_bool(&hmd->openxr_eye_tracking_data_, &hmd->openxr_eye_tracking_data_.enabled, "Force Enable Enabled");

#if 0
	{
		u_var_add_gui_header(&hmd->openxr_eye_tracking_data_, NULL, "Eye Tracker Data");
		psvr2_openxr_eye_tracking_data_* openxr_eye_tracking_data_ = &hmd->openxr_eye_tracking_data_;

		u_var_add_ro_i64_ns(openxr_eye_tracking_data_, &openxr_eye_tracking_data_->last_remote_report_sample_time_ns, "Timestamp");
		u_var_add_ro_u32(openxr_eye_tracking_data_, &openxr_eye_tracking_data_->last_remote_report_sample_time_us, "Raw Timestamp (us)");

		u_var_add_bool(openxr_eye_tracking_data_, &openxr_eye_tracking_data_->unk_float_4_valid, "unk_float_4 Valid");
		u_var_add_f32(openxr_eye_tracking_data_, &openxr_eye_tracking_data_->unk_float_4, "unk_float_4");

		u_var_add_bool(openxr_eye_tracking_data_, &openxr_eye_tracking_data_->unk_float_5_valid, "unk_float_5 Valid");
		u_var_add_f32(openxr_eye_tracking_data_, &openxr_eye_tracking_data_->unk_float_5, "unk_float_5");
	}
#endif

#if 0
	for(size_t i = 0; i < 2; i++)
	{
		u_var_add_gui_header(&hmd->openxr_eye_tracking_data_, NULL, i == 0 ? "Left Eye" : "Right Eye");
		psvr2_et_eye_data* eye = &hmd->openxr_eye_tracking_data_.eyes[i];

#if SUPPORT_FILTERED_GAZE_DIRECTIONS
		m_filter_euro_vec3_init(&eye->gaze_direction_filter, M_EURO_FILTER_EYE_TRACKING_FCMIN, M_EURO_FILTER_EYE_TRACKING_FCMIN_D, M_EURO_FILTER_EYE_TRACKING_BETA);
#endif

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &eye->blink_valid, "Blink Valid");
		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &eye->blink, "Blink");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &eye->pupil_diameter_valid, "Pupil Diameter Valid");
		u_var_add_f32(&hmd->openxr_eye_tracking_data_, &eye->pupil_diameter, "Pupil Diameter (meters)");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &eye->gaze_direction_valid, "Gaze Direction Valid");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &eye->gaze_direction, "Gaze Direction");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &eye->filtered_gaze_direction, "Filtered Gaze Direction");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &eye->gaze_point_valid, "Gaze Point Valid");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &eye->gaze_point, "Gaze Point");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &eye->unk_float_2_valid, "unk_float_2 Valid");
		u_var_add_ro_vec2_f32(&hmd->openxr_eye_tracking_data_, &eye->unk_float_2, "unk_float_2");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &eye->unk_float_4_valid, "unk_float_4 Valid");
		u_var_add_ro_vec2_f32(&hmd->openxr_eye_tracking_data_, &eye->unk_float_4, "unk_float_4");
	}
#endif

#if 0
	{
		u_var_add_gui_header(&hmd->openxr_eye_tracking_data_, NULL, "Combined Eye Data");
		psvr2_et_combined_data* combined = &hmd->openxr_eye_tracking_data_.combined;

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &combined->gaze_point_valid, "Gaze Direction Valid");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &combined->gaze_direction, "Gaze Direction");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &combined->filtered_gaze_direction, "Filtered Gaze Direction");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &combined->gaze_point_valid, "Gaze Point Valid");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &combined->gaze_point, "Gaze Point");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &combined->is_valid, "Valid");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &combined->unk_float_8_valid, "unk_float_8 Valid");
		u_var_add_f32(&hmd->openxr_eye_tracking_data_, &combined->unk_float_8, "unk_float_8");

		u_var_add_bool(&hmd->openxr_eye_tracking_data_, &combined->unk_float3_pair_valid, "unk_float3_pair Valid");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &combined->unk_float_12, "unk_float_12");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &combined->unk_float_15, "unk_float_15");
		u_var_add_vec3_f32(&hmd->openxr_eye_tracking_data_, &combined->unk_float_18, "unk_float_18");
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

	hmd->openxr_eye_tracking_data_.data_mutex.lock();

	// @todo: store a history of facial sample data to be able to interpolate/extrapolate to at_timestamp_ns
	//        for now, let's just always use the latest data

	timepoint_ns latest_local_sample_time_ns = hmd->openxr_eye_tracking_data_.last_remote_report_sample_time_ns + hmd->hw2mono_vts;

	bool gaze_directions_valid = true;
	float confidence[2];
	float blink[2];
	struct xrt_vec3 gaze_directions[2];

	for(int eye = LEFT; eye < NUM_EYES; eye++)
	{
		psvr2_et_eye_data& gaze = hmd->openxr_eye_tracking_data_.eyes[eye];

		confidence[eye] = 1.0f;
		blink[eye] = (float)eye->blink_interp;
		gaze_directions[eye] = eye->filtered_gaze_direction;

		if(!eye->blink_valid)
		{
			confidence[eye] *= 0.25f;
		}

		if(!eye->gaze_direction_valid)
		{
			confidence[eye] *= 0.66f;

			// if the per-eye gaze dir isn't valid, pull from the combined data
			gaze_directions[eye] = hmd->openxr_eye_tracking_data_.combined.filtered_gaze_direction;

			// no per eye or combined gaze data :c
			// we'll still set it based on the combined gaze data, since that *more frequently* has
			// more up to date info (since it will still work with only one eye open)
			if(!hmd->openxr_eye_tracking_data_.combined.gaze_direction_valid)
			{
				confidence[eye] *= 0.66f;
				gaze_directions_valid = false;
			}
		}
	}

	switch(facial_expression_type)
	{
	case XRT_INPUT_ANDROID_FACE_TRACKING:
	{
		if(hmd->openxr_eye_tracking_data_.processed_sample_packet)
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
			.state = hmd->openxr_eye_tracking_data_.enabled ? XRT_FACE_TRACKING_STATE_TRACKING_ANDROID
										  : XRT_FACE_TRACKING_STATE_STOPPED_ANDROID,
			.sample_time_ns = latest_local_sample_time_ns,
			.is_valid = hmd->openxr_eye_tracking_data_.processed_sample_packet && hmd->openxr_eye_tracking_data_.enabled,
		};
		break;
	}
	case XRT_INPUT_FB_FACE_TRACKING2_VISUAL:
	{
		if(hmd->openxr_eye_tracking_data_.processed_sample_packet)
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
			.is_valid = hmd->openxr_eye_tracking_data_.processed_sample_packet && hmd->openxr_eye_tracking_data_.enabled,
			.is_eye_following_blendshapes_valid = gaze_directions_valid,
		};
		break;
	}
	case XRT_INPUT_HTC_EYE_FACE_TRACKING:
	{
		if(hmd->openxr_eye_tracking_data_.processed_sample_packet)
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
					.is_active = hmd->openxr_eye_tracking_data_.processed_sample_packet && hmd->openxr_eye_tracking_data_.enabled,
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

	hmd->openxr_eye_tracking_data_.data_mutex.unlock();

	return result;
}
#endif // SUPPORT_FACE_TRACKING

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
		printf("Could not alloc USB transfer for status reports");
		goto out;
	}

	libusb_fill_interrupt_transfer(hmd->status_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_STATUS_ENDPOINT, status_buf, USB_STATUS_XFER_SIZE, status_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->status_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for status reports");
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
			printf("Could not alloc USB transfer %d for camera data", i);
			goto out;
		}

		libusb_fill_bulk_transfer(hmd->camera_xfers[i], hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_CAMERA_ENDPOINT, recv_buf[i], USB_CAM_MODE10_XFER_SIZE, img_xfer_cb, hmd, 0);

		res = libusb_submit_transfer(hmd->camera_xfers[i]);
		if(res < 0)
		{
			printf("Could not submit USB transfer %d for camera data", i);
			goto out;
		}
		hmd->usb_active_xfers++;
	}
#endif

	hmd->slam_xfer = libusb_alloc_transfer(0);

	if(hmd->slam_xfer == NULL)
	{
		printf("Could not alloc USB transfer for SLAM data");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->slam_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_SLAM_ENDPOINT, slam_buf, USB_SLAM_XFER_SIZE, slam_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->slam_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for SLAM data");
		goto out;
	}
	hmd->usb_active_xfers++;

	// LD endpoint
	hmd->led_detector_xfer = libusb_alloc_transfer(0);

	if(hmd->led_detector_xfer == NULL)
	{
		printf("Could not alloc USB transfer for LED Detector data");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->led_detector_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_LD_ENDPOINT, led_detector_buf, USB_LD_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->led_detector_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for LED Detector data");
		goto out;
	}
	hmd->usb_active_xfers++;

	// RP endpoint
	hmd->relocalizer_xfer = libusb_alloc_transfer(0);

	if(hmd->relocalizer_xfer == NULL)
	{
		printf("Could not alloc USB transfer for RP data");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->relocalizer_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_RP_ENDPOINT, relocalizer_buf, USB_RP_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->relocalizer_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for RP data");
		goto out;
	}

	hmd->usb_active_xfers++;

	// VD endpoint
	hmd->vd_xfer = libusb_alloc_transfer(0);

	if(hmd->vd_xfer == NULL)
	{
		printf("Could not alloc USB transfer for VD data");
		goto out;
	}
	
	libusb_fill_bulk_transfer(hmd->vd_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_VD_ENDPOINT, vd_buf, USB_VD_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->vd_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for VD data");
		goto out;
	}

	hmd->usb_active_xfers++;

#if SUPPORT_EYE_TRACKING
	res = psvr2_start_gaze_tracking(hmd);

	if(res < 0)
	{
		printf("Could not start gaze tracking");
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
