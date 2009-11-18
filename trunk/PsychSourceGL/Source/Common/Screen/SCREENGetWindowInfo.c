/*
  SCREENGetWindowInfo.c		
  
  AUTHORS:

  mario.kleiner at tuebingen.mpg.de  mk
  
  PLATFORMS:	All

  HISTORY:
  06/03/07  mk		Created.
 
  DESCRIPTION:
  
  Returns all kind of misc information about a specific window in a struct.
  This is a catch-all for information that's not of too much interest for regular
  users, but useful for Psychtoolbox helper functions (M-Files).
  
  NOTES:

  Be careful with length of struct field names! Only names up to 31 characters are
  supported by Matlab 5.x (and maybe 6.x -- untested). Larger names cause matching
  failure!

*/

#include "Screen.h"

#if PSYCH_SYSTEM == PSYCH_OSX

/* This function defines of Apple undocumented functions are taken from CGSDebug.h, a
 * file provided by Alacatia Labs. This is the copyright info associated with it:
 *
 * Routines for debugging the window server and application drawing.
 *
 * Copyright (C) 2007-2008 Alacatia Labs
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 
 * Joe Ranieri joe@alacatia.com
 *
 *
 */
 
typedef enum {
	/*! Clears all flags. */
	kCGSDebugOptionNone = 0,
	
	/*! All screen updates are flashed in yellow. Regions under a DisableUpdate are flashed in orange. Regions that are hardware accellerated are painted green. */
	kCGSDebugOptionFlashScreenUpdates = 0x4,
	
	/*! Colors windows green if they are accellerated, otherwise red. Doesn't cause things to refresh properly - leaves excess rects cluttering the screen. */
	kCGSDebugOptionColorByAccelleration = 0x20,
	
	/*! Disables shadows on all windows. */
	kCGSDebugOptionNoShadows = 0x4000,
	
	/*! Setting this disables the pause after a flash when using FlashScreenUpdates or FlashIdenticalUpdates. */
	kCGSDebugOptionNoDelayAfterFlash = 0x20000,
	
	/*! Flushes the contents to the screen after every drawing operation. */
	kCGSDebugOptionAutoflushDrawing = 0x40000,
	
	/*! Highlights mouse tracking areas. Doesn't cause things to refresh correctly - leaves excess rectangles cluttering the screen. */
	kCGSDebugOptionShowMouseTrackingAreas = 0x100000,
	
	/*! Flashes identical updates in red. */
	kCGSDebugOptionFlashIdenticalUpdates = 0x4000000,
	
	/*! Dumps a list of windows to /tmp/WindowServer.winfo.out. This is what Quartz Debug uses to get the window list. */
	kCGSDebugOptionDumpWindowListToFile = 0x80000001,
	
	/*! Dumps a list of connections to /tmp/WindowServer.cinfo.out. */
	kCGSDebugOptionDumpConnectionListToFile = 0x80000002,
	
	/*! Dumps a very verbose debug log of the WindowServer to /tmp/CGLog_WinServer_<PID>. */
	kCGSDebugOptionVerboseLogging = 0x80000006,

	/*! Dumps a very verbose debug log of all processes to /tmp/CGLog_<NAME>_<PID>. */
	kCGSDebugOptionVerboseLoggingAllApps = 0x80000007,
	
	/*! Dumps a list of hotkeys to /tmp/WindowServer.keyinfo.out. */
	kCGSDebugOptionDumpHotKeyListToFile = 0x8000000E,
	
	/*! Dumps information about OpenGL extensions, etc to /tmp/WindowServer.glinfo.out. */
	kCGSDebugOptionDumpOpenGLInfoToFile = 0x80000013,
	
	/*! Dumps a list of shadows to /tmp/WindowServer.shinfo.out. */
	kCGSDebugOptionDumpShadowListToFile = 0x80000014,
	
	/*! Leopard: Dumps information about caches to `/tmp/WindowServer.scinfo.out`. */
	kCGSDebugOptionDumpCacheInformationToFile = 0x80000015,
	
	/*! Leopard: Purges some sort of cache - most likely the same caches dummped with `kCGSDebugOptionDumpCacheInformationToFile`. */
	kCGSDebugOptionPurgeCaches = 0x80000016,
	
	/*! Leopard: Dumps a list of windows to `/tmp/WindowServer.winfo.plist`. This is what Quartz Debug on 10.5 uses to get the window list. */
	kCGSDebugOptionDumpWindowListToPlist = 0x80000017,

	/*! Leopard: DOCUMENTATION PENDING */
	kCGSDebugOptionEnableSurfacePurging = 0x8000001B,

	// Leopard: 0x8000001C - invalid
	
	/*! Leopard: DOCUMENTATION PENDING */
	kCGSDebugOptionDisableSurfacePurging = 0x8000001D,
	
	/*! Leopard: Dumps information about an application's resource usage to `/tmp/CGResources_<NAME>_<PID>`. */
	kCGSDebugOptionDumpResourceUsageToFiles = 0x80000020,
	
	// Leopard: 0x80000022 - something about QuartzGL?
	
	// Leopard: Returns the magic mirror to its normal mode. The magic mirror is what the Dock uses to draw the screen reflection. For more information, see `CGSSetMagicMirror`. */
	kCGSDebugOptionSetMagicMirrorModeNormal = 0x80000023,
	
	/*! Leopard: Disables the magic mirror. It still appears but draws black instead of a reflection. */
	kCGSDebugOptionSetMagicMirrorModeDisabled = 0x80000024,
} CGSDebugOption;
 
typedef int CGSConnectionID;
extern CGSConnectionID _CGSDefaultConnection(void);
extern CGError CGSGetPerformanceData(CGSConnectionID cid, float *outFPS, float *unk, float *unk2, float *unk3);
extern CGError CGSSetDebugOptions(int options);
extern int CGSGetDebugOptions(int *outCurrentOptions);

#endif

static char useString[] = "info = Screen('GetWindowInfo', windowPtr [, infoType=0] [, auxArg1]);";
static char synopsisString[] = 
	"Returns a struct with miscellaneous info for the specified onscreen window."
	"\"windowPtr\" is the handle of the onscreen window for which info should be returned. "
	"\"infoType\" If left out or set to zero, all available information for the 'windowPtr' is returned. "
	"If set to 1, only the rasterbeam position is returned (or -1 if unsupported).\n"
	"If set to 2, information about the window server is returned (or -1 if unsupported).\n"
	"If set to 3, low-level window server settings are changed according to 'auxArg1'. Do *not* use, "
	"unless you really know what you're doing and have read the relevant PTB source code!\n"
    "If set to 4, returns a single value with the current activity status of asynchronous flips.\n"
	"If set to 5, will start measurement of GPU time for render operations. The clock will start "
	"on the next drawing command after this call. The clock will stop at next bufferswap with call "
	"to Screen('Flip', ..); After the flip, the elapsed rendertime will be returned in the 'GPULastFrameRenderTime' "
	"field of the struct that you get when calling with infoType=0. Not all GPU's support this function. If the "
	"function is unsupported, a value of zero will be returned in the info struct.\n"
	"1 if a Screen('AsyncFlipBegin') was called and the flip is still active, ie., hasn't "
	"been finished with a matching Screen('AsyncFlipEnd') or Screen('AsyncFlipCheckEnd');, 0 otherwise."
    "You can call this function with an infoType of zero only if no async flips are active. This is why "
    "you need to use the special infoType 4 to find out if async flips are active.\n"
	"The info struct contains all kinds of information. Just check its output to see what "
	"is returned. Most of this info is not interesting for normal users, mostly provided "
	"for internal use by M-Files belonging to Psychtoolbox itself, e.g., display tests.\n\n"
	"The info struct contains the following fields:\n"
	"----------------------------------------------\n\n"
	"BeamPosition: Current rasterbeam position of the video scanout cycle.\n"
	"LastVBLTimeOfFlip: VBL timestamp of last finished Screen('Flip') operation.\n"
	"TimeAtSwapRequest: Timestamp taken prior to submission of the low-level swap command. Useful for micro-benchmarking.\n"
	"TimePostSwapRequest: Timestamp taken after submission of the low-level swap command. Useful for micro-benchmarking.\n"
	"GPULastFrameRenderTime: Duration of all rendering operations in the last frame, as measured by GPU, if infoType 5 was used.\n"
	"RawSwapTimeOfFlip: Raw (uncorrected by high-precision timestamping) timestamp of last finished Screen('Flip') operation.\n"
	"LastVBLTime: System time when last vertical blank happened, or the same as "
	"LastVBLTimeOfFlip if the system doesn't support queries of this property (currently only OS/X does.)\n"
	"VBLCount: Running count of vertical blank intervals since (graphics)system startup. Or zero if not"
	"supported by system. Currently only OS/X and Linux do support this with some GPU's.\n"
	"VideoRefreshFromBeamposition: Estimate of video refresh cycle from beamposition measurement method.\n"
	"GLVendor, GLRenderer, GLVersion: Vendor name, renderer name and version of the OpenGL implementation.\n"
	"StereoMode: Currently selected stereomode, as requested in call to Screen('OpenWindow', ...);\n"
	"StereoDrawBuffer: Current drawbuffer for stereo display (0 = left eye, 1 = right eye, 2 = None in mono mode).\n"
	"ImagingMode: Currently selected imging pipeline mode, as requested in call to Screen('OpenWindow', ...);\n"
	"MultiSampling: Currently selected multisample anti-aliasing mode, as requested in call to Screen('OpenWindow', ...);\n"
	"MissedDeadlines: Number of missed Screen('Flip') stimulus onset deadlines, according to internal skip detector.\n"
	"GuesstimatedMemoryUsageMB: Estimated memory usage of window or texture in Megabytes. Can be very inaccurate or unavailable!\n"
	"VBLStartLine, VBLEndline: Start/Endline of vertical blanking interval. The VBLEndline value is not available/valid on all GPU's.\n"
	"\n"
	"The following settings are derived from a builtin detection heuristic, which works on most common GPU's:\n\n"
	"GPUCoreId: Symbolic name string that roughly describes the name of the GPU core of the graphics card. This string is arbitrarily\n"
	"chosen to roughly group the cores by common capabilities (and quirks). Currently defined are:\n"
	"R100 = Very old ATI GPUs, R300 = GPU's roughly starting at Radeon 9000, R500 = Radeon X1000 or later, R600 = Radeon HD2000 or later.\n"
	"NV10 = Very old NVidia GPUs, NV30 = NV30 or later, NV40 = Geforce6000/7000 or later, G80 = Geforce8000 or later.\n"
	"An empty GPUCoreId string means a different, unspecified core.\n\n"
	"BitsPerColorComponent: Effective color depths of window/framebuffer in bits per color channel component (bpc).\n"
	"GLSupportsFBOUpToBpc: 0 = No support for framebuffer objects. Otherwise maximum supported bpc (8, 16, 32).\n"
	"GLSupportsBlendingUpToBpc: Maximum supported bpc for hardware accelerated framebuffer blending (alpha blending).\n"
	"GLSupportsTexturesUpToBpc: Maximum supported bpc for textures (8, 16, 32).\n"
	"GLSupportsFilteringUpToBpc: Maximum supported bpc for hardware accelerated linear filtering of textures (8, 16, 32).\n"
	"GLSupportsPrecisionColors: 1 = Hardware can be fully trusted to rasterize perfect 32 bpc colors in floating point color mode "
	"without special support of PTB. 0 = Needs special (slower) support from PTB to work.\n"
	"GLSupportsFP32Shading: 1 = All internal caculations of the GPU are done with IEEE single precision 32 bit floating point, i.e., "
	"very accurate.\n\n";

static char seeAlsoString[] = "OpenWindow, Flip, NominalFrameRate";
	 
PsychError SCREENGetWindowInfo(void) 
{
    const char *FieldNames[]={ "Beamposition", "LastVBLTimeOfFlip", "LastVBLTime", "VBLCount", "TimeAtSwapRequest", "TimePostSwapRequest", "RawSwapTimeOfFlip",
							   "GPULastFrameRenderTime", "StereoMode", "ImagingMode", "MultiSampling", "MissedDeadlines", "StereoDrawBuffer",
							   "GuesstimatedMemoryUsageMB", "VBLStartline", "VBLEndline", "VideoRefreshFromBeamposition", "GLVendor", "GLRenderer", "GLVersion", "GPUCoreId", 
							   "GLSupportsFBOUpToBpc", "GLSupportsBlendingUpToBpc", "GLSupportsTexturesUpToBpc", "GLSupportsFilteringUpToBpc", "GLSupportsPrecisionColors",
							   "GLSupportsFP32Shading", "BitsPerColorComponent", "IsFullscreen", "SpecialFlags" };
							   
	const int  fieldCount = 30;
	PsychGenericScriptType	*s;

    PsychWindowRecordType *windowRecord;
    double beamposition, lastvbl;
	int infoType = 0, retIntArg;
	double auxArg1, auxArg2, auxArg3;
	CGDirectDisplayID displayId;
	psych_uint64 postflip_vblcount;
	double vbl_startline;
	long scw, sch;
	psych_bool onscreen;
    
    //all subfunctions should have these two lines.  
    PsychPushHelp(useString, synopsisString, seeAlsoString);
    if(PsychIsGiveHelp()){PsychGiveHelp();return(PsychError_none);};
    
    PsychErrorExit(PsychCapNumInputArgs(5));     //The maximum number of inputs
    PsychErrorExit(PsychRequireNumInputArgs(1)); //The required number of inputs	
    PsychErrorExit(PsychCapNumOutputArgs(1));    //The maximum number of outputs

    // Query infoType flag: Defaults to zero.
    PsychCopyInIntegerArg(2, FALSE, &infoType);
	if (infoType < 0 || infoType > 5) PsychErrorExitMsg(PsychError_user, "Invalid 'infoType' argument specified! Valid are 0, 1, 2, 3, 4 and 5.");

	// Windowserver info requested?
	if (infoType == 2 || infoType == 3) {
		// Return info about WindowServer:
		#if PSYCH_SYSTEM == PSYCH_OSX

		const char *CoreGraphicsFieldNames[]={ "CGSFps", "CGSValue1", "CGSValue2", "CGSValue3", "CGSDebugOptions" };
		const int CoreGraphicsFieldCount = 5;
		float cgsFPS, val1, val2, val3;
		
		// This (undocumented) Apple call retrieves information about performance statistics of
		// the Core graphics server, also known as WindowServer or Quartz compositor:
		CGSGetPerformanceData(_CGSDefaultConnection(), &cgsFPS, &val1, &val2, &val3);
		if (CGSGetDebugOptions(&retIntArg)) {
			if (PsychPrefStateGet_Verbosity() > 1) printf("PTB-WARNING: GetWindowInfo: Call to CGSGetDebugOptions() failed!\n");
		}
		
		PsychAllocOutStructArray(1, FALSE, 1, CoreGraphicsFieldCount, CoreGraphicsFieldNames, &s);
		PsychSetStructArrayDoubleElement("CGSFps", 0   , cgsFPS, s);
		PsychSetStructArrayDoubleElement("CGSValue1", 0, val1, s);
		PsychSetStructArrayDoubleElement("CGSValue2", 0, val2, s);
		PsychSetStructArrayDoubleElement("CGSValue3", 0, val3, s);
		PsychSetStructArrayDoubleElement("CGSDebugOptions", 0, (double) retIntArg, s);
		
		if ( (infoType == 3) && PsychCopyInDoubleArg(3, FALSE, &auxArg1) ) {
			// Type 3 setup request with auxArg1 provided. Apple auxArg1 as debugFlag setting
			// for the CoreGraphics server: DANGEROUS!
			if (CGSSetDebugOptions((unsigned int) auxArg1)) {
				if (PsychPrefStateGet_Verbosity() > 1) printf("PTB-WARNING: GetWindowInfo: Call to CGSSetDebugOptions() failed!\n");
			}
		}

		#endif
		
		#if PSYCH_SYSTEM == PSYCH_WINDOWS
		psych_uint64 onsetVBLCount, frameId;
		double onsetVBLTime, compositionRate;
		psych_uint64 targetVBL;
		
		PsychAllocInWindowRecordArg(kPsychUseDefaultArgPosition, TRUE, &windowRecord);
		// Query all DWM presentation timing info, return full info as struct in optional return argument '1':
		if (PsychOSGetPresentationTimingInfo(windowRecord, TRUE, 0, &onsetVBLCount, &onsetVBLTime, &frameId, &compositionRate, 1)) {
			// Query success: Info struct has been created and returned by PsychOSGetPresentationTimingInfo()...
			auxArg1 = auxArg2 = 0;
			auxArg3 = 2;
			
			// Want us to change settings?
			if ( (infoType == 3) && PsychCopyInDoubleArg(3, FALSE, &auxArg1) && PsychCopyInDoubleArg(4, FALSE, &auxArg2) && PsychCopyInDoubleArg(5, FALSE, &auxArg3)) {
				if (auxArg1 < 0) auxArg1 = 0;
				targetVBL = auxArg1;
				if (PsychOSSetPresentParameters(windowRecord, targetVBL, (int) auxArg3, auxArg2)) {
					if (PsychPrefStateGet_Verbosity() > 5) printf("PTB-DEBUG: GetWindowInfo: Call to PsychOSSetPresentParameters(%i, %i, %f) SUCCESS!\n", (int) auxArg1, (int) auxArg3, auxArg2);
				}
				else {
					if (PsychPrefStateGet_Verbosity() > 1) printf("PTB-WARNING: GetWindowInfo: Call to PsychOSSetPresentParameters() failed!\n");
				}
			}
		}
		else {
			// Unsupported / Failed:
			PsychCopyOutDoubleArg(1, FALSE, -1);
		}

		#endif

		#if PSYCH_SYSTEM == PSYCH_LINUX
			if (infoType == 2) {
				// MMIO register Read for screenid "auxArg1", register offset "auxArg2":
				PsychCopyInDoubleArg(3, TRUE, &auxArg1);
				PsychCopyInDoubleArg(4, TRUE, &auxArg2);
				PsychCopyOutDoubleArg(1, FALSE, (double) PsychOSKDReadRegister((int) auxArg1, (unsigned int) auxArg2, NULL));
			}
			
			if (infoType == 3) {
				// MMIO register Write for screenid "auxArg1", register offset "auxArg2", to value "auxArg3":
				PsychCopyInDoubleArg(3, TRUE, &auxArg1);
				PsychCopyInDoubleArg(4, TRUE, &auxArg2);
				PsychCopyInDoubleArg(5, TRUE, &auxArg3);
				PsychOSKDWriteRegister((int) auxArg1, (unsigned int) auxArg2, (unsigned int) auxArg3, NULL);
			}
		#endif

		// Done.
		return(PsychError_none);
	}

    // Get the window record:
    PsychAllocInWindowRecordArg(kPsychUseDefaultArgPosition, TRUE, &windowRecord);
	onscreen = PsychIsOnscreenWindow(windowRecord);

	if (onscreen) {
		// Query rasterbeam position: Will return -1 if unsupported.
		PsychGetCGDisplayIDFromScreenNumber(&displayId, windowRecord->screenNumber);
		beamposition = (double) PsychGetDisplayBeamPosition(displayId, windowRecord->screenNumber);
	}
	else {
		beamposition = -1;
	}
	
	if (infoType == 1) {
		// Return the measured beamposition:
		PsychCopyOutDoubleArg(1, FALSE, beamposition);
	}
    else if (infoType == 4) {
        // Return async flip state: 1 = Active, 0 = Inactive.
        PsychCopyOutDoubleArg(1, FALSE, (((NULL != windowRecord->flipInfo) && (0 != windowRecord->flipInfo->asyncstate)) ? 1 : 0));
    }
	else if (infoType == 5) {
		// Create a GL_EXT_timer_query object for this window:
		if (glewIsSupported("GL_EXT_timer_query")) {
			// Pending queries finished?
			if (windowRecord->gpuRenderTimeQuery > 0) {
				PsychErrorExitMsg(PsychError_user, "Tried to create a new GPU rendertime query, but last query not yet finished! Call Screen('Flip') first!");
			}
			
			// Enable our rendering context by selecting this window as drawing target:
			PsychSetDrawingTarget(windowRecord);
			
			// Generate Query object:
			glGenQueries(1, &windowRecord->gpuRenderTimeQuery);
			
			// Emit Query: GPU will measure elapsed processing time in Nanoseconds, starting
			// with the first GL command executed after this command:
			glBeginQuery(GL_TIME_ELAPSED_EXT, windowRecord->gpuRenderTimeQuery);
			
			// Reset last measurement:
			windowRecord->gpuRenderTime = 0;
		}
		else {
			if (PsychPrefStateGet_Verbosity() > 4) printf("PTB-INFO: GetWindowInfo for infoType 5: GPU timer query objects are unsupported on this platform and GPU. Call ignored!\n");
		}
	}
	else {
		// Return all information:
		PsychAllocOutStructArray(1, FALSE, 1, fieldCount, FieldNames, &s);

		// Rasterbeam position:
		PsychSetStructArrayDoubleElement("Beamposition", 0, beamposition, s);

		// Time of last vertical blank when a double-buffer swap occured:
		PsychSetStructArrayDoubleElement("LastVBLTimeOfFlip", 0, windowRecord->time_at_last_vbl, s);

		// Uncorrected timestamp of flip swap completion:
		PsychSetStructArrayDoubleElement("RawSwapTimeOfFlip", 0, windowRecord->rawtime_at_swapcompletion, s);

		// Timestamp immediately prior to call to PsychOSFlipWindowBuffers(), i.e., time at swap request submission:
		PsychSetStructArrayDoubleElement("TimeAtSwapRequest", 0, windowRecord->time_at_swaprequest, s);

		// Timestamp immediately after call to PsychOSFlipWindowBuffers() returns, i.e., time at swap request submission completion:
		PsychSetStructArrayDoubleElement("TimePostSwapRequest", 0, windowRecord->time_post_swaprequest, s);

		// Result from last GPU rendertime query as triggered by infoType 5: Zero if undefined.
		PsychSetStructArrayDoubleElement("GPULastFrameRenderTime", 0, windowRecord->gpuRenderTime, s);

		// Try to determine system time of last VBL on display, independent of any
		// flips / bufferswaps.
		lastvbl = -1;
		postflip_vblcount = 0;
		
		// On supported systems, we can query the OS for the system time of last VBL, so we can
		// use the most recent VBL timestamp as baseline for timing calculations, 
		// instead of one far in the past.
		if (onscreen) { lastvbl = PsychOSGetVBLTimeAndCount(windowRecord, &postflip_vblcount); }

		// If we couldn't determine this information we just set lastvbl to the last known
		// vbl timestamp of last flip -- better than nothing...
		if (lastvbl < 0) lastvbl = windowRecord->time_at_last_vbl;
		PsychSetStructArrayDoubleElement("LastVBLTime", 0, lastvbl, s);
		PsychSetStructArrayDoubleElement("VBLCount", 0, (double) (psych_int64) postflip_vblcount, s);
        
		// Misc. window parameters:
		PsychSetStructArrayDoubleElement("StereoMode", 0, windowRecord->stereomode, s);
		PsychSetStructArrayDoubleElement("ImagingMode", 0, windowRecord->imagingMode, s);
		PsychSetStructArrayDoubleElement("SpecialFlags", 0, windowRecord->specialflags, s);
		PsychSetStructArrayDoubleElement("IsFullscreen", 0, (windowRecord->specialflags & kPsychIsFullscreenWindow) ? 1 : 0, s);
		PsychSetStructArrayDoubleElement("MultiSampling", 0, windowRecord->multiSample, s);
		PsychSetStructArrayDoubleElement("MissedDeadlines", 0, windowRecord->nr_missed_deadlines, s);
		PsychSetStructArrayDoubleElement("StereoDrawBuffer", 0, windowRecord->stereodrawbuffer, s);
		PsychSetStructArrayDoubleElement("GuesstimatedMemoryUsageMB", 0, (double) windowRecord->surfaceSizeBytes / 1024 / 1024, s);
		PsychSetStructArrayDoubleElement("BitsPerColorComponent", 0, (double) windowRecord->bpc, s);
		
		// Query real size of the underlying display in order to define the vbl_startline:
		PsychGetScreenSize(windowRecord->screenNumber, &scw, &sch);
		vbl_startline = (double) sch;
		PsychSetStructArrayDoubleElement("VBLStartline", 0, vbl_startline, s);

		// And VBL endline:
		PsychSetStructArrayDoubleElement("VBLEndline", 0, windowRecord->VBL_Endline, s);

		// Video refresh interval duration from beamposition method:
		PsychSetStructArrayDoubleElement("VideoRefreshFromBeamposition", 0, windowRecord->ifi_beamestimate, s);
    
        // Which basic GPU architecture is this?
		PsychSetStructArrayStringElement("GPUCoreId", 0, windowRecord->gpuCoreId, s);
		
		// FBO's supported, and how deep?
		if (windowRecord->gfxcaps & kPsychGfxCapFBO) {
			if (windowRecord->gfxcaps & kPsychGfxCapFPFBO32) {
				PsychSetStructArrayDoubleElement("GLSupportsFBOUpToBpc", 0, 32, s);
			} else
			if (windowRecord->gfxcaps & kPsychGfxCapFPFBO16) {
				PsychSetStructArrayDoubleElement("GLSupportsFBOUpToBpc", 0, 16, s);
			} else PsychSetStructArrayDoubleElement("GLSupportsFBOUpToBpc", 0, 8, s);
		}
		else {
			PsychSetStructArrayDoubleElement("GLSupportsFBOUpToBpc", 0, 0, s);
		}

		// How deep is alpha blending supported?
		if (windowRecord->gfxcaps & kPsychGfxCapFPBlend32) {
			PsychSetStructArrayDoubleElement("GLSupportsBlendingUpToBpc", 0, 32, s);
		} else if (windowRecord->gfxcaps & kPsychGfxCapFPBlend16) {
			PsychSetStructArrayDoubleElement("GLSupportsBlendingUpToBpc", 0, 16, s);
		} else PsychSetStructArrayDoubleElement("GLSupportsBlendingUpToBpc", 0, 8, s);
		
		// How deep is texture mapping supported?
		if (windowRecord->gfxcaps & kPsychGfxCapFPTex32) {
			PsychSetStructArrayDoubleElement("GLSupportsTexturesUpToBpc", 0, 32, s);
		} else if (windowRecord->gfxcaps & kPsychGfxCapFPTex16) {
			PsychSetStructArrayDoubleElement("GLSupportsTexturesUpToBpc", 0, 16, s);
		} else PsychSetStructArrayDoubleElement("GLSupportsTexturesUpToBpc", 0, 8, s);
		
		// How deep is texture filtering supported?
		if (windowRecord->gfxcaps & kPsychGfxCapFPFilter32) {
			PsychSetStructArrayDoubleElement("GLSupportsFilteringUpToBpc", 0, 32, s);
		} else if (windowRecord->gfxcaps & kPsychGfxCapFPFilter16) {
			PsychSetStructArrayDoubleElement("GLSupportsFilteringUpToBpc", 0, 16, s);
		} else PsychSetStructArrayDoubleElement("GLSupportsFilteringUpToBpc", 0, 8, s);

		if (windowRecord->gfxcaps & kPsychGfxCapVCGood) {
			PsychSetStructArrayDoubleElement("GLSupportsPrecisionColors", 0, 1, s);
		} else PsychSetStructArrayDoubleElement("GLSupportsPrecisionColors", 0, 0, s);

		if (windowRecord->gfxcaps & kPsychGfxCapFP32Shading) {
			PsychSetStructArrayDoubleElement("GLSupportsFP32Shading", 0, 1, s);
		} else PsychSetStructArrayDoubleElement("GLSupportsFP32Shading", 0, 0, s);

		// Renderer information: This comes last, and would fail if async flips
        // are active, because it needs PsychSetDrawingTarget, which in turn needs async
        // flips to be inactive:
        PsychSetDrawingTarget(windowRecord);
        PsychSetStructArrayStringElement("GLVendor", 0, glGetString(GL_VENDOR), s);
        PsychSetStructArrayStringElement("GLRenderer", 0, glGetString(GL_RENDERER), s);
        PsychSetStructArrayStringElement("GLVersion", 0, glGetString(GL_VERSION), s);
    }
    
    // Done.
    return(PsychError_none);
}
