// MIT License
//
// Copyright(c) 2022-2023 Matthieu Bucchianeri
// Based on https://github.com/mbucchia/OpenXR-Layer-Template.
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

#include "layer.h"
#include "utils.h"
#include <log.h>
#include <util.h>

#include "trackers.h"

namespace openxr_api_layer 
{
    using namespace log;
    using namespace xr::math;

    // Our API layer implement these extensions, and their specified version.
    const std::vector<std::pair<std::string, uint32_t>> advertisedExtensions = {

#if ENABLE_BOTH_PSVR2_LAYERS
    std::make_pair(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME, 2), std::make_pair(XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME, 1)};
	const std::vector<std::string> blockedExtensions = { XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME, XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME };
#elif ENABLE_SOCIAL_GAZES
    std::make_pair(XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME, 1) };
    const std::vector<std::string> blockedExtensions = {XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME};
#elif ENABLE_EXT_GAZE_INTERACTION
    std::make_pair(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME, 2) };
    const std::vector<std::string> blockedExtensions = {XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME};
#endif

    const std::vector<std::string> implicitExtensions = {XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME, XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME};

    // This class implements our API layer.
    class OpenXrLayer : public openxr_api_layer::OpenXrApi {
      public:
        OpenXrLayer() = default;
        ~OpenXrLayer() = default;

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetInstanceProcAddr
        XrResult xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) override {
            TraceLoggingWrite(g_traceProvider,
                              "xrGetInstanceProcAddr",
                              TLXArg(instance, "Instance"),
                              TLArg(name, "Name"),
                              TLArg(m_bypassApiLayer, "Bypass"));

            XrResult result = m_bypassApiLayer ? m_xrGetInstanceProcAddr(instance, name, function)
                                               : OpenXrApi::xrGetInstanceProcAddr(instance, name, function);

            TraceLoggingWrite(g_traceProvider, "xrGetInstanceProcAddr", TLPArg(*function, "Function"));

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateInstance
        XrResult xrCreateInstance(const XrInstanceCreateInfo* createInfo) override {
            if (createInfo->type != XR_TYPE_INSTANCE_CREATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            // Needed to resolve the requested function pointers.
            OpenXrApi::xrCreateInstance(createInfo);

            // Dump the application name, OpenXR runtime information and other useful things for debugging.
            TraceLoggingWrite(g_traceProvider,
                              "xrCreateInstance",
                              TLArg(xr::ToString(createInfo->applicationInfo.apiVersion).c_str(), "ApiVersion"),
                              TLArg(createInfo->applicationInfo.applicationName, "ApplicationName"),
                              TLArg(createInfo->applicationInfo.applicationVersion, "ApplicationVersion"),
                              TLArg(createInfo->applicationInfo.engineName, "EngineName"),
                              TLArg(createInfo->applicationInfo.engineVersion, "EngineVersion"),
                              TLArg(createInfo->createFlags, "CreateFlags"));
            Log(fmt::format("Application: {}\n", createInfo->applicationInfo.applicationName));

            for (uint32_t i = 0; i < createInfo->enabledApiLayerCount; i++) {
                TraceLoggingWrite(
                    g_traceProvider, "xrCreateInstance", TLArg(createInfo->enabledApiLayerNames[i], "ApiLayerName"));
            }

#if ENABLE_SOCIAL_GAZES
            // Bypass the API layer unless the application requested the eye gaze interaction extension.
            bool requestedSocialGazes = false;

            for (uint32_t i = 0; i < createInfo->enabledExtensionCount; i++) {
                const std::string_view ext(createInfo->enabledExtensionNames[i]);
                TraceLoggingWrite(g_traceProvider, "xrCreateInstance", TLArg(ext.data(), "ExtensionName"));
                if (ext == XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME) {
                    requestedSocialGazes = true;
                }
            }

            if (requestedSocialGazes)
			{
                m_bypassApiLayer = false;
            }
#endif
            
#if ENABLE_EXT_GAZE_INTERACTION
            // Bypass the API layer unless the application requested the eye gaze interaction extension.
            bool requestedEyeGazeInteraction = false;

            for (uint32_t i = 0; i < createInfo->enabledExtensionCount; i++) {
                const std::string_view ext(createInfo->enabledExtensionNames[i]);
                TraceLoggingWrite(g_traceProvider, "xrCreateInstance", TLArg(ext.data(), "ExtensionName"));
                if (ext == XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME) {
                    requestedEyeGazeInteraction = true;
                }
            }

			if(requestedEyeGazeInteraction)
			{
				m_bypassApiLayer = false;
			}
#endif

            if (m_bypassApiLayer) 
            {
                Log(fmt::format("{} layer will be bypassed\n", LayerName));
                return XR_SUCCESS;
            }

            XrInstanceProperties instanceProperties = {XR_TYPE_INSTANCE_PROPERTIES};
            CHECK_XRCMD(OpenXrApi::xrGetInstanceProperties(GetXrInstance(), &instanceProperties));

            const auto runtimeName = fmt::format("{} {}.{}.{}",
                                                 instanceProperties.runtimeName,
                                                 XR_VERSION_MAJOR(instanceProperties.runtimeVersion),
                                                 XR_VERSION_MINOR(instanceProperties.runtimeVersion),
                                                 XR_VERSION_PATCH(instanceProperties.runtimeVersion));

            TraceLoggingWrite(g_traceProvider, "xrCreateInstance", TLArg(runtimeName.c_str(), "RuntimeName"));
            Log(fmt::format("Using OpenXR runtime: {}\n", runtimeName));

            return XR_SUCCESS;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetSystem
        XrResult xrGetSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) override 
        {
            if (getInfo->type != XR_TYPE_SYSTEM_GET_INFO) 
            {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetSystem",
                              TLXArg(instance, "Instance"),
                              TLArg(xr::ToCString(getInfo->formFactor), "FormFactor"));

            const XrResult result = OpenXrApi::xrGetSystem(instance, getInfo, systemId);

            if (XR_SUCCEEDED(result) && getInfo->formFactor == XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY) 
            {
                if (*systemId != m_systemId) 
                {

#if (ENABLE_SOCIAL_GAZES && !ENABLE_BOTH_PSVR2_LAYERS)
                    XrSystemEyeTrackingPropertiesFB social_eye_tracking_properties{XR_TYPE_SYSTEM_EYE_TRACKING_PROPERTIES_FB};

                    XrSystemProperties system_properties{XR_TYPE_SYSTEM_PROPERTIES};
                    system_properties.next = &social_eye_tracking_properties;

                    CHECK_XRCMD(OpenXrApi::xrGetSystemProperties(instance, *systemId, &system_properties));

                    TraceLoggingWrite(
                        g_traceProvider,
                        "xrGetSystem",
                        TLArg(system_properties.systemName, "SystemName"),
                        TLArg(!!social_eye_tracking_properties.supportsEyeTracking, "SupportsEyeTracking"));

                    std::string_view systemName(system_properties.systemName);
                    Log(fmt::format("Using OpenXR system: {}\n", systemName.data()));

                    m_trackerType = TrackerType::None;

#if ENABLE_PSVR2_SOCIAL_GAZES
                    if (!social_eye_tracking_properties.supportsEyeTracking && (systemName.find("SteamVR/OpenXR : playstation_vr2") != std::string::npos))
                    {
                        m_tracker = createPSVR2_Social_Gazes();
                        m_trackerType = m_tracker->getType();
                    }
#endif

#if ENABLE_DUMMY_SOCIAL_GAZES
                    if (!social_eye_tracking_properties.supportsEyeTracking && !m_tracker)
                    {
                        m_tracker = create_Dummy_Social_Gazes();
                        m_trackerType = m_tracker->getType();
                    }
#endif

#else

                    // Check if the system supports eye tracking.
                    XrSystemEyeGazeInteractionPropertiesEXT eyeGazeInteractionProperties{XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT};
                    XrSystemEyeTrackingPropertiesFB eyeTrackingProperties{XR_TYPE_SYSTEM_EYE_TRACKING_PROPERTIES_FB, &eyeGazeInteractionProperties};
                    XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
                    systemProperties.next = &eyeTrackingProperties;

                    CHECK_XRCMD(OpenXrApi::xrGetSystemProperties(instance, *systemId, &systemProperties));

                    TraceLoggingWrite(
                        g_traceProvider,
                        "xrGetSystem",
                        TLArg(systemProperties.systemName, "SystemName"),
                        TLArg(!!eyeGazeInteractionProperties.supportsEyeGazeInteraction, "SupportsEyeGazeInteraction"),
                        TLArg(!!eyeTrackingProperties.supportsEyeTracking, "SupportsEyeTracking"));

                    std::string_view systemName(systemProperties.systemName);
                    Log(fmt::format("Using OpenXR system: {}\n", systemName.data()));

                    m_trackerType = TrackerType::None;

#if ENABLE_BOTH_PSVR2_LAYERS
                    if(systemName.find("SteamVR/OpenXR : playstation_vr2") != std::string::npos)
                    {
                        m_tracker = createPSVR2_Tracker();
						m_trackerType = m_tracker->getType();
                    }
                    else
#elif ENABLE_DUMMY_EXT_COMBINED_GAZE
                    if (true)
                    {
                        m_tracker = create_Dummy_EXT_Combined_Gaze();
                        m_trackerType = m_tracker->getType();
                    } 
                    else
#elif ENABLE_PSVR2_EXT_COMBINED_GAZE
                    if (systemName.find("SteamVR/OpenXR : playstation_vr2") != std::string::npos) 
                    {
                        m_tracker = createPSVR2_OpenXR_EXT();
                        m_trackerType = m_tracker->getType();
                    } 
                    else
#endif
#if SUPPORT_WMR
                    if (eyeGazeInteractionProperties.supportsEyeGazeInteraction && systemName.find("Windows Mixed Reality") == std::string::npos) 
                    {
                        m_trackerType = TrackerType::EyeGazeInteraction;
                        Log(fmt::format(
                            "Upstream layer/runtime reported supportsEyeGazeInteraction, {} layer will be bypassed\n",
                            LayerName));
                    } 
#endif
#if SUPPORT_SIMULATED_EYE_TRACKING
                    else if (utilities::RegGetDword(HKEY_LOCAL_MACHINE, "SOFTWARE\\OpenXR-Eye-Trackers", "SimulateTracker").value_or(false)) 
                    {
                        // Configuration requested the mouse simulated eye tracking.
                        m_tracker = createSimulatedEyeTracker();
                    } 
#endif
#if SUPPORT_QUEST_PRO
                    else if (eyeTrackingProperties.supportsEyeTracking) 
                    {
                        // Quest Pro only supports "social eye tracking", which we can translate into eye gaze
                        // interaction.
                        m_tracker = createQuestProEyeTracker(*this);

                    } 
#endif

#if 0
                    else 
                    {
                        //else if (systemName.find("SteamVR/OpenXR : oculus") != std::string::npos) 
                        {
#if SUPPORT_VIRTUAL_DESKTOP
                            m_tracker = createVirtualDesktopEyeTracker();
#endif

#if SUPPORT_STEAMLINK
                            if (!m_tracker) 
                            {
                                m_tracker = createSteamLinkEyeTracker();
                            }
#endif
                        } 
                        else if (systemName.find("SteamVR/OpenXR") != std::string::npos) 
                        {
                        }
                    }
#endif

#endif // ENABLE_SOCIAL_GAZES

                    if (m_tracker) 
                    {
                        m_trackerType = m_tracker->getType();
                        Log(fmt::format("Using eye tracking: {}\n", getTrackerType(m_trackerType)));
                    }

                    TraceLoggingWrite(g_traceProvider, "xrGetSystem", TLArg((int)m_trackerType, "TrackerType"));

                    if (m_trackerType == TrackerType::None) 
                    {
                        Log("No supported eye tracking device found\n");
                    }
                }

                // Remember the XrSystemId to use.
                m_systemId = *systemId;
            }

            TraceLoggingWrite(g_traceProvider, "xrGetSystem", TLArg((int)*systemId, "SystemId"));

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetSystemProperties
        XrResult xrGetSystemProperties(XrInstance instance,
                                       XrSystemId systemId,
                                       XrSystemProperties* properties) override 
        {
            TraceLoggingWrite(g_traceProvider,
                              "xrGetSystemProperties",
                              TLXArg(instance, "Instance"),
                              TLArg((int)systemId, "SystemId"));

            const XrResult result = OpenXrApi::xrGetSystemProperties(instance, systemId, properties);

            if (XR_SUCCEEDED(result)) 
            {
                if (isSystemHandled(systemId) && !isPassthrough()) 
                {
#if ENABLE_SOCIAL_GAZES
                    XrSystemEyeTrackingPropertiesFB* social_eye_tracking_properties = reinterpret_cast<XrSystemEyeTrackingPropertiesFB*>(properties->next);

                    while (social_eye_tracking_properties) 
                    {
                        if (social_eye_tracking_properties->type == XR_TYPE_SYSTEM_EYE_TRACKING_PROPERTIES_FB) 
                        {
#if ENABLE_BOTH_PSVR2_LAYERS
                            social_eye_tracking_properties->supportsEyeTracking = (m_trackerType == TrackerType::PSVR2_TRACKERS) ? XR_TRUE : XR_FALSE;
#elif ENABLE_DUMMY_SOCIAL_GAZES
                            social_eye_tracking_properties->supportsEyeTracking = (m_trackerType == TrackerType::DUMMY_SOCIAL_GAZES) ? XR_TRUE : XR_FALSE;
#elif ENABLE_PSVR2_SOCIAL_GAZES
                            social_eye_tracking_properties->supportsEyeTracking = (m_trackerType == TrackerType::PSVR2_SOCIAL_GAZES) ? XR_TRUE : XR_FALSE;
#endif // ENABLE_BOTH_PSVR2_LAYERS

                            TraceLoggingWrite(g_traceProvider, "xrGetSystemProperties", TLArg(!!social_eye_tracking_properties->supportsEyeTracking,"supportsEyeTracking"));
                            break;
                        }
                        social_eye_tracking_properties = reinterpret_cast<XrSystemEyeTrackingPropertiesFB*>(social_eye_tracking_properties->next);
                    }

#endif // ENABLE_SOCIAL_GAZES

#if ENABLE_EXT_GAZE_INTERACTION

                    XrSystemEyeGazeInteractionPropertiesEXT* eyeGazeInteractionProperties = reinterpret_cast<XrSystemEyeGazeInteractionPropertiesEXT*>(properties->next);

                    while (eyeGazeInteractionProperties) 
                    {
                        if (eyeGazeInteractionProperties->type == XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT) 
                        {
#if ENABLE_BOTH_PSVR2_LAYERS
                            eyeGazeInteractionProperties->supportsEyeGazeInteraction = (m_trackerType == TrackerType::PSVR2_TRACKERS) ? XR_TRUE : XR_FALSE;
#else
                            eyeGazeInteractionProperties->supportsEyeGazeInteraction = (m_trackerType != TrackerType::None) ? XR_TRUE : XR_FALSE;
#endif

                            TraceLoggingWrite(g_traceProvider, "xrGetSystemProperties", TLArg(!!eyeGazeInteractionProperties->supportsEyeGazeInteraction, "SupportsEyeGazeInteraction"));
                            break;
                        }
                        eyeGazeInteractionProperties = reinterpret_cast<XrSystemEyeGazeInteractionPropertiesEXT*>(eyeGazeInteractionProperties->next);
                    }
#endif // ENABLE_EXT_GAZE_INTERACTION
                }
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateSession
        XrResult xrCreateSession(XrInstance instance,
                                 const XrSessionCreateInfo* createInfo,
                                 XrSession* session) override 
        {
            if (createInfo->type != XR_TYPE_SESSION_CREATE_INFO) 
            {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrCreateSession",
                              TLXArg(instance, "Instance"),
                              TLArg((int)createInfo->systemId, "SystemId"),
                              TLArg(createInfo->createFlags, "CreateFlags"));

            const XrResult result = OpenXrApi::xrCreateSession(instance, createInfo, session);

            if (XR_SUCCEEDED(result)) 
            {
                TraceLoggingWrite(g_traceProvider, "xrCreateSession", TLXArg(*session, "Session"));

                if (isSystemHandled(createInfo->systemId)) 
                {
                    m_session = *session;

                    if (m_tracker) 
                    {
                        m_tracker->start(m_session);
                    }
                    {
                        XrReferenceSpaceCreateInfo referenceSpaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
                        referenceSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
                        referenceSpaceInfo.poseInReferenceSpace = Pose::Identity();
                        CHECK_XRCMD(OpenXrApi::xrCreateReferenceSpace(m_session, &referenceSpaceInfo, &m_viewSpace));
                    }
                }
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroySession
        XrResult xrDestroySession(XrSession session) override 
        {
            TraceLoggingWrite(g_traceProvider, "xrDestroySession", TLXArg(session, "Session"));

            const XrResult result = OpenXrApi::xrDestroySession(session);

            if (XR_SUCCEEDED(result)) 
            {
                if (isSessionHandled(session)) 
                {
                    if (m_tracker) 
                    {
                        m_tracker->stop();
                    }

                    m_session = XR_NULL_HANDLE;
                }
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrSuggestInteractionProfileBindings
        XrResult xrSuggestInteractionProfileBindings(XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings) override 
        {
            if (suggestedBindings->type != XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING) 
            {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrSuggestInteractionProfileBindings",
                              TLXArg(instance, "Instance"),
                              TLArg(getXrPath(suggestedBindings->interactionProfile).c_str(), "InteractionProfile"));

            XrResult result = XR_ERROR_RUNTIME_FAILURE;
            const std::string& interactionProfile = getXrPath(suggestedBindings->interactionProfile);
            if (!isPassthrough() && interactionProfile == "/interaction_profiles/ext/eye_gaze_interaction") 
            {
                std::unique_lock lock(m_actionsAndSpacesMutex);

                result = XR_SUCCESS;
                for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
                    TraceLoggingWrite(
                        g_traceProvider,
                        "xrSuggestInteractionProfileBindings",
                        TLXArg(suggestedBindings->suggestedBindings[i].action, "Action"),
                        TLArg(getXrPath(suggestedBindings->suggestedBindings[i].binding).c_str(), "Path"));

                    const std::string& path = getXrPath(suggestedBindings->suggestedBindings[i].binding);
                    if (path == "/user/eyes_ext/input/gaze_ext/pose" || path == "/user/eyes_ext/input/gaze_ext") {
                        m_eyeGazeActions.insert(suggestedBindings->suggestedBindings[i].action);
                    } else {
                        result = XR_ERROR_PATH_UNSUPPORTED;
                    }
                }

                // We don't actually suggest the bindings, they would cause an error since the interaction profile is
                // not supported.
            } else {
                result = OpenXrApi::xrSuggestInteractionProfileBindings(instance, suggestedBindings);
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrCreateActionSpace
        XrResult xrCreateActionSpace(XrSession session,
                                     const XrActionSpaceCreateInfo* createInfo,
                                     XrSpace* space) override {
            if (createInfo->type != XR_TYPE_ACTION_SPACE_CREATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrCreateActionSpace",
                              TLXArg(session, "Session"),
                              TLXArg(createInfo->action, "Action"),
                              TLArg(getXrPath(createInfo->subactionPath).c_str(), "SubactionPath"),
                              TLArg(xr::ToString(createInfo->poseInActionSpace).c_str(), "PoseInActionSpace"));

            const XrResult result = OpenXrApi::xrCreateActionSpace(session, createInfo, space);

            if (XR_SUCCEEDED(result)) {
                TraceLoggingWrite(g_traceProvider, "xrCreateActionSpace", TLXArg(*space, "Space"));

                if (isSessionHandled(session) && !isPassthrough()) {
                    std::unique_lock lock(m_actionsAndSpacesMutex);

                    ActionSpace actionSpace{};
                    actionSpace.action = createInfo->action;
                    actionSpace.pose = createInfo->poseInActionSpace;
                    m_actionSpaces.insert_or_assign(*space, actionSpace);
                }
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrDestroySpace
        XrResult xrDestroySpace(XrSpace space) override {
            TraceLoggingWrite(g_traceProvider, "xrDestroySpace", TLXArg(space, "Space"));

            std::unique_lock lock(m_actionsAndSpacesMutex);

            const XrResult result = OpenXrApi::xrDestroySpace(space);

            if (XR_SUCCEEDED(result)) {
                m_actionSpaces.erase(space);
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrWaitFrame
        XrResult xrWaitFrame(XrSession session,
                             const XrFrameWaitInfo* frameWaitInfo,
                             XrFrameState* frameState) override {
            TraceLoggingWrite(g_traceProvider, "xrWaitFrame", TLXArg(session, "Session"));

            const XrResult result = OpenXrApi::xrWaitFrame(session, frameWaitInfo, frameState);

            if (XR_SUCCEEDED(result)) {
                TraceLoggingWrite(g_traceProvider,
                                  "xrWaitFrame",
                                  TLArg(!!frameState->shouldRender, "ShouldRender"),
                                  TLArg(frameState->predictedDisplayTime, "PredictedDisplayTime"),
                                  TLArg(frameState->predictedDisplayPeriod, "PredictedDisplayPeriod"));

                if (isSessionHandled(session)) {
                    m_lastFrameWaitedTime = frameState->predictedDisplayTime;
                }
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrBeginFrame
        XrResult xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) override {
            TraceLoggingWrite(g_traceProvider, "xrBeginFrame", TLXArg(session, "Session"));

            const XrResult result = OpenXrApi::xrBeginFrame(session, frameBeginInfo);

            if (XR_SUCCEEDED(result) && isSessionHandled(session)) {
                m_lastFrameBegunTime = m_lastFrameWaitedTime;
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrLocateSpace
        XrResult xrLocateSpace(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location) override {
            if (location->type != XR_TYPE_SPACE_LOCATION) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrLocateSpace",
                              TLXArg(space, "Space"),
                              TLXArg(baseSpace, "BaseSpace"),
                              TLArg(time, "Time"));

            std::unique_lock lock(m_actionsAndSpacesMutex);

            XrPosef queryPoseOffset;
            bool isQueryEyeGaze = false;
            {
                auto it = m_actionSpaces.find(space);
                if (it != m_actionSpaces.end()) {
                    if (!it->second.isEyeGaze) {
                        it->second.isEyeGaze = !!m_eyeGazeActions.count(it->second.action);
                    }
                    isQueryEyeGaze = it->second.isEyeGaze.value();
                    queryPoseOffset = it->second.pose;
                }
            }

            XrPosef basePoseOffset;
            bool isBaseEyeGaze = false;
            {
                auto it = m_actionSpaces.find(baseSpace);
                if (it != m_actionSpaces.end()) {
                    if (!it->second.isEyeGaze) {
                        it->second.isEyeGaze = !!m_eyeGazeActions.count(it->second.action);
                    }
                    isBaseEyeGaze = it->second.isEyeGaze.value();
                    basePoseOffset = it->second.pose;
                }
            }

            XrResult result = XR_ERROR_RUNTIME_FAILURE;
            if (isQueryEyeGaze || isBaseEyeGaze) {
                assert(!isPassthrough());
                // TODO: Support the notion of (in)active actionsets and actionset priority.
                if (isQueryEyeGaze && isBaseEyeGaze) {
                    location->pose = Pose::Multiply(queryPoseOffset, Pose::Invert(basePoseOffset));
                    location->locationFlags =
                        XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT |
                        XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
                    result = XR_SUCCESS;
                } else {
                    location->locationFlags = 0;
                    XrVector3f gazeUnitVector;

                    int eye = INVALID_INDEX;
                    bool is_open = true;

                    if (getEyeGaze(time, false, eye, gazeUnitVector, is_open)) {
                        XrSpaceLocation viewToSpace{XR_TYPE_SPACE_LOCATION};
                        result = OpenXrApi::xrLocateSpace(
                            m_viewSpace, isQueryEyeGaze ? baseSpace : space, time, &viewToSpace);
                        TraceLoggingWrite(
                            g_traceProvider, "xrLocateSpace_LocateViewSpace", TLArg(xr::ToCString(result), "Result"));
                        if (XR_SUCCEEDED(result) && Pose::IsPoseValid(viewToSpace.locationFlags)) {
                            const XrPosef eyeGazeToView = Pose::MakePose(
                                Quaternion::RotationRollPitchYaw({tan(gazeUnitVector.y), -tan(gazeUnitVector.x), 0.f}),
                                XrVector3f{0, 0, 0});

                            location->pose = Pose::Multiply(
                                Pose::Multiply(eyeGazeToView, isQueryEyeGaze ? queryPoseOffset : basePoseOffset),
                                viewToSpace.pose);
                            if (isBaseEyeGaze) {
                                location->pose = Pose::Invert(location->pose);
                            }

                            location->locationFlags = viewToSpace.locationFlags;

                            // Handle the sample time struct if needed.
                            XrEyeGazeSampleTimeEXT* gazeSampleTime =
                                reinterpret_cast<XrEyeGazeSampleTimeEXT*>(location->next);
                            while (gazeSampleTime) {
                                if (gazeSampleTime->type == XR_TYPE_EYE_GAZE_SAMPLE_TIME_EXT) {
                                    // TODO: Handle sample time for trackers that support it.
                                    gazeSampleTime->time = time;
                                    break;
                                }
                                gazeSampleTime = reinterpret_cast<XrEyeGazeSampleTimeEXT*>(gazeSampleTime->next);
                            }
                        } else if (result == XR_ERROR_TIME_INVALID) {
                            // Workaround for DCS loading screen, be tolerant to gaps in XrTime. We still return a
                            // non-locatable location.
                            result = XR_SUCCESS;
                        }
                    } else {
                        result = XR_SUCCESS;
                    }
                }
            } else {
                result = OpenXrApi::xrLocateSpace(space, baseSpace, time, location);
            }

            if (XR_SUCCEEDED(result)) {
                TraceLoggingWrite(g_traceProvider,
                                  "xrLocateSpace",
                                  TLArg(location->locationFlags, "LocationFlags"),
                                  TLArg(xr::ToString(location->pose).c_str(), "Pose"));
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetActionStatePose
        XrResult xrGetActionStatePose(XrSession session,
                                      const XrActionStateGetInfo* getInfo,
                                      XrActionStatePose* state) override {
            if (getInfo->type != XR_TYPE_ACTION_STATE_GET_INFO || state->type != XR_TYPE_ACTION_STATE_POSE) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetActionStatePose",
                              TLXArg(session, "Session"),
                              TLXArg(getInfo->action, "Action"),
                              TLArg(getXrPath(getInfo->subactionPath).c_str(), "SubactionPath"));

            std::unique_lock lock(m_actionsAndSpacesMutex);

            XrResult result = XR_ERROR_RUNTIME_FAILURE;
            if (isSessionHandled(session) && !isPassthrough() && m_eyeGazeActions.count(getInfo->action)) {
                // TODO: Support the notion of (in)active actionsets and actionset priority.
                XrVector3f dummy{};

                int eye = INVALID_INDEX;
                bool is_open = true;

                state->isActive = getEyeGaze(m_lastFrameBegunTime, true, eye, dummy, is_open) ? XR_TRUE : XR_FALSE;
                result = XR_SUCCESS;
            } else {
                result = OpenXrApi::xrGetActionStatePose(session, getInfo, state);
            }

            if (XR_SUCCEEDED(result)) {
                TraceLoggingWrite(g_traceProvider, "xrGetActionStatePose", TLArg(!!state->isActive, "Active"));
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetCurrentInteractionProfile
        XrResult xrGetCurrentInteractionProfile(XrSession session,
                                                XrPath topLevelUserPath,
                                                XrInteractionProfileState* interactionProfile) override {
            if (interactionProfile->type != XR_TYPE_INTERACTION_PROFILE_STATE) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetCurrentInteractionProfile",
                              TLXArg(session, "Session"),
                              TLArg(getXrPath(topLevelUserPath).c_str(), "TopLevelUserPath"));

            XrResult result = XR_ERROR_RUNTIME_FAILURE;
            if (isSessionHandled(session) && !isPassthrough() && getXrPath(topLevelUserPath) == "/user/eyes_ext") {
                CHECK_XRCMD(OpenXrApi::xrStringToPath(GetXrInstance(),
                                                      "/interaction_profiles/ext/eye_gaze_interaction",
                                                      &interactionProfile->interactionProfile));
                result = XR_SUCCESS;
            } else {
                result = OpenXrApi::xrGetCurrentInteractionProfile(session, topLevelUserPath, interactionProfile);
            }

            if (XR_SUCCEEDED(result)) {
                TraceLoggingWrite(
                    g_traceProvider,
                    "xrGetCurrentInteractionProfile",
                    TLArg(getXrPath(interactionProfile->interactionProfile).c_str(), "InteractionProfile"));
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrEnumerateBoundSourcesForAction
        XrResult xrEnumerateBoundSourcesForAction(XrSession session,
                                                  const XrBoundSourcesForActionEnumerateInfo* enumerateInfo,
                                                  uint32_t sourceCapacityInput,
                                                  uint32_t* sourceCountOutput,
                                                  XrPath* sources) override {
            if (enumerateInfo->type != XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrEnumerateBoundSourcesForAction",
                              TLXArg(session, "Session"),
                              TLXArg(enumerateInfo->action, "Action"),
                              TLArg(sourceCapacityInput, "SourceCapacityInput"));

            std::unique_lock lock(m_actionsAndSpacesMutex);

            XrResult result = XR_ERROR_RUNTIME_FAILURE;
            if (isSessionHandled(session) && !isPassthrough() && m_eyeGazeActions.count(enumerateInfo->action)) {
                // TODO: Support the notion of (in)active actionsets and actionset priority.
                *sourceCountOutput = 1;
                result = XR_SUCCESS;

                if (sourceCapacityInput) {
                    CHECK_XRCMD(xrStringToPath(GetXrInstance(), "/user/eyes_ext/input/gaze_ext/pose", &sources[0]));
                }
            } else {
                result = OpenXrApi::xrEnumerateBoundSourcesForAction(
                    session, enumerateInfo, sourceCapacityInput, sourceCountOutput, sources);
            }

            if (XR_SUCCEEDED(result)) {
                TraceLoggingWrite(g_traceProvider,
                                  "xrEnumerateBoundSourcesForAction",
                                  TLArg(*sourceCountOutput, "SourceCountOutput"));

                if (sourceCapacityInput) {
                    for (uint32_t i = 0; i < *sourceCountOutput; i++) {
                        TraceLoggingWrite(g_traceProvider,
                                          "xrEnumerateBoundSourcesForAction",
                                          TLArg((uint64_t)sources[i], "Source"),
                                          TLArg(getXrPath(sources[i]).c_str(), "Path"));
                    }
                }
            }

            return result;
        }

        // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#xrGetInputSourceLocalizedName
        XrResult xrGetInputSourceLocalizedName(XrSession session,
                                               const XrInputSourceLocalizedNameGetInfo* getInfo,
                                               uint32_t bufferCapacityInput,
                                               uint32_t* bufferCountOutput,
                                               char* buffer) override {
            if (getInfo->type != XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO) {
                return XR_ERROR_VALIDATION_FAILURE;
            }

            TraceLoggingWrite(g_traceProvider,
                              "xrGetInputSourceLocalizedName",
                              TLXArg(session, "Session"),
                              TLArg(getXrPath(getInfo->sourcePath).c_str(), "SourcePath"),
                              TLArg(getInfo->whichComponents, "WhichComponents"));

            XrResult result = XR_ERROR_RUNTIME_FAILURE;
            const std::string& sourcePath = getXrPath(getInfo->sourcePath);
            if (isSessionHandled(session) && !isPassthrough() && sourcePath == "/user/eyes_ext/input/gaze_ext/pose") {
                std::string localizedName;
                if ((getInfo->whichComponents & (XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
                                                 XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT)) ==
                    (XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
                     XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT)) {
                    localizedName += "Eye Gaze Interaction Eye Tracker";
                } else if ((getInfo->whichComponents & XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT)) {
                    localizedName += "Eye Gaze Interaction";
                } else if ((getInfo->whichComponents & XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT)) {
                    localizedName += "Eye Tracker";
                }

                *bufferCountOutput = (uint32_t)localizedName.length();
                result = XR_SUCCESS;

                if (bufferCapacityInput && bufferCapacityInput >= *bufferCountOutput) {
                    sprintf_s(buffer, bufferCapacityInput, "%s", localizedName.c_str());
                } else {
                    result = XR_ERROR_SIZE_INSUFFICIENT;
                }
            } else {
                result = OpenXrApi::xrGetInputSourceLocalizedName(
                    session, getInfo, bufferCapacityInput, bufferCountOutput, buffer);
            }

            if (XR_SUCCEEDED(result)) {
                TraceLoggingWrite(
                    g_traceProvider, "xrGetInputSourceLocalizedName", TLArg(*bufferCountOutput, "BufferCountOutput"));

                if (bufferCapacityInput) {
                    TraceLoggingWrite(g_traceProvider, "xrGetInputSourceLocalizedName", TLArg(buffer, "String"));
                }
            }

            return result;
        }

#if ENABLE_SOCIAL_GAZES
        XrResult xrCreateEyeTrackerFB(XrSession session, const XrEyeTrackerCreateInfoFB* createInfo, XrEyeTrackerFB* eyeTracker) override 
        {
            *eyeTracker = *(XrEyeTrackerFB*)this;
            return XR_SUCCESS;
        }

        XrResult xrDestroyEyeTrackerFB(XrEyeTrackerFB eyeTracker) override 
        {
            return XR_SUCCESS;
        }

        XrResult xrGetEyeGazesFB(XrEyeTrackerFB eyeTracker, const XrEyeGazesInfoFB* gazeInfo, XrEyeGazesFB* eyeGazes) override 
        {
            if (eyeTracker && gazeInfo && eyeGazes)
            {
                const bool getStateOnly = false;

                XrEyeGazesFB& gazes = *eyeGazes;

                XrEyeGazeFB& left_gaze = gazes.gaze[XR_EYE_POSITION_LEFT_FB];
                XrEyeGazeFB& right_gaze = gazes.gaze[XR_EYE_POSITION_RIGHT_FB];

                bool left_eye_open = true;
                bool right_eye_open = true;

                XrVector3f left_gaze_direction  = {0.0f, 0.0f, -1.0f};
                XrVector3f right_gaze_direction = {0.0f, 0.0f, -1.0f};

                const bool left_gaze_ok = getEyeGaze(gazeInfo->time, getStateOnly, BVR::LEFT, left_gaze_direction, left_eye_open);
                const bool right_gaze_ok = getEyeGaze(gazeInfo->time, getStateOnly, BVR::RIGHT, right_gaze_direction, right_eye_open);

                const float left_angle_rad_X = atan2f(-left_gaze_direction.x, -left_gaze_direction.z);
                const float left_angle_rad_Y = atan2f(left_gaze_direction.y, -left_gaze_direction.z);

                XrMatrix4x4f left_gaze_matrix = {};
                XrMatrix4x4f_CreateRotationRadians(&left_gaze_matrix, left_angle_rad_Y, left_angle_rad_X, 0.0f);
                XrMatrix4x4f_GetRotation(&left_gaze.gazePose.orientation, &left_gaze_matrix);
                XrQuaternionf_Normalize(&left_gaze.gazePose.orientation);

                left_gaze.gazePose.position = {0.0f, 0.0f, 0.0f};
                left_gaze.isValid = left_gaze_ok&& left_eye_open;
                left_gaze.gazeConfidence = left_eye_open ? 1.0f : 0.0f; // For now. add lerp later

                const float right_angle_rad_X = atan2f(-right_gaze_direction.x, -right_gaze_direction.z);
                const float right_angle_rad_Y = atan2f(right_gaze_direction.y, -right_gaze_direction.z);

                XrMatrix4x4f right_gaze_matrix = {};
                XrMatrix4x4f_CreateRotationRadians(&right_gaze_matrix, right_angle_rad_Y, right_angle_rad_X, 0.0f);
                XrMatrix4x4f_GetRotation(&right_gaze.gazePose.orientation, &right_gaze_matrix);
                XrQuaternionf_Normalize(&right_gaze.gazePose.orientation);

                right_gaze.gazePose.position = {0.0f, 0.0f, 0.0f};
                right_gaze.isValid = right_gaze_ok&& right_eye_open;
                right_gaze.gazeConfidence = right_eye_open ? 1.0f : 0.0f; // For now. add lerp later

                return XR_SUCCESS;
            }

            return XR_ERROR_FEATURE_UNSUPPORTED;
        }
#endif // ENABLE_SOCIAL_GAZES

      private:
        bool getEyeGaze(XrTime time, bool getStateOnly, int eye, XrVector3f& unitVector, bool& is_open) {
            bool result = false;
            
            switch (m_trackerType) {
            default:
                if (m_tracker) {
                    if (!getStateOnly) {
                        result = m_tracker->getGaze(time, eye, unitVector, is_open);
                    } else {
                        result = m_tracker->isGazeAvailable(time, eye);
                    }
                }
                break;

            case TrackerType::None:
                break;
            }

            TraceLoggingWrite(g_traceProvider,
                              "EyeGaze",
                              TLArg(result, "Valid"),
                              TLArg(xr::ToString(unitVector).c_str(), "GazeUnitVector"));

            return result;
        }

        const std::string getXrPath(XrPath path) {
            if (path == XR_NULL_PATH) {
                return "";
            }

            char buf[XR_MAX_PATH_LENGTH];
            uint32_t count;
            CHECK_XRCMD(OpenXrApi::xrPathToString(GetXrInstance(), path, sizeof(buf), &count, buf));
            std::string str;
            str.assign(buf, count - 1);
            return str;
        }

        bool isSystemHandled(XrSystemId systemId) const {
            return systemId == m_systemId;
        }

        bool isSessionHandled(XrSession session) const {
            return session == m_session;
        }

        bool isPassthrough() const {
            return m_trackerType == TrackerType::EyeGazeInteraction;
        }

        struct ActionSpace {
            XrAction action;
            XrPosef pose;

            std::optional<bool> isEyeGaze;
        };

        bool m_bypassApiLayer{false};
        XrSystemId m_systemId{XR_NULL_SYSTEM_ID};
        XrSession m_session{XR_NULL_HANDLE};
        XrSpace m_viewSpace{XR_NULL_HANDLE};
        std::unique_ptr<IEyeTracker> m_tracker{};
        TrackerType m_trackerType{TrackerType::None};

        XrTime m_lastFrameBegunTime{};
        XrTime m_lastFrameWaitedTime{};

        std::mutex m_actionsAndSpacesMutex;
        std::unordered_set<XrAction> m_eyeGazeActions;
        std::unordered_map<XrSpace, ActionSpace> m_actionSpaces;
    };

    // This method is required by the framework to instantiate your OpenXrApi implementation.
    OpenXrApi* GetInstance() {
        if (!g_instance) {
            g_instance = std::make_unique<OpenXrLayer>();
        }
        return g_instance.get();
    }

} // namespace openxr_api_layer

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DetourRestoreAfterWith();
        TraceLoggingRegister(openxr_api_layer::log::g_traceProvider);
        break;

    case DLL_PROCESS_DETACH:
        TraceLoggingUnregister(openxr_api_layer::log::g_traceProvider);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
