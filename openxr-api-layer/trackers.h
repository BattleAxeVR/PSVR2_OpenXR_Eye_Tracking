// MIT License
//
// Copyright(c) 2022-2023 Matthieu Bucchianeri
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

#pragma once

#include "defines.h"

namespace openxr_api_layer {

    struct EyeTrackerNotSupportedException : public std::exception {
        const char* what() const throw() {
            return "Eye tracker is not supported";
        }
    };

    enum class TrackerType {
        None = 0,
        EyeGazeInteraction, // Passthru
        Simulated,
#ifdef _WIN64
        Omnicept,
#endif
        Varjo,
        QuestPro,
        Pimax,
        VirtualDesktop,
        SteamLink,
        OpenXr,
#if ENABLE_BOTH_PSVR2_LAYERS
		PSVR2_TRACKERS,
#else
#if ENABLE_DUMMY_EXT_COMBINED_GAZE
        DUMMY_EXT_COMBINED_GAZE,
#endif
#if ENABLE_PSVR2_EXT_COMBINED_GAZE
        PSVR2_EXT_COMBINED_GAZE,
#endif
#if ENABLE_DUMMY_SOCIAL_GAZES
        DUMMY_SOCIAL_GAZES,
#endif
#if ENABLE_PSVR2_SOCIAL_GAZES
        PSVR2_SOCIAL_GAZES,
#endif
#endif // ENABLE_BOTH_PSVR2_LAYERS
        
    };

    static inline std::string getTrackerType(TrackerType type) {
        switch (type) {
        case TrackerType::None:
            return "None";
        case TrackerType::EyeGazeInteraction:
            return "Passthrough";
        case TrackerType::Simulated:
            return "Simulated";
#ifdef _WIN64
        case TrackerType::Omnicept:
            return "HP Omnicept";
#endif
        case TrackerType::Varjo:
            return "Varjo";
        case TrackerType::QuestPro:
            return "Quest Pro";
        case TrackerType::Pimax:
            return "Pimax";
        case TrackerType::VirtualDesktop:
            return "Virtual Desktop";
        case TrackerType::SteamLink:
            return "Steam Link";
        case TrackerType::OpenXr:
            return "OpenXR";
#if ENABLE_BOTH_PSVR2_LAYERS
		case TrackerType::PSVR2_TRACKERS:
			return "PSVR2_TRACKERS";
#else
#if ENABLE_DUMMY_EXT_COMBINED_GAZE
        case TrackerType::DUMMY_EXT_COMBINED_GAZE:
            return "DUMMY_EXT_COMBINED_GAZE";
#endif
#if ENABLE_PSVR2_EXT_COMBINED_GAZE
        case TrackerType::PSVR2_EXT_COMBINED_GAZE:
            return "PSVR2_EXT_COMBINED_GAZE";
#endif
#if ENABLE_DUMMY_SOCIAL_GAZES
        case TrackerType::DUMMY_SOCIAL_GAZES:
            return "DUMMY_SOCIAL_GAZES";
#endif
#if ENABLE_PSVR2_SOCIAL_GAZES
        case TrackerType::PSVR2_SOCIAL_GAZES:
            return "PSVR2_SOCIAL_GAZES";
#endif
#endif // ENABLE_BOTH_PSVR2_LAYERS
        }
        return "<Unknown>";
    }

    struct IEyeTracker {
        virtual ~IEyeTracker() = default;

        virtual void start(XrSession session) = 0;
        virtual void stop() = 0;
        virtual void update() = 0;
        virtual bool isGazeAvailable(XrTime time, int eye) const = 0;
        virtual bool getGaze(XrTime time, int eye, XrVector3f& unitVector, bool& is_open) = 0;
        virtual TrackerType getType() const = 0;
    };

#if SUPPORT_SIMULATED_EYE_TRACKING
    std::unique_ptr<IEyeTracker> createSimulatedEyeTracker();
#endif

#if SUPPORT_QUEST_PRO
    std::unique_ptr<IEyeTracker> createQuestProEyeTracker(OpenXrApi& openXrApi);
#endif

#if SUPPORT_VIRTUAL_DESKTOP
    std::unique_ptr<IEyeTracker> createVirtualDesktopEyeTracker();
#endif

#if SUPPORT_STEAMLINK
    std::unique_ptr<IEyeTracker> createSteamLinkEyeTracker();
#endif

#if ENABLE_BOTH_PSVR2_LAYERS
    std::unique_ptr<IEyeTracker> createPSVR2_Tracker();
#else
#if ENABLE_DUMMY_EXT_COMBINED_GAZE
    std::unique_ptr<IEyeTracker> create_Dummy_EXT_Combined_Gaze();
#endif

#if ENABLE_PSVR2_EXT_COMBINED_GAZE
    std::unique_ptr<IEyeTracker> createPSVR2_OpenXR_EXT();
#endif

#if ENABLE_DUMMY_SOCIAL_GAZES
    std::unique_ptr<IEyeTracker> create_Dummy_Social_Gazes();
#endif

#if ENABLE_PSVR2_SOCIAL_GAZES
    std::unique_ptr<IEyeTracker> createPSVR2_Social_Gazes();
#endif

#endif // ENABLE_BOTH_PSVR2_LAYERS

} // namespace openxr_api_layer
