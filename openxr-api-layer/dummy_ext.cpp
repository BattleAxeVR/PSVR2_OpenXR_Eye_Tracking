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

#if ENABLE_DUMMY_EXT_COMBINED_GAZE

#include "dummy_ext.h"

#include "layer.h"
#include "utils.h"
#include <log.h>
#include <util.h>

#include "trackers.h"

namespace openxr_api_layer 
{
Dummy_EXT_Combined_Gaze::Dummy_EXT_Combined_Gaze()
{
}

void Dummy_EXT_Combined_Gaze::start(XrSession session) 
{
}

void Dummy_EXT_Combined_Gaze::stop() 
{
}

void Dummy_EXT_Combined_Gaze::update() 
{
}

bool Dummy_EXT_Combined_Gaze::isGazeAvailable(XrTime time, int eye) const 
{
    return true;
}

bool Dummy_EXT_Combined_Gaze::getGaze(XrTime time, int eye, XrVector3f& unitVector, bool& is_open) 
{
    unitVector.x = 0.0f;
    unitVector.y = 0.0f;
    unitVector.z = -1.0f;

    is_open = true;

    return true;
}

TrackerType Dummy_EXT_Combined_Gaze::getType() const 
{
    return TrackerType::DUMMY_EXT_COMBINED_GAZE;
}

} // namespace openxr_api_layer

#endif // ENABLE_DUMMY_EXT_COMBINED_GAZE