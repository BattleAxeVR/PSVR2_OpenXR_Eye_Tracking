#ifndef PSVR2_STRUCTS_H
#define PSVR2_STRUCTS_H

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

#define SERIAL_LENGTH 14

#define GYRO_SCALE (2000.0 / 32767.0)
#define ACCEL_SCALE (4.0 * MATH_GRAVITY_M_S2 / 32767.0)

#define IMU_FREQ 2000.0f
#define IMU_PERIOD_NS ((time_duration_ns)(1000000000.0f / IMU_FREQ))

typedef uint32_t uint;


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

struct imu_usb_record
{
	uint vts_us;
	int16_t accel[3];
	int16_t gyro[3];
	int16_t dp_frame_cnt;
	int16_t dp_line_cnt;
	int16_t imu_ts_us;
	int16_t status;
};// __attribute__((packed));

struct status_record_hdr
{
	uint8_t dprx_status;      //< 0 = not ready. 2 = cinematic? and 1 = unknown. HDCP? Other?
	uint8_t prox_sensor_flag; //< 0 = not triggered. 1 = triggered?
	uint8_t function_button;  //< 0 = not pressed, 1 = pressed
	uint8_t empty0[2];
	uint8_t ipd_dial_mm; //< 59 to 72mm

	uint8_t remainder[26];
};// __attribute__((packed));

struct slam_record
{
	uint32_t vts_us;  //< Timestamp of the SLAM, in microseconds
	double pos[3];    //< 32-bit floats
	double orient[4]; //< Orientation quaternion
	uint8_t remainder[470];
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
};// __attribute__((packed));

struct sie_ctrl_pkt
{
	int16_t report_id;
	int16_t subcmd;
	uint len;
	uint8_t data[512 - 8];
};// __attribute__((packed));

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

enum psvr2_report_ids
{
	SET_PERIPHERAL_ = 0x8,
	SET_CAMERA_MODE_ = 0xB,
	SET_GAZE_STREAM_ = 0xC,
	SET_GAZE_CALIBRATION_ = 0xD,
	SET_BRIGHTNESS_ = 0x12,
};

struct psvr2_per_eye_gaze
{
	uint is_gaze_point_valid = 0;
	vec3 gaze_point;

	uint is_gaze_direction_valid = 0;
	vec3 gaze_direction;

	uint is_pupil_diameter_valid = 0;
	float pupil_diameter = 0.0f;

	uint dummy0 = 0;
	vec2 dummy1;

	uint dummy3 = 0;
	vec2 dummy4;

	uint is_blink_state_valid = 0;
	uint blink_state = 0;
};

struct psvr2_combined_gaze
{
	uint is_gaze_point_valid = 0;
	vec3 gaze_point;

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

	psvr2_per_eye_gaze left_gaze_;
	psvr2_per_eye_gaze right_gaze_;
	psvr2_combined_gaze combined_gaze_;
};

struct psvr2_gaze_state
{
	USHORT header = 0;
	USHORT version = 2;

	psvr2_gaze_packet gaze_data_;
};

#endif