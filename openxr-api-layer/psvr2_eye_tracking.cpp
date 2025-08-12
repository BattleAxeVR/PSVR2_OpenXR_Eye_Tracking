//--------------------------------------------------------------------------------------
// Copyright (c) 2025 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

// Author: Bela Kampis
// Date: April 11, 2025

// MIT License
//
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "pch.h"
#include "defines.h"

#if ENABLE_PSVR2_EYE_TRACKING

#include "psvr2_eye_tracking.h"

namespace BVR 
{

template<typename T> static inline T bvr_clamp(T v, T mn, T mx)
{
	return (v < mn) ? mn : (v > mx) ? mx : v;
}

PSVR2EyeTracker::PSVR2EyeTracker()
{
}

bool PSVR2EyeTracker::connect()
{
	if(!is_connected_)
	{
		// Connect to pipe job
		while(true)
		{
			LPTSTR named_pipe_name = (LPTSTR)TEXT(PSVR2_SERVER_NAMED_PIPE_NAME);
			named_pipe_handle_ = CreateFile(named_pipe_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

			if(named_pipe_handle_ != INVALID_HANDLE_VALUE)
			{
				break;
			}

			if(GetLastError() != ERROR_PIPE_BUSY)
			{
				return false;
			}

			if(!WaitNamedPipe(named_pipe_name, PSVR2_SERVER_CONNECT_WAIT_TIME))
			{
				return false;
			}
		}

		DWORD mode = PIPE_READMODE_MESSAGE;

		if(!SetNamedPipeHandleState(named_pipe_handle_, &mode, 0, 0))
		{
			return false;
		}

		Response handshake_response;
		const bool handshake_ok = send_and_receive(Request(START_HANDSHAKE_), handshake_response);

		if(!handshake_ok || (handshake_response.type_ != ResponseType::HANDSHAKE_OK_))
		{
			return false;
		}

		is_connected_ = handshake_ok;

#if ENABLE_PSVR2_EYE_TRACKING_AUTOMATICALLY
		set_enabled(is_connected_);
#endif
	}

	return is_connected_;
}


void PSVR2EyeTracker::disconnect()
{
	if(is_connected_)
	{
		if(named_pipe_handle_ != INVALID_HANDLE_VALUE)
		{
			CloseHandle(named_pipe_handle_);
			named_pipe_handle_ = INVALID_HANDLE_VALUE;
		}

		is_connected_ = false;
	}
}

bool PSVR2EyeTracker::send_and_receive(const Request& request, Response& response) const
{
	const bool request_ok = send_request(request);
	const bool response_ok = request_ok && receive_request(response);

	return response_ok;
}

bool PSVR2EyeTracker::send_request(const Request& request) const
{
	DWORD write_size = 0;
	const BOOL write_ok = WriteFile(named_pipe_handle_, &request, sizeof(request), &write_size, 0);
	return write_ok;
}

bool PSVR2EyeTracker::receive_request(Response& response) const
{
	DWORD read_size = 0;

	const BOOL success = ReadFile(named_pipe_handle_, &response, sizeof(response), &read_size, 0);

	if(!success)
	{
		const DWORD lastError = GetLastError();

		if(lastError != ERROR_MORE_DATA)
		{
			return false;
		}
	}

	if(read_size != sizeof(response))
	{
		return false;
	}

	return true;
}


bool PSVR2EyeTracker::update_gazes()
{
	if(!is_connected_)
	{
		return false;
	}

	AllXRGazeStates xr_gaze_states = {};
	Response gaze_response;

	const bool gazes_ok = send_and_receive(Request(GET_GAZES_), gaze_response);

	if (gazes_ok)
	{
		memcpy(&xr_gaze_states, &gaze_response.gazes_, sizeof(xr_gaze_states));

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
		combined_gaze_ = xr_gaze_states.combined_gaze_;
#endif

#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
		per_eye_gazes_[LEFT] = xr_gaze_states.per_eye_gazes_[LEFT];
		per_eye_gazes_[RIGHT] = xr_gaze_states.per_eye_gazes_[RIGHT];
#endif

		return true;
	}

    return false;
}

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
bool PSVR2EyeTracker::is_combined_gaze_available() const
{
	if(!is_connected() || !is_enabled())
	{
		return false;
	}

	return combined_gaze_.is_valid_;
}
#endif


#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
bool PSVR2EyeTracker::is_gaze_available(const int eye) const
{
	if(!is_connected() || !is_enabled())
	{
		return false;
	}

	return per_eye_gazes_[eye].is_valid_;
}
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
bool PSVR2EyeTracker::get_combined_gaze(XrVector3f& combined_gaze_direction, const bool should_apply_gaze)
{
	(void)should_apply_gaze;

    if (combined_gaze_.is_valid_)
    {
#if ENABLE_GAZE_CALIBRATION
		if(apply_calibration_ && calibrations_[2].is_calibrated())
		{
			combined_gaze_direction = calibrations_[2].apply_calibration(combined_gaze_.direction_);
			return true;
		}
#endif

		combined_gaze_direction = combined_gaze_.direction_;
		return true;
	}

    return false;
}
#endif // ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE

#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
bool PSVR2EyeTracker::get_per_eye_gaze(const int eye, XrVector3f& per_eye_gaze_direction, const bool should_apply_gaze)
{
#if AUTO_CALIBRATE
	if(calibrations_[eye].is_calibrating() && (increment_countdown_ > 0))
	{
		increment_countdown_--;

		if(increment_countdown_ == 0)
		{
			increment_raster();
		}
	}
#endif

	if(per_eye_gazes_[eye].is_valid_)
	{
#if ENABLE_GAZE_CALIBRATION
		if (calibrations_[eye].is_calibrating())
		{
			CalibrationPoint& point = calibrations_[eye].get_raster_point();

			if (point.is_calibrated_)
			{
#if AUTO_INCREMENT_ON_CALIBRATION_DONE
				increment_raster();
#endif
			}
			else if (should_apply_gaze)
			{
				const bool sample_ok = point.add_sample(per_eye_gazes_[eye].direction_);

				if (sample_ok)
				{
					if(point.is_calibrated_)
					{
						calibrations_[eye].num_calibrated_++;
					}

					increment_countdown_ = AUTO_INCREMENT_COUNTDOWN;
				}
			}
		}

		if (apply_calibration_ && calibrations_[eye].is_calibrated())
		{
			per_eye_gaze_direction = calibrations_[eye].apply_calibration(per_eye_gazes_[eye].direction_);
			return true;
		}
#else
		(void)should_apply_gaze;
#endif
		
		per_eye_gaze_direction = per_eye_gazes_[eye].direction_;
		return true;
	}

    return false;
}
#endif // ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES

#if ENABLE_GAZE_CALIBRATION
void PSVR2EyeTracker::set_apply_calibration(const bool enabled)
{
	apply_calibration_ = enabled;
}

void PSVR2EyeTracker::toggle_apply_calibration()
{
	apply_calibration_ = !apply_calibration_;
}

void PSVR2EyeTracker::reset_calibrations()
{
#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
	calibrating_eye_index_ = INVALID_INDEX;
	calibrations_[LEFT_CALIBRATION_INDEX].reset_calibration();
	calibrations_[RIGHT_CALIBRATION_INDEX].reset_calibration();
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
	calibrations_[COMBINED_CALIBRATION_INDEX].reset_calibration();
#endif
}

bool PSVR2EyeTracker::load_calibrations()
{
	bool success = true;

#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
	success &= calibrations_[LEFT_CALIBRATION_INDEX].load_calibration();
	success &= calibrations_[RIGHT_CALIBRATION_INDEX].load_calibration();
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
	success &= calibrations_[COMBINED_CALIBRATION_INDEX].reset_calibration();
#endif

	return success;
}

bool PSVR2EyeTracker::save_calibrations()
{
	bool success = true;

#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
	success &= calibrations_[LEFT_CALIBRATION_INDEX].save_calibration();
	success &= calibrations_[RIGHT_CALIBRATION_INDEX].save_calibration();
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
	success &= calibrations_[COMBINED_CALIBRATION_INDEX].save_calibration();
#endif

	return success;
}

bool PSVR2EyeTracker::is_calibrating() const
{
#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
	if (is_eye_calibrating(LEFT) || is_eye_calibrating(RIGHT))
	{
		return true;
	}
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
	if(is_combined_calibrating())
	{
		return true;
	}
#endif

	return false;
}

bool PSVR2EyeTracker::is_fully_calibrated() const
{
	bool calibrated = true;

#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
	calibrated &= is_eye_calibrated(LEFT);
	calibrated &= is_eye_calibrated(RIGHT);
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
	calibrated &= is_combined_calibrated();
#endif

	return calibrated;
}

void PSVR2EyeTracker::start_calibrating()
{
	if (is_fully_calibrated() || is_calibrating())
	{
		return;
	}

#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
	if (!is_eye_calibrated(LEFT))
	{
		start_eye_calibration(LEFT);
		return;
	}
	else if(is_eye_calibrated(LEFT) && !is_eye_calibrated(RIGHT))
	{
		start_eye_calibration(RIGHT);
		return;
	}
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
	if(!is_combined_calibrated())
	{
		start_combined_calibration();
		return;
	}
#endif
}

void PSVR2EyeTracker::stop_calibrating()
{

}

void PSVR2EyeTracker::increment_raster()
{
#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
	if(calibrating_eye_index_ != INVALID_INDEX)
	{
		if(calibrations_[calibrating_eye_index_].is_calibrating())
		{
			return calibrations_[calibrating_eye_index_].increment_raster();
		}
	}
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
	if(calibrations_[COMBINED_CALIBRATION_INDEX].is_calibrating())
	{
		return calibrations_[COMBINED_CALIBRATION_INDEX].increment_raster();
	}
#endif
}

GLMPose PSVR2EyeTracker::get_calibration_cube() const
{
#if ENABLE_PSVR2_EYE_TRACKING_PER_EYE_GAZES
	if(calibrating_eye_index_ != INVALID_INDEX)
	{
		if(calibrations_[calibrating_eye_index_].is_calibrating())
		{
			return calibrations_[calibrating_eye_index_].get_calibration_cube();
		}
	}
#endif

#if ENABLE_PSVR2_EYE_TRACKING_COMBINED_GAZE
	if(calibrations_[COMBINED_CALIBRATION_INDEX].is_calibrating())
	{
		return calibrations_[COMBINED_CALIBRATION_INDEX].get_calibration_cube();
	}
#endif

	return {};
}

#endif

} // BVH


#endif // ENABLE_PSVR2_EYE_TRACKING
