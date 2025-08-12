# PSVR2_OpenXR_Eye_Tracking

OpenXR-based Eye-tracking for PSVR 2 on PC / SteamVR. 

NOTE: THIS REPO HAS NO 3RD PARTY PRIVATE UNRELEASED CODE or BINARIES IN IT.

These API layers rely on a THIRD PARTY closed-source and as-yet unreleased DLL (do NOT ask me for it).

Calibration isn't finished yet, it will be done in a separate app.

To code this, I modified the framework provided by mbucchia here:

https://github.com/mbucchia/_ARCHIVE_OpenXR-Eye-Trackers

OPTION 1: PSVR 2 - UNIFIED EXT + SOCIAL GAZES in a single API layer:

<img width="804" height="620" alt="image" src="https://github.com/user-attachments/assets/c6a875ab-3c68-43d1-a873-ce4198f704ca" />

OPTION 2: PSVR 2 - EXT Gaze Interaction ONLY:

<img width="602" height="464" alt="EXT_Installed_Active" src="https://github.com/user-attachments/assets/62f3eb5a-beae-45a5-b02f-9f3a3a86a8e5" />

https://github.com/user-attachments/assets/4302812f-49ba-4d5c-9a02-86de3c717b3b

OPTION 3: PSVR 2 - FB/Meta Social Gazes (dual independent eye support) ONLY (uses Quest Pro method):

<img width="598" height="465" alt="Social_Installed_Active" src="https://github.com/user-attachments/assets/9ea9cb40-c965-4b97-9625-e06792b9ba31" />

https://github.com/user-attachments/assets/99f63767-f74a-4ce7-b07c-db22997a0045

MIT license.

WARNING: USE AT YOUR OWN RISK!!! No Warranty is provided! 


BUILD INSTRUCTIONS:

To make these API layers work:

This client code or server (DLL) have to be compiled to use the same named pipe string for direct IPC communication of the gazes and handshake, 

#define PSVR2_SERVER_NAMED_PIPE_NAME "\\\\.\\pipe\\PlaystationVR2ServerPipe"

I also reduced the data sent over IPC to avoid transferring junk / garbage that these OpenXR API layers can't export or use anyway.

	struct XRGazeState
	{
		XrVector3f direction_ = { 0.0f, 0.0f, -1.0f };
		bool is_valid_ = false;
	};

	struct AllXRGazeStates
	{
		XRGazeState combined_gaze_;
		XRGazeState per_eye_gazes_[BVR::NUM_EYES];
	};

AllXRGazeStates is the data type that should be sent over IPC. I also did not use a thread to copy the data out from IPC on the client, it is fast enough I think to do it synchronously as I have.
 
