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


bool psvr2_usb_xfer_continue(struct libusb_transfer* xfer, const char* type)
{
	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;

	switch(xfer->status)
	{
	case LIBUSB_TRANSFER_OVERFLOW:
		printf("%s xfer returned overflow!\n", type);
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
		printf("%s xfer is aborting with status %d\n", type, xfer->status);
		return false;

	case LIBUSB_TRANSFER_COMPLETED: 
		break;
	}

	return true;
}

#if SUPPORT_PSVR2_STATUS
static void LIBUSB_CALL status_xfer_cb(libusb_transfer* xfer)
{
	if(!psvr2_usb_xfer_continue(xfer, "Status"))
	{
		return;
	}

	//timepoint_ns received_ns = os_monotonic_get_ns();

	// handle status packet
	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;
	hmd->data_lock.lock();

	if((size_t)xfer->actual_length >= sizeof(status_record_hdr))
	{
		//printf("Status - %d bytes\n", xfer->actual_length);
		//printf(xfer->buffer, xfer->actual_length);
		//process_status_report(hmd, xfer->buffer, xfer->actual_length, received_ns);
	}

	libusb_submit_transfer(xfer);
	hmd->data_lock.unlock();
}
#endif // SUPPORT_PSVR2_STATUS

#if SUPPORT_PSVR2_SLAM_TRACKING
static void LIBUSB_CALL slam_xfer_cb(libusb_transfer* xfer)
{
	if(!psvr2_usb_xfer_continue(xfer, "SLAM frame"))
	{
		return;
	}

	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;

	if(xfer->actual_length == sizeof(slam_usb_record))
	{
		//process_slam_record(hmd, xfer->buffer, xfer->actual_length);
	}

	hmd->data_lock.lock();
	libusb_submit_transfer(xfer);
	hmd->data_lock.unlock();
}
#endif // SUPPORT_PSVR2_SLAM_TRACKING

#if (SUPPORT_PSVR2_LED_DETECTOR || SUPPORT_PSVR2_RELOCALIZER || SUPPORT_PSVR2_VD)
static void LIBUSB_CALL dump_xfer_cb(libusb_transfer* xfer)
{
	psvr2_hmd* hmd = (psvr2_hmd*)xfer->user_data;

	const char* name = NULL;

#if SUPPORT_PSVR2_LED_DETECTOR
	if(xfer == hmd->led_detector_xfer)
	{
		name = "LED Detector";
	}
#endif // SUPPORT_PSVR2_LED_DETECTOR
	
#if SUPPORT_PSVR2_RELOCALIZER
	if(xfer == hmd->relocalizer_xfer)
	{
		name = "RP";
	}
#endif // SUPPORT_PSVR2_RELOCALIZER

#if SUPPORT_PSVR2_VD
	if(xfer == hmd->vd_xfer)
	{
		name = "VD";
	}
#endif // SUPPORT_PSVR2_VD
		
	assert(name != NULL);

	if(!psvr2_usb_xfer_continue(xfer, name))
	{
		return;
	}

	//printf("%s xfer size %u\n", name, xfer->actual_length);
	//printf(xfer->buffer, xfer->actual_length);

	hmd->data_lock.lock();
	libusb_submit_transfer(xfer);
	hmd->data_lock.unlock();
}
#endif // (SUPPORT_PSVR2_LED_DETECTOR || SUPPORT_PSVR2_RELOCALIZER || SUPPORT_PSVR2_VD)

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
		printf("Failed to send report id %u subcmd %u\n", report_id, subcmd);
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
#endif // SUPPORT_PSVR2_CAMERAS

#if SUPPORT_EYE_TRACKING

uint8_t gaze_buf[USB_GAZE_XFER_SIZE] = { 0 };

static void process_gaze_packet(psvr2_hmd* hmd, uint8_t* buf, size_t bytes_read)
{
	//printf("START process_gaze_packet: %zu bytes\n", bytes_read);

	psvr2_gaze_state input_gaze_state = {};

	if(bytes_read < sizeof(input_gaze_state))
	{
		printf("Gaze packet too small: %zu bytes\n", bytes_read);
		return;
	}

	memcpy(&input_gaze_state, buf, sizeof(input_gaze_state));

	if(memcmp(&input_gaze_state.header, "GS", 2) != 0)
	{
		printf("Got gaze with bad header %d\n", input_gaze_state.header);
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
		const psvr2_per_eye_gaze& psvr2_per_eye_gaze_data = input_gaze_state.gaze_data_.gazes_[eye];
		openxr_per_eye_gaze& openxr_per_eye_gaze = hmd->openxr_eye_tracking_data_.openxr_gazes_[eye];

		const vec3 gaze_point_m = convert_m_to_mm(psvr2_per_eye_gaze_data.gaze_point_mm);
		const vec3 gaze_point_openxr_m = convert_psvr2_direction_to_openxr(gaze_point_m);
		const vec3 gaze_direction_openxr = convert_psvr2_direction_to_openxr(psvr2_per_eye_gaze_data.gaze_direction);

#if ENABLE_DEBUG_LOG_GAZES
		if((fabs(gaze_direction_openxr.x > 0.01f)) || (fabs(gaze_direction_openxr.y > 0.01f)))
		{
			printf("%s EYE GAZE DIR: X = %.2f, Y = %.2f\n", (eye == LEFT) ? "LEFT" : "RIGHT", gaze_direction_openxr.x, gaze_direction_openxr.y);
		}
#endif

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
		//printf("Gaze - %d bytes\n", xfer->actual_length);
		//printf(xfer->buffer, xfer->actual_length);

		process_gaze_packet(hmd, xfer->buffer, xfer->actual_length);
	}
	else
	{
		printf("bad gaze - %d bytes\n", xfer->actual_length);
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
			printf("Failed to send gaze keepalive\n");
			return NULL;
		}

		hmd->openxr_eye_tracking_data_.enabled = enable;

		std::this_thread::sleep_for(5ms);
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
		printf("Could not alloc USB transfer for gaze data\n");
		return -1;
	}
	
	libusb_fill_bulk_transfer(hmd->gaze_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_GAZE_ENDPOINT, gaze_buf, USB_GAZE_XFER_SIZE, gaze_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->gaze_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for gaze data\n");
		libusb_free_transfer(hmd->gaze_xfer);
		hmd->gaze_xfer = NULL;
		return -1;
	}
	hmd->usb_active_xfers++;

#if 0
	m_relation_history_create(&hmd->openxr_eye_tracking_data_.gaze_relation_history);

	if(hmd->openxr_eye_tracking_data_.gaze_relation_history == NULL)
	{
		printf("Could not create relation histor\ny");
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
				printf("Could not send gaze calibration\n");
				return -1;
			}

			free(contents);
		}

		fclose(eye_calib_file);
	}
#endif // SUPPORT_SONY_ET_CALIBRATION

	// Start eye-tracking enable/disable/keepalive thread
	hmd->openxr_eye_tracking_data_.eye_tracking_thread = std::thread(psvr2_eye_tracking_control_thread, hmd);

#if SUPPORT_FILTERED_GAZE_DIRECTIONS
	m_filter_euro_vec3_init(&hmd->openxr_eye_tracking_data_.combined.gaze_direction_filter, M_EURO_FILTER_EYE_TRACKING_FCMIN, M_EURO_FILTER_EYE_TRACKING_FCMIN_D, M_EURO_FILTER_EYE_TRACKING_BETA);

	for(size_t i = 0; i < 2; i++)
	{
		psvr2_et_eye_data* eye = &hmd->openxr_eye_tracking_data_.eyes[i];

		m_filter_euro_vec3_init(&eye->gaze_direction_filter, M_EURO_FILTER_EYE_TRACKING_FCMIN, M_EURO_FILTER_EYE_TRACKING_FCMIN_D, M_EURO_FILTER_EYE_TRACKING_BETA);

	}
#endif // SUPPORT_FILTERED_GAZE_DIRECTIONS

	return 0;
}
#endif // SUPPORT_EYE_TRACKING

#if SUPPORT_FACE_TRACKING
xrt_result_t psvr2_get_face_tracking(xrt_device* xdev,	enum xrt_input_name facial_expression_type, int64_t at_timestamp_ns, xrt_facial_expression_set* out_value)
{
	xrt_result_t result = XRT_SUCCESS;

	psvr2_hmd* hmd = psvr2_hmd(xdev);

	hmd->openxr_eye_tracking_data_.data_mutex.lock();

	timepoint_ns latest_local_sample_time_ns = hmd->openxr_eye_tracking_data_.last_remote_report_sample_time_ns + hmd->hw2mono_vts;

	bool gaze_directions_valid = true;
	float confidence[NUM_EYES] = { 0.0f, 0.0f };
	float blink[NUM_EYES] = { 0.0f, 0.0f };

	xrt_vec3 gaze_directions[2];

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

#if SUPPORT_PSVR2_STATUS
uint8_t status_buf[USB_STATUS_XFER_SIZE] = { 0 };
#endif // SUPPORT_PSVR2_STATUS

#if SUPPORT_PSVR2_SLAM_TRACKING
uint8_t slam_buf[USB_SLAM_XFER_SIZE] = { 0 };
#endif // SUPPORT_PSVR2_SLAM_TRACKING

#if SUPPORT_PSVR2_LED_DETECTOR
uint8_t led_detector_buf[USB_LD_XFER_SIZE] = { 0 };
#endif // SUPPORT_PSVR2_LED_DETECTOR

#if SUPPORT_PSVR2_RELOCALIZER
uint8_t relocalizer_buf[USB_RP_XFER_SIZE] = { 0 };
#endif // SUPPORT_PSVR2_RELOCALIZER

#if SUPPORT_PSVR2_VD
uint8_t vd_buf[USB_VD_XFER_SIZE] = { 0 };
#endif // SUPPORT_PSVR2_VD

#if SUPPORT_PSVR2_CAMERAS
uint8_t recv_buf[NUM_CAM_XFERS][USB_CAM_MODE10_XFER_SIZE] = { 0 };
#endif // SUPPORT_PSVR2_CAMERAS

bool psvr2_usb_start(psvr2_hmd* hmd)
{
	bool result = false;
	int res = 0;

#if SUPPORT_PSVR2_STATUS
	hmd->status_xfer = libusb_alloc_transfer(0);

	if(hmd->status_xfer == NULL)
	{
		printf("Could not alloc USB transfer for status reports\n");
		goto out;
	}

	libusb_fill_interrupt_transfer(hmd->status_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_STATUS_ENDPOINT, status_buf, USB_STATUS_XFER_SIZE, status_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->status_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for status reports\n");
		goto out;
	}
	hmd->usb_active_xfers++;
#endif // SUPPORT_PSVR2_STATUS

#if SUPPORT_PSVR2_CAMERAS
	hmd->camera_enable = true;
	hmd->camera_mode = PSVR2_CAMERA_MODE_10;

	set_camera_mode(hmd, hmd->camera_mode);
	
	for(int i = 0; i < NUM_CAM_XFERS; i++)
	{
		hmd->camera_xfers[i] = libusb_alloc_transfer(0);

		if(hmd->camera_xfers[i] == NULL)
		{
			printf("Could not alloc USB transfer %d for camera data\n", i);
			goto out;
		}

		libusb_fill_bulk_transfer(hmd->camera_xfers[i], hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_CAMERA_ENDPOINT, recv_buf[i], USB_CAM_MODE10_XFER_SIZE, img_xfer_cb, hmd, 0);

		res = libusb_submit_transfer(hmd->camera_xfers[i]);
		if(res < 0)
		{
			printf("Could not submit USB transfer %d for camera data\n", i);
			goto out;
		}
		hmd->usb_active_xfers++;
	}
#endif // SUPPORT_PSVR2_CAMERAS

#if SUPPORT_PSVR2_SLAM_TRACKING
	hmd->slam_xfer = libusb_alloc_transfer(0);

	if(hmd->slam_xfer == NULL)
	{
		printf("Could not alloc USB transfer for SLAM data\n");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->slam_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_SLAM_ENDPOINT, slam_buf, USB_SLAM_XFER_SIZE, slam_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->slam_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for SLAM data\n");
		goto out;
	}
	hmd->usb_active_xfers++;
#endif // SUPPORT_PSVR2_SLAM_TRACKING

#if SUPPORT_PSVR2_LED_DETECTOR
	hmd->led_detector_xfer = libusb_alloc_transfer(0);

	if(hmd->led_detector_xfer == NULL)
	{
		printf("Could not alloc USB transfer for LED Detector data\n");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->led_detector_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_LD_ENDPOINT, led_detector_buf, USB_LD_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->led_detector_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for LED Detector data\n");
		goto out;
	}
	hmd->usb_active_xfers++;
#endif // SUPPORT_PSVR2_LED_DETECTOR

#if SUPPORT_PSVR2_RELOCALIZER
	hmd->relocalizer_xfer = libusb_alloc_transfer(0);

	if(hmd->relocalizer_xfer == NULL)
	{
		printf("Could not alloc USB transfer for RP data\n");
		goto out;
	}

	libusb_fill_bulk_transfer(hmd->relocalizer_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_RP_ENDPOINT, relocalizer_buf, USB_RP_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->relocalizer_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for RP data\n");
		goto out;
	}

	hmd->usb_active_xfers++;
#endif // SUPPORT_PSVR2_RELOCALIZER

#if SUPPORT_PSVR2_VD
	hmd->vd_xfer = libusb_alloc_transfer(0);

	if(hmd->vd_xfer == NULL)
	{
		printf("Could not alloc USB transfer for VD data\n");
		goto out;
	}
	
	libusb_fill_bulk_transfer(hmd->vd_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_VD_ENDPOINT, vd_buf, USB_VD_XFER_SIZE, dump_xfer_cb, hmd, 0);

	res = libusb_submit_transfer(hmd->vd_xfer);

	if(res < 0)
	{
		printf("Could not submit USB transfer for VD data\n");
		goto out;
	}

	hmd->usb_active_xfers++;
#endif // SUPPORT_PSVR2_VD

#if SUPPORT_EYE_TRACKING
	res = psvr2_start_gaze_tracking(hmd);

	if(res < 0)
	{
		printf("Could not start gaze tracking\n");
		goto out;
	}
#endif // SUPPORT_EYE_TRACKING

	result = true;

out:
	//os_thread_helper_unlock(&hmd->usb_thread);
	return result;
}

struct psvr2_interface_info
{
	int interface_no = 0;
	int altmode = 0;
	const char* name = nullptr;
};

psvr2_interface_info interface_list[] = {
	{.interface_no = PSVR2_STATUS_INTERFACE, .altmode = 1, .name = "status"},
	{.interface_no = PSVR2_SLAM_INTERFACE, .altmode = 0, .name = "SLAM"},
	{.interface_no = PSVR2_GAZE_INTERFACE, .altmode = 0, .name = "Gaze"},
	{.interface_no = PSVR2_CAMERA_INTERFACE, .altmode = 0, .name = "Camera"},
	{.interface_no = PSVR2_LD_INTERFACE, .altmode = 0, .name = "LED Detector"},
	{.interface_no = PSVR2_RP_INTERFACE, .altmode = 0, .name = "Relocalizer"},
	{.interface_no = PSVR2_VD_INTERFACE, .altmode = 0, .name = "VD"},
};

static bool psvr2_usb_open(psvr2_hmd* hmd)
{
	printf("Trying to open PSVR2 USB connection...\n");

	int res = 0;

	res = libusb_init(&hmd->ctx);

	if(res < 0)
	{
		printf("Failed to init USB\n");
		return false;
	}

	hmd->dev = libusb_open_device_with_vid_pid(hmd->ctx, PSVR2_VID, PSVR2_PID);

	if(hmd->dev == NULL)
	{
		printf("Failed to open USB device\n");
		return false;
	}

	size_t num_interfaces = sizeof(interface_list) / sizeof(interface_list[0]);

	for(size_t interface_index = 0; interface_index < num_interfaces; interface_index++)
	{
		psvr2_interface_info& interface = interface_list[interface_index];
		int intf_no = interface.interface_no;
		int altmode = interface.altmode;
		const char* name = interface.name;

		res = libusb_claim_interface(hmd->dev, intf_no);

		if(res < 0)
		{
			printf("Failed to claim USB %s interface\n", name);
			return false;
		}

		res = libusb_set_interface_alt_setting(hmd->dev, intf_no, altmode);

		if(res < 0)
		{
			printf("Failed to set USB %s interface alt %d\n", name, altmode);
			return false;
		}
	}

	printf("PSVR2 USB connection SUCCESS\n");
	return true;
}

static void psvr2_usb_stop(psvr2_hmd* hmd)
{
	hmd->data_lock.lock();

	int ret = 0;

#if SUPPORT_PSVR2_CAMERAS
	for (int i = 0; i < NUM_CAM_XFERS; i++)
	{                               
		if(hmd->camera_xfers[i])
		{
			ret = libusb_cancel_transfer(hmd->camera_xfers[i]);
			assert(ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND);
		}
	} 
#endif // SUPPORT_PSVR2_CAMERAS

#if SUPPORT_PSVR2_STATUS
	if(hmd->status_xfer)
	{
		ret = libusb_cancel_transfer(hmd->status_xfer);
		assert(ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND);
	}
#endif // SUPPORT_PSVR2_STATUS

#if SUPPORT_PSVR2_SLAM_TRACKING
	if(hmd->slam_xfer)
	{
		ret = libusb_cancel_transfer(hmd->slam_xfer);
		assert(ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND);
	}
#endif // SUPPORT_PSVR2_SLAM_TRACKING

#if SUPPORT_PSVR2_LED_DETECTOR
	if(hmd->led_detector_xfer)
	{
		ret = libusb_cancel_transfer(hmd->led_detector_xfer);
		assert(ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND);
	}
#endif // SUPPORT_PSVR2_LED_DETECTOR

#if SUPPORT_PSVR2_RELOCALIZER
	if(hmd->relocalizer_xfer)
	{
		ret = libusb_cancel_transfer(hmd->relocalizer_xfer);
		assert(ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND);
	}
#endif // SUPPORT_PSVR2_RELOCALIZER

#if SUPPORT_PSVR2_VD
	if(hmd->vd_xfer)
	{
		ret = libusb_cancel_transfer(hmd->vd_xfer);
		assert(ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND);
	}
#endif // SUPPORT_PSVR2_VD

#if SUPPORT_EYE_TRACKING
	if(hmd->gaze_xfer)
	{
		ret = libusb_cancel_transfer(hmd->gaze_xfer);
		assert(ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND);
	}
#endif // SUPPORT_EYE_TRACKING

	hmd->data_lock.unlock();
}

void psvr2_usb_destroy(psvr2_hmd* hmd)
{
#if SUPPORT_PSVR2_CAMERAS
	for(int i = 0; i < NUM_CAM_XFERS; i++)
	{
		if(hmd->camera_xfers[i])
		{
			libusb_free_transfer(hmd->camera_xfers[i]);
			hmd->camera_xfers[i] = nullptr;
		}
	}
#endif // SUPPORT_PSVR2_CAMERAS

#if SUPPORT_PSVR2_STATUS
	if(hmd->status_xfer)
	{
		libusb_free_transfer(hmd->status_xfer);
		hmd->status_xfer = nullptr;
	}
#endif // SUPPORT_PSVR2_STATUS

#if SUPPORT_PSVR2_SLAM_TRACKING
	if(hmd->slam_xfer)
	{
		libusb_free_transfer(hmd->slam_xfer);
		hmd->slam_xfer = nullptr;
	}
#endif // SUPPORT_PSVR2_SLAM_TRACKING

#if SUPPORT_PSVR2_LED_DETECTOR
	if(hmd->led_detector_xfer)
	{
		libusb_free_transfer(hmd->led_detector_xfer);
		hmd->led_detector_xfer = nullptr;
	}
#endif // SUPPORT_PSVR2_LED_DETECTOR

#if SUPPORT_PSVR2_RELOCALIZER
	if(hmd->relocalizer_xfer)
	{
		libusb_free_transfer(hmd->relocalizer_xfer);
		hmd->relocalizer_xfer = nullptr;
	}
#endif // SUPPORT_PSVR2_RELOCALIZER

#if SUPPORT_PSVR2_VD
	if(hmd->vd_xfer)
	{
		libusb_free_transfer(hmd->vd_xfer);
		hmd->vd_xfer = nullptr;
	}
#endif // SUPPORT_PSVR2_VD

#if SUPPORT_EYE_TRACKING
	if(hmd->gaze_xfer)
	{
		libusb_free_transfer(hmd->gaze_xfer);
		hmd->gaze_xfer = nullptr;
	}
#endif // SUPPORT_EYE_TRACKING
}

static void psvr2_hmd_destroy(psvr2_hmd* hmd)
{
	stop_gaze_keepalive_thread(hmd);

	//os_thread_helper_lock(&hmd->usb_thread);
	hmd->usb_complete = 1;
	//os_thread_helper_unlock(&hmd->usb_thread);
	//os_thread_helper_destroy(&hmd->usb_thread);

	psvr2_usb_stop(hmd);
	psvr2_usb_destroy(hmd);

	// if (hmd->dev != NULL) 
	// {
	// 	libusb_close(hmd->dev);
	// }

	if(hmd->ctx != NULL)
	{
		libusb_exit(hmd->ctx);
		hmd->ctx = nullptr;
	}

	//m_ff_vec3_f32_free(&hmd->ff_gyro);
	//m_relation_history_destroy(&hmd->slam_relation_history);
	//os_mutex_destroy(&hmd->data_lock);
	//u_device_free(&hmd->base);
}

int main(int argc, char* argv[])
{
	const std::string welcome_str = "PSVR 2 Direct Gaze Reader\n\n";
	printf(welcome_str.c_str());

	psvr2_hmd hmd = {};

	bool usb_open_ok = psvr2_usb_open(&hmd);

	if(!usb_open_ok)
	{
		return -1;
	}

	bool start_ok = psvr2_usb_start(&hmd);

	if(!start_ok)
	{
		return -1;
	}

	bool keep_going = true;

	while(keep_going)
	{
		std::this_thread::sleep_for(4ms);
	}

	psvr2_hmd_destroy(&hmd);

	return 0;
}
