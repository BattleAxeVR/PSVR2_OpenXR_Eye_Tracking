//--------------------------------------------------------------------------------------
// Copyright (c) 2025 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

// Author: Bela Kampis
// Date: July 12th, 2025

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

#if ENABLE_BOTH_PSVR2_LAYERS

#include "psvr2_tracker.h"

#include "utils.h"
#include <log.h>
#include <util.h>

#include "trackers.h"

namespace openxr_api_layer 
{
using namespace log;

PSVR2_Tracker::PSVR2_Tracker() 
{
}

void PSVR2_Tracker::start(XrSession session) 
{
}

void PSVR2_Tracker::stop() 
{
}

void PSVR2_Tracker::update() 
{
    psvr2_eye_tracker_.update_gazes();
}

bool PSVR2_Tracker::isGazeAvailable(XrTime time, int eye) const
{
    const bool is_combined = (eye == BVR::BOTH_EYES);

    if (is_combined)
    {
        return psvr2_eye_tracker_.is_combined_gaze_available();
    }

    return psvr2_eye_tracker_.is_gaze_available(eye);
}

bool PSVR2_Tracker::getGaze(XrTime time, int eye, XrVector3f& unitVector, bool& is_open) 
{
    if (!psvr2_eye_tracker_.is_connected()) 
    {
        psvr2_eye_tracker_.connect();

        if (!psvr2_eye_tracker_.is_connected()) 
        {
            return false;
        }
    }
	
    const bool is_combined = (eye == BVR::BOTH_EYES);
    const bool is_left_eye = (eye == BVR::LEFT);

	if (is_combined || is_left_eye)
    {
        update();
    }

	if (is_combined)
	{
		const bool combined_gaze_ok = psvr2_eye_tracker_.get_combined_gaze(unitVector, false);
		return combined_gaze_ok;	
	}
	
    const bool per_eye_gaze_ok = psvr2_eye_tracker_.get_per_eye_gaze(eye, unitVector, false);
    return per_eye_gaze_ok;
}

TrackerType PSVR2_Tracker::getType() const 
{
    return TrackerType::PSVR2_TRACKERS;
}

} // namespace openxr_api_layer


#endif // ENABLE_BOTH_PSVR2_LAYERS