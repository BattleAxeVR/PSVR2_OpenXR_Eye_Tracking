#ifndef PSVR2_STRUCTS_H
#define PSVR2_STRUCTS_H

#include <stdint.h>
#include <libusb.h>
#include <mutex>
#include <thread>

#define SUPPORT_PSVR2_SLAM_TRACKING 0
#define SUPPORT_PSVR2_STATUS 0
#define SUPPORT_PSVR2_LED_DETECTOR 0
#define SUPPORT_PSVR2_RELOCALIZER 0
#define SUPPORT_PSVR2_VD 0

#define SUPPORT_PSVR2_EYE_TRACKING 1

#define SUPPORT_PSVR2_PER_EYE_GAZES (SUPPORT_PSVR2_EYE_TRACKING && 1)
#define ENABLE_DEBUG_LOG_PER_EYE_GAZES (SUPPORT_PSVR2_PER_EYE_GAZES && 0)

#define SUPPORT_PSVR2_COMBINED_GAZE (SUPPORT_PSVR2_EYE_TRACKING && 1)
#define ENABLE_DEBUG_LOG_COMBINED_GAZE (SUPPORT_PSVR2_COMBINED_GAZE && 1)

#define SUPPORT_SONY_ET_CALIBRATION (SUPPORT_PSVR2_EYE_TRACKING && 0)
#define SUPPORT_FILTERED_GAZE_DIRECTIONS (SUPPORT_PSVR2_EYE_TRACKING && 0)
#define SUPPORT_LERPED_BLINK_STATES (SUPPORT_PSVR2_EYE_TRACKING && 0)

#define SUPPORT_PSVR2_CAMERAS 0
#define SUPPORT_FACE_TRACKING (SUPPORT_PSVR2_EYE_TRACKING && 0)

#ifndef NUM_EYES
#define NUM_EYES 2
#endif

#ifndef LEFT
#define LEFT 0
#endif

#ifndef RIGHT
#define RIGHT 1
#endif

#define NUM_CAM_XFERS 1

#define TIMESTAMP_SAMPLES 100

#define PSVR2_SLAM_INTERFACE 3
#define PSVR2_SLAM_ENDPOINT 3

#define PSVR2_GAZE_INTERFACE 5
#define PSVR2_GAZE_ENDPOINT 0x85

#define PSVR2_CAMERA_INTERFACE 6
#define PSVR2_CAMERA_ENDPOINT 7

#define PSVR2_STATUS_INTERFACE 7
#define PSVR2_STATUS_ENDPOINT 8

#define PSVR2_LD_INTERFACE 8
#define PSVR2_LD_ENDPOINT 9

#define PSVR2_RP_INTERFACE 9
#define PSVR2_RP_ENDPOINT 10

#define PSVR2_VD_INTERFACE 10
#define PSVR2_VD_ENDPOINT 11

#define USB_SLAM_XFER_SIZE 1024
#define USB_STATUS_XFER_SIZE 1024
#define USB_GAZE_XFER_SIZE 32768
#define USB_CAM_MODE10_XFER_SIZE 1040640
#define USB_CAM_MODE1_XFER_SIZE 819456
#define USB_LD_XFER_SIZE 36944
#define USB_RP_XFER_SIZE 821120
#define USB_VD_XFER_SIZE 32768

#define PSVR2_VID 0x054C
#define PSVR2_PID 0x0CDE

#define SERIAL_LENGTH 14

#define GYRO_SCALE (2000.0 / 32767.0)
#define ACCEL_SCALE (4.0 * MATH_GRAVITY_M_S2 / 32767.0)

#define IMU_FREQ 2000.0f
#define IMU_PERIOD_NS ((time_duration_ns)(1000000000.0f / IMU_FREQ))

#define METERS_TO_MM 1000.0f
#define MM_TO_METERS 0.001f

typedef uint32_t uint;
typedef uint64_t timepoint_ns;

struct vec2
{
	float x = 0.0f;
	float y = 0.0f;
};

struct vec3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

vec3 convert_m_to_mm(const vec3& input_m);
vec3 convert_mm_to_m(const vec3& input_mm);
vec3 convert_psvr2_direction_to_openxr(const vec3& psvr2_direction);
float square(const float input);
vec3 safe_normalize(const vec3& input);

#if SUPPORT_PSVR2_SLAM_TRACKING
struct imu_record
{
	uint vts_us;
	int16_t accel[3];
	int16_t gyro[3];
	uint16_t dp_frame_cnt;
	uint16_t dp_line_cnt;
	uint16_t imu_ts_us;
	uint16_t status;
};

struct slam_record
{
	uint32_t vts_us;  //< Timestamp of the SLAM, in microseconds
	double pos[3];    //< 32-bit floats
	double orient[4]; //< Orientation quaternion
	uint8_t remainder[470];
};

#pragma pack(push, 1)
struct imu_usb_record
{
	uint vts_us;
	int16_t accel[3];
	int16_t gyro[3];
	int16_t dp_frame_cnt;
	int16_t dp_line_cnt;
	int16_t imu_ts_us;
	int16_t status;
};

struct slam_usb_record
{
	char SLAhdr[3];    //< "SLA"
	uint8_t const1;    //< Constant 0x01?
	uint pkt_size;   //< 0x0200 = 512 bytes;
	uint vts_ts_us;  //< Timestamp
	uint unknown1;   //< Unknown. Constant 3?
	float pos[3];    //< 32-bit floats
	float orient[4]; //< Orientation quaternion
	uint8_t remainder[468];
};
#pragma pack(pop)

#endif // SUPPORT_PSVR2_SLAM_TRACKING

#if SUPPORT_PSVR2_STATUS
#pragma pack(push, 1)
struct status_record_hdr
{
	uint8_t dprx_status;      //< 0 = not ready. 2 = cinematic? and 1 = unknown. HDCP? Other?
	uint8_t prox_sensor_flag; //< 0 = not triggered. 1 = triggered?
	uint8_t function_button;  //< 0 = not pressed, 1 = pressed
	uint8_t empty0[2];
	uint8_t ipd_dial_mm; //< 59 to 72mm

	uint8_t remainder[26];
};
#pragma pack(pop)

#endif // SUPPORT_PSVR2_STATUS

// Sony control packets are needed no matter what
#pragma pack(push, 1)
struct sie_ctrl_pkt
{
	int16_t report_id;
	int16_t subcmd;
	uint len;
	uint8_t data[512 - 8];
};
#pragma pack(pop)


#if SUPPORT_PSVR2_CAMERAS
enum psvr2_camera_mode
{
	PSVR2_CAMERA_MODE_OFF = 0,
	// 819456 byte 640x640x2 SBS bottom cameras
	PSVR2_CAMERA_MODE_BOTTOM_SBS_CROPPED = 1,
	// 819456 byte 640x640x2 SBS (mode 1) + 409856 byte 640x640 frames (from top cameras alternately) interleaved
	PSVR2_CAMERA_MODE_2 = 2,
	// 819456 byte 640x640x2 SBS interleaved bottom and top camera paired images
	PSVR2_CAMERA_MODE_3 = 3,
	// 520448 byte 512x508x2 Top-Bottom fisheye, *Controller Tracking* interleaved top and bottom camera pairs
	PSVR2_CAMERA_MODE_4 = 4,
	// 80256 byte 400x200 nearly black (no value higher than 0x0f)
	PSVR2_CAMERA_MODE_400_200_DARK = 5,
	// no packets / off
	PSVR2_CAMERA_MODE_EYE_CAMERAS = 6,
	// 819456 byte 640x640x2 + 520448 byte 512x508x2 fisheye *Controller Tracking* alternating bottom camera
	PSVR2_CAMERA_MODE_7 = 7,
	// 819456 byte 640x640x2 SBS bottom cameras + 80256 byte 400x200 nearly black packets like mode 5
	PSVR2_CAMERA_MODE_8 = 8,
	// interleaved mode 2 + mode 5 packets
	PSVR2_CAMERA_MODE_9 = 9,
	// 640x640x2 SBS interleaved bottom and top cameras + 80256 byte mode 5 packets
	PSVR2_CAMERA_MODE_10 = 0xa,
	// Mode 4 + Mode 5 packets interleaved
	PSVR2_CAMERA_MODE_11 = 0xb,
	// 409856 byte 320x640x4 vertical stack of all 4 cameras + 260352 byte 256x254x4 vertical stack fisheye
	// controller-tracking all-4-cameras packets interleaved
	PSVR2_CAMERA_MODE_12 = 0xc,
	// mode 1, but upside down
	PSVR2_CAMERA_MODE_13 = 0xd,
	// mode 1 upside down + mode 4 bottom cameras only
	PSVR2_CAMERA_MODE_14 = 0xe,
	// mode 0xc + mode 5
	PSVR2_CAMERA_MODE_15 = 0xf,
	// 1024x1024x2 BC4 compressed images (really 1000x1000, with 24 padding pixels you can ignore on the
	// right/bottom)
	PSVR2_CAMERA_MODE_BOTTOM_SBS_BC4 = 0x10,
};
#endif // SUPPORT_PSVR2_CAMERAS

enum psvr2_report_id
{
	PSVR2_REPORT_ID_SET_PERIPHERAL = 0x8,
	PSVR2_REPORT_ID_SET_CAMERA_MODE = 0xB,
	PSVR2_REPORT_ID_SET_GAZE_STREAM = 0xC,
	PSVR2_REPORT_ID_SET_GAZE_USER_CALIBRATION = 0xD,
	PSVR2_REPORT_ID_SET_BRIGHTNESS = 0x12,
};

enum psvr2_gaze_stream_subcommand
{
	PSVR2_GAZE_STREAM_SUBCMD_ENABLE = 0x01,
	PSVR2_GAZE_STREAM_SUBCMD_DISABLE = 0x02,
};

enum psvr2_set_peripheral_subcommand
{
	PSVR2_SET_PERIPHERAL_SUBCMD_MOTOR = 0x01,
};

#if SUPPORT_PSVR2_EYE_TRACKING
struct psvr2_per_eye_gaze
{
	uint is_gaze_point_valid = 0;
	vec3 gaze_point_mm;

	uint is_gaze_direction_valid = 0;
	vec3 gaze_direction;

	uint is_pupil_diameter_valid = 0;
	float pupil_diameter_mm = 0.0f;

	uint dummy0 = 0;
	vec2 dummy1;

	uint dummy3 = 0;
	vec2 dummy4;

	uint is_blink_state_valid = 0;
	uint blink_state = 0;
};

struct openxr_per_eye_gaze
{
	uint is_gaze_point_valid = 0;
	vec3 gaze_point_m;

	uint is_gaze_direction_valid = 0;
	vec3 gaze_direction;

	uint is_pupil_diameter_valid = 0;
	float pupil_diameter_m = 0.0f;

	uint is_blink_state_valid = 0;
	uint blink_state = 0;
};

struct psvr2_combined_gaze
{
	uint is_gaze_point_valid = 0;
	vec3 gaze_point_mm;

	uint is_normalized_gaze_direction_valid = 0;
	vec3 normalized_gaze_direction;

	uint is_valid = 0;
	uint timestamp = 0;

	uint dummy0 = 0;
	float dummy1 = 0.0f;

	uint dummy2 = 0;
	vec3 dummy3;
	vec3 dummy4;
	vec3 dummy5;
};

struct openxr_combined_gaze
{
	uint is_gaze_point_valid = 0;
	vec3 gaze_point_m;

	uint is_normalized_gaze_direction_valid = 0;
	vec3 normalized_gaze_direction;

	uint is_valid = 0;
	uint timestamp = 0;
};

struct psvr2_gaze_packet
{
	uint size = 0;
	uint dummy0 = 0;
	uint dummy1 = 0;
	uint dummy2 = 0;

	uint timestamp1 = 0;
	uint timestamp2 = 0;
	uint timestamp3 = 0;

	uint dummy3 = 0;
	float dummy4 = 0.0f;
	uint dummy5 = 0;
	uint dummy6 = 0;
	float dummy7 = 0.0f;
	uint dummy8 = 0;
	uint dummy9 = 0;
	uint dummy10 = 0;
	uint dummy11 = 0;
	uint dummy12 = 0;
	float dummy13 = 0.0f;

	uint dumm14 = 0;
	float dummy15 = 0.0f;

	uint dumm16 = 0;
	float dummy17 = 0.0f;

	psvr2_per_eye_gaze gazes_[NUM_EYES];
	psvr2_combined_gaze combined_gaze_;
};

struct psvr2_gaze_state
{
	USHORT header = 0;
	USHORT version = 2;

	psvr2_gaze_packet gaze_data_;
};

struct openxr_eye_tracking_data
{
	std::thread eye_tracking_thread;
	std::mutex data_mutex;

	bool want_enabled = true;
	bool enabled = false;

	//m_relation_history* gaze_relation_history = nullptr;
	
	openxr_per_eye_gaze openxr_gazes_[NUM_EYES];
	openxr_combined_gaze openxr_combined_gaze_;

	bool processed_sample_packet = false;

	uint32_t last_remote_report_sample_time_us = 0;
	timepoint_ns last_remote_report_sample_time_ns = 0;

};
#endif // SUPPORT_PSVR2_EYE_TRACKING

struct psvr2_hmd
{
	libusb_context* ctx = nullptr;
	libusb_device_handle* dev = nullptr;

	int usb_complete = 0;
	int usb_active_xfers = 0;

	std::thread usb_thread;
	std::mutex data_lock;
	//xrt_pose pose;

	//xrt_atomic_s32_t proximity_sensor; //< Atomic state for whether the proximity sensor is triggered

	bool function_button = false;//< Boolean state for whether the function button is pressed

	bool ipd_updated = false; //< Whether the IPD has been updated, and an HMD info refresh is needed
	float ipd_mm = 67.0f;   //< IPD dial value in mm, from 59 to 72mm

#if SUPPORT_PSVR2_CAMERAS
	bool camera_enable = false; //< Whether the camera is enabled
	psvr2_camera_mode camera_mode; //< The current camera mode

	//u_var_button camera_enable_btn;
	//u_var_button camera_mode_btn;

	// Camera (bulk) transfers
	libusb_transfer* camera_xfers[NUM_CAM_XFERS] = {};

	// Camera debug sinks
	//u_sink_debug debug_sinks[4];
#endif

	//u_var_button brightness_btn;
	float brightness = 1.0f;

#if SUPPORT_PSVR2_SLAM_TRACKING
	// IMU input data
	uint last_imu_vts_us = 0;   //< Last VTS timestamp, in microseconds
	uint16_t last_imu_ts = 0; //< Last IMU timestamp, in microseconds

	//xrt_vec3 last_gyro;  //< Last gyro reading, in rad/s
	//xrt_vec3 last_accel; //< Last accel reading, in m/s²

	// SLAM input data
	uint last_slam_vts_us = 0;      //< Last slam timestamp, in microseconds
	//xrt_pose last_slam_pose; //< Last SLAM pose reading

	//xrt_pose slam_correction_pose;
	//u_var_button slam_correction_set_btn;
	//u_var_button slam_correction_reset_btn;

	//xrt_pose T_imu_head; //< Constant transform from SLAM tracker pose to head pose

	// Display parameters
	//u_device_simple_info info;

	// SLAM (bulk) transfer
	libusb_transfer* slam_xfer = nullptr;

	timepoint_ns last_imu_vts_ns = 0;
	timepoint_ns last_slam_vts_ns = 0;

	// Tracking state 
	//m_relation_history* slam_relation_history = nullptr;
	//m_ff_vec3_f32* ff_gyro = nullptr;
#endif

#if SUPPORT_PSVR2_STATUS
	uint8_t dprx_status = 0; //< DisplayPort receiver status
	libusb_transfer* status_xfer = nullptr;
#endif // SUPPORT_PSVR2_STATUS

#if SUPPORT_PSVR2_LED_DETECTOR
	// LD EP9 (bulk) transfer
	libusb_transfer* led_detector_xfer = nullptr;
#endif // SUPPORT_PSVR2_LED_DETECTOR

#if SUPPORT_PSVR2_RELOCALIZER
	// RP EP10 (bulk) transfer
	libusb_transfer* relocalizer_xfer = nullptr;
#endif // SUPPORT_PSVR2_RELOCALIZER

#if SUPPORT_PSVR2_VD
	// VD EP11 (bulk) transfer
	libusb_transfer* vd_xfer = nullptr;
#endif // SUPPORT_PSVR2_VD

#if SUPPORT_PSVR2_EYE_TRACKING
	// Gaze transfer
	libusb_transfer* gaze_xfer = nullptr;
#endif

	/* Distortion calibration parameters, to be used with
	 * psvr2_compute_distortion_asymmetric. Very specific to
	 * PS VR2. */
	float distortion_calibration[8];

	// Timing data
	int timestamp_samples = 0;

	timepoint_ns system_zero_ns = 0;
	timepoint_ns last_imu_ns = 0;

	//time_duration_ns hw2mono_vts = 0;
	//time_duration_ns hw2mono_imu = 0;

#if SUPPORT_PSVR2_EYE_TRACKING
	openxr_eye_tracking_data openxr_eye_tracking_data_;
#endif

#if SUPPORT_FACE_TRACKING
	bool face_feature_enabled = true;
#endif
};

enum psvr2_hmd_input_name
{
	PSVR2_HMD_INPUT_HEAD_POSE,
	PSVR2_HMD_INPUT_FUNCTION_BUTTON,
	PSVR2_HMD_INPUT_EYE_GAZE_POSE,
	PSVR2_HMD_INPUT_FB_FACE_TRACKING2_SOCIAL,
	PSVR2_HMD_INPUT_HTC_EYE_FACE_TRACKING,
	PSVR2_HMD_INPUT_ANDROID_FACE_TRACKING,
	PSVR2_HMD_INPUT_COUNT,
};

//void psvr2_compute_distortion_asymmetric(float* calibration, xrt_uv_triplet* distCoords, int eEye, float fU, float fV);

bool psvr2_usb_xfer_continue(libusb_transfer* xfer, const char* type);
bool send_psvr2_control(psvr2_hmd* hmd, uint16_t report_id, uint8_t subcmd, uint8_t* pkt_data, uint32_t pkt_len);

#if SUPPORT_PSVR2_EYE_TRACKING
void stop_gaze_keepalive_thread(psvr2_hmd* hmd);
int psvr2_start_gaze_tracking(psvr2_hmd* hmd);
#endif

#if SUPPORT_FACE_TRACKING
xrt_result_t psvr2_get_face_tracking(xrt_device* xdev, enum xrt_input_name facial_expression_type, int64_t at_timestamp_ns, xrt_facial_expression_set* out_value);
#endif

#endif // PSVR2_STRUCTS_H

