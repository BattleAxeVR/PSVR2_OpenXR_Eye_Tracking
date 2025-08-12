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

#if ENABLE_BOTH_PSVR2_LAYERS

#include "psvr2_eye_tracking.h"

#include "trackers.h"

namespace openxr_api_layer 
{
struct PSVR2_Tracker : IEyeTracker 
{
    PSVR2_Tracker();

    void start(XrSession session) override;
    void stop() override;
    void update() override;
    bool isGazeAvailable(XrTime time, int eye) const override;
    bool getGaze(XrTime time, int eye, XrVector3f& unitVector, bool& is_open) override;
    TrackerType getType() const override;

    private:
    BVR::PSVR2EyeTracker psvr2_eye_tracker_;
};

std::unique_ptr<IEyeTracker> createPSVR2_Tracker() 
{
    return std::make_unique<PSVR2_Tracker>();
}

} // namespace openxr_api_layer


#endif // ENABLE_BOTH_PSVR2_LAYERS