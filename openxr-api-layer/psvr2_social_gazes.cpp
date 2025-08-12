//--------------------------------------------------------------------------------------
// Copyright (c) 2025 BattleAxeVR. All rights reserved.
//--------------------------------------------------------------------------------------

// Author: Bela Kampis
// Date: July 5th, 2025

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

#if ENABLE_PSVR2_SOCIAL_GAZES

#include "psvr2_social_gazes.h"

#include "utils.h"
#include <log.h>
#include <util.h>

#include "trackers.h"

namespace openxr_api_layer 
{
using namespace log;

PSVR2_Social_Gazes::PSVR2_Social_Gazes() 
{
}

void PSVR2_Social_Gazes::start(XrSession session) 
{
}

void PSVR2_Social_Gazes::stop() 
{
}

void PSVR2_Social_Gazes::update() 
{
    psvr2_eye_tracker_.update_gazes();
}

bool PSVR2_Social_Gazes::isGazeAvailable(XrTime time, int eye) const
{
    if (eye == INVALID_INDEX) 
    {
        return false;
    }

    return psvr2_eye_tracker_.is_gaze_available(eye);
}

bool PSVR2_Social_Gazes::getGaze(XrTime time, int eye, XrVector3f& unitVector, bool& is_open) 
{
    if (eye == INVALID_INDEX) 
    {
        return false;
    }

    if (!psvr2_eye_tracker_.is_connected()) 
    {
        psvr2_eye_tracker_.connect();

        if (!psvr2_eye_tracker_.is_connected()) 
        {
            return false;
        }
    }
            
    if (eye == BVR::LEFT)
    {
        update();
    }

    const bool gaze_ok = psvr2_eye_tracker_.get_per_eye_gaze(eye, unitVector, false);
    return gaze_ok;
}

TrackerType PSVR2_Social_Gazes::getType() const 
{
    return TrackerType::PSVR2_SOCIAL_GAZES;
}

} // namespace openxr_api_layer


#endif // ENABLE_PSVR2_SOCIAL_GAZES