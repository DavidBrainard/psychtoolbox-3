/*
	PsychToolbox3/Source/Common/Screen/PsychGraphicsHardwareHALSupport.c		

	AUTHORS:
	
		mario.kleiner@tuebingen.mpg.de		mk

	PLATFORMS:	
	
		All. However with dependencies on OS specific glue-layers which are mostly Linux/OSX for now...

	HISTORY:
	
	01/12/2008	mk		Created.
	
	DESCRIPTION:

	This file is a container for miscellaneous routines that take advantage of specific low level
	features of graphics/related hardware and the target operating system to achieve special tasks.
	
	Most of the routines here are more tied to specific displays (screens) than to windows and usually
	only a subset of these routines is available for a specific system configuration with a specific
	model of graphics card. Other layers of PTB should not rely on these routines being supported on
	a given system config and should be prepared to have fallback-implementations.
	
	Many of the features are experimental in nature!

	TO DO:

*/

#include "PsychGraphicsHardwareHALSupport.h"

// Include specifications of the GPU registers:
#include "PsychGraphicsCardRegisterSpecs.h"

// Maps screenid's to Graphics hardware pipelines: Used to choose pipeline for beampos-queries and similar
// GPU crtc specific stuff:
static int	displayScreensToPipes[kPsychMaxPossibleDisplays];

// Corrective values for beamposition queries to correct for any constant and systematic offsets in
// the scanline positions returned by low-level code:
static int	screenBeampositionBias[kPsychMaxPossibleDisplays];
static int	screenBeampositionVTotal[kPsychMaxPossibleDisplays];

/* PsychSynchronizeDisplayScreens() -- (Try to) synchronize display refresh cycles of multiple displays
 *
 * This tries whatever method is available/appropriate/or requested to synchronize the video refresh
 * cycles of multiple graphics cards physical display heads -- corresponding to PTB logical Screens.
 *
 * The method may or may not be supported on a specific OS/gfx-card combo. It will return a PsychError_unimplemented
 * if it can't do what core wants.
 *
 * numScreens	=	Ptr to the number of display screens to sync. If numScreens>0, all screens with the screenIds stored
 *					in the integer array 'screenIds' will be synched. If numScreens == 0, PTB will try to sync all
 *					available screens in the system. On return, the location will contain the count of synced screens.
 *
 * screenIds	=	Either a list with 'numScreens' screenIds for the screens to sync, or NULL if numScreens == 0.
 *
 * residuals	=	List with 'numScreens' (on return) values indicating the residual sync error wrt. to the first
 *					screen (the reference). Ideally all items should contain zero for perfect sync on return.
 *
 * syncMethod	=	Numeric Id for the sync method to use: 0 = Don't care, whatever is appropriate. 1 = Only hard
 *					sync, which is fast and reliable if supported. 2 = Soft sync via drift-syncing. More to come...
 *
 * syncTimeOut	=	If some non-immediate method is requested/chosen, it should give up after syncTimeOut seconds if
 *					it doesn't manage to bring the displays in sync in that timeframe.
 *
 * allowedResidual = How many scanlines offset after sync are acceptable? Will retry until syncTimeOut if criterion not met.
 *
 */
PsychError PsychSynchronizeDisplayScreens(int *numScreens, int* screenIds, int* residuals, unsigned int syncMethod, double syncTimeOut, int allowedResidual)
{
	// Currently, we only support a hard, immediate sync of all display heads of a single dual-head gfx-card,
	// so we ignore most of our arguments. Well, still check them for validity, but then ignore them after
	// successfull validation ;-)
	
	if (numScreens == NULL) PsychErrorExitMsg(PsychError_internal, "NULL-Ptr passed as numScreens argument!");
	if (*numScreens < 0 || *numScreens >= PsychGetNumDisplays()) PsychErrorExitMsg(PsychError_internal, "Invalid number passed as numScreens argument! (Negativ or more than available screens)");
	if (syncMethod < 0 || syncMethod > 2) PsychErrorExitMsg(PsychError_internal, "Invalid syncMethod argument passed!");
	if (syncTimeOut < 0) PsychErrorExitMsg(PsychError_internal, "Invalid (negative) syncTimeOut argument passed!");
	if (allowedResidual < 0) PsychErrorExitMsg(PsychError_internal, "Invalid (negative) allowedResidual argument passed!");
	
	// System support:
	#if PSYCH_SYSTEM == PSYCH_WINDOWS
		if(PsychPrefStateGet_Verbosity() > 1) printf("PTB-WARNING: Synchronization of graphics display heads requested, but this is not supported on MS-Windows.\n");
		return(PsychError_unimplemented);
	#endif
	
	#if PSYCH_SYSTEM == PSYCH_LINUX
		// Dispatch to routine in PsychScreenGlue.c Linux:
		return(PsychOSSynchronizeDisplayScreens(numScreens, screenIds, residuals, syncMethod, syncTimeOut, allowedResidual));
	#endif
	
	#if PSYCH_SYSTEM == PSYCH_OSX
		// Dispatch to routine in PsychScreenGlue.c OSX:
		return(PsychOSSynchronizeDisplayScreens(numScreens, screenIds, residuals, syncMethod, syncTimeOut, allowedResidual));
	#endif
	
	// Often not reached...
	return(PsychError_none);
}

/* PsychEnableNative10BitFramebuffer()  - Enable/Disable native 10 bpc RGB framebuffer modes.
 *
 * This function enables or disables the native ARGB2101010 framebuffer readout mode of supported
 * graphics hardware. Currently the ATI Radeon X1000/HD2000/HD3000 and later cards should allow this.
 *
 * This needs support from the Psychtoolbox kernel level support driver for low-level register reads
 * and writes to the GPU registers.
 *
 * 'windowRecord'	Is used to find the Id of the screen for which mode should be changed, as well as enable
 *					flags to see if a change is required at all, and the OpenGL context for some specific
 *					fixups. A value of NULL will try to apply the operation to all heads, but may only work
 *					for *disabling* 10 bpc mode, not for enabling it -- Mostly useful for a master reset to
 *					system default, e.g., as part of error handling or Screen shutdown handling.
 * 'enable'   True = Enable ABGR2101010 support, False = Disable ARGB2101010 support, reenable ARGB8888 support. 
 *
 */
psych_bool	PsychEnableNative10BitFramebuffer(PsychWindowRecordType* windowRecord, psych_bool enable)
{
	int i,si,ei, headid, screenId;
	unsigned int lutreg, ctlreg, value, status;
	
	// Child protection:
	if (windowRecord && !PsychIsOnscreenWindow(windowRecord)) PsychErrorExitMsg(PsychError_internal, "Invalid non-onscreen windowRecord provided!!!");
	
	// Either screenid from windowRecord or our special -1 "all Screens" Id:
	screenId = (windowRecord) ? windowRecord->screenNumber : -1;
	
	// Define range of screens: Either a single specific one, or all:
	si = (screenId!=-1) ? screenId   : 0;
	ei = (screenId!=-1) ? screenId+1 : PsychGetNumDisplays();

#if PSYCH_SYSTEM == PSYCH_OSX || PSYCH_SYSTEM == PSYCH_LINUX
	// Loop over all target screens:
	for (i=si; i<ei; i++) {
		// Map screenid to headid: For now we only support 2 heads.
		headid = PsychScreenToHead(i);
		
		// Select Radeon HW registers for corresponding heads:
		lutreg = (headid == 0) ? RADEON_D1GRPH_LUT_SEL : RADEON_D2GRPH_LUT_SEL;
		ctlreg = (headid == 0) ? RADEON_D1GRPH_CONTROL : RADEON_D2GRPH_CONTROL;

		// Enable or Disable?
		if (enable) {
			// Enable:
			
			// Switch hardware LUT's to bypass mode:
			// We set bit 8 to enable "bypass LUT in 2101010 mode":
			value = PsychOSKDReadRegister(screenId, lutreg, &status);
			if (status) {
				printf("PTB-ERROR: Failed to set 10 bit framebuffer mode (LUTReg read failed).\n");
				return(false);
			}

			// Set the bypass bit:
			value = value | 0x1 << 8;

			PsychOSKDWriteRegister(screenId, lutreg, value, &status);
			if (status) {
				printf("PTB-ERROR: Failed to set 10 bit framebuffer mode (LUTReg write failed).\n");
				return(false);
			}
			
			// Switch CRTC to ABGR2101010 readout mode:
			// We set bit 8 to enable that mode:
			value = PsychOSKDReadRegister(screenId, ctlreg, &status);
			if (status) {
				printf("PTB-ERROR: Failed to set 10 bit framebuffer mode (CTLReg read failed).\n");
				return(false);
			}

			// Set 2101010 moe bit:
			value = value | 0x1 << 8;

			PsychOSKDWriteRegister(screenId, ctlreg, value, &status);
			if (status) {
				printf("PTB-ERROR: Failed to set 10 bit framebuffer mode (CTLReg write failed).\n");
				return(false);
			}
			
			// Pipe should be in 10 bpc mode now...
			if (PsychPrefStateGet_Verbosity() > 2) printf("PTB-INFO: System framebuffer switched to ARGB2101010 mode for screen %i [head %i].\n", i, headid);
		}
		else {
			// Disable:

			// Switch CRTC to ABGR8888 readout mode:
			// We clear bit 8 to enable that mode:
			value = PsychOSKDReadRegister(screenId, ctlreg, &status);
			if (status) {
				// This codepath gets always called in PsychCloseWindow(), so we should skip it
				// silently if register read fails, as this is expected on MS-Windows and on all
				// non-Radeon hardware and if kernel driver isn't loaded:
				if (PsychPrefStateGet_Verbosity() > 5) printf("PTB-ERROR: Failed to set 8 bit framebuffer mode (CTLReg read failed).\n");
				return(false);
			}
			else if (PsychPrefStateGet_Verbosity() > 5) printf("PTB-DEBUG: In disable 10bpc: Readreg. ctlreg yields %lx\n", value);

			// Clear 2101010 mode bit:
			value = value & ~(0x1 << 8);

			PsychOSKDWriteRegister(screenId, ctlreg, value, &status);
			if (status) {
				printf("PTB-ERROR: Failed to set 8 bit framebuffer mode (CTLReg write failed).\n");
				return(false);
			}
			else if (PsychPrefStateGet_Verbosity() > 5) printf("PTB-DEBUG: In disable 10bpc: ctlreg reset\n");

			// Wait 500 msecs for GPU to settle:
			PsychWaitIntervalSeconds(0.5);

			// Switch hardware LUT's to standard mapping mode:
			// We clear bit 8 to disable "bypass LUT in 2101010 mode":
			value = PsychOSKDReadRegister(screenId, lutreg, &status);
			if (status) {
				printf("PTB-ERROR: Failed to set 8 bit framebuffer mode (LUTReg read failed).\n");
				return(false);
			}
			else if (PsychPrefStateGet_Verbosity() > 5) printf("PTB-DEBUG: In disable 10bpc: Readreg. lutreg yields %lx\n", value);

			// Clear LUT bypass bit:
			value = value & ~(0x1 << 8);

			PsychOSKDWriteRegister(screenId, lutreg, value, &status);
			if (status) {
				printf("PTB-ERROR: Failed to set 8 bit framebuffer mode (LUTReg write failed).\n");
				return(false);
			}
			else if (PsychPrefStateGet_Verbosity() > 5) printf("PTB-DEBUG: In disable 10bpc: lutreg reset\n");

			// Pipe should be in 8 bpc mode now...
			if (PsychPrefStateGet_Verbosity() > 2) printf("PTB-INFO: System framebuffer switched to standard ARGB8888 mode for screen %i [head %i].\n", i, headid);
		}

		// Next display head...
	}
#else
	// This cool stuff not supported on the uncool Windows OS:
	return(FALSE);
#endif

	// Done.
	return(TRUE);
}

/* PsychFixupNative10BitFramebufferEnableAfterEndOfSceneMarker()
 *
 * Undo changes made by the graphics driver to the framebuffer pixel format control register
 * as part of an OpenGL/Graphics op that marks "End of Scene", e.g., a glClear() command, that
 * would revert the framebuffers opmode to standard 8bpc mode and thereby kill our 10 bpc mode
 * setting.
 *
 * This routine *must* be called after each such problematic "End of scene" marker command like
 * glClear(). The routine does nothing if 10bpc mode is not enabled/requested for the corresponding
 * display head associated with the given onscreen window. It rewrites the control register on
 * 10 bpc configured windows to basically undo the unwanted change of the gfx-driver *before*
 * a vertical retrace cycle starts, ie., before that changes take effect (The register is double-
 * buffered and latched to update only at VSYNC time, so we just have to be quick enough).
 *
 *
 * Expected Sequence of operations is:
 * 1. Some EOS command like glClear() issued.
 * 2. EOS command schedules ctrl register update to "bad" value at next VSYNC.
 * 3. This routine gets called, detects need for fixup, glGetError() waits for "2." to finish.
 * 4. This routine undos the "bad" value change request by overwriting the latch with our
 *    "good" value --> Scheduled for next VSYNC. Then it returns...
 * 5. At next VSYNC or old "good" value is overwritten/latched with our new old "good" value,
 *    --> "good value" persists, framebuffer stays in 2101010 configuration --> All good.
 *
 * So far the theory, let's see if this really works in real world...
 *
 * This is not needed in Carbon+AGL windowed mode, as the driver doesnt' mess with the control
 * register there, but that mode has its own share of drawback, e.g., generally reduced performance
 * and less robust stimulus onset timing and timestamping... Life's full of tradeoffs...
 */
void PsychFixupNative10BitFramebufferEnableAfterEndOfSceneMarker(PsychWindowRecordType* windowRecord)
{
#if PSYCH_SYSTEM == PSYCH_OSX || PSYCH_SYSTEM == PSYCH_LINUX

	int i,si,ei, headid, screenId;
	unsigned int lutreg, ctlreg, value, status;

	// Fixup needed? Only if 10bpc mode is supposed to be active! Early exit if not:
	if (!(windowRecord->specialflags & kPsychNative10bpcFBActive)) return;

	// This command must be called with the OpenGL context of the given windowRecord active, so
	// we can rely on glGetError() waiting for the correct pipeline to settle! Wait via glGetError()
	// for the end-of-scene marker to finish completely, so our register write happens after
	// the "wrong" register write of that command. glFinish() doesn't work here for unknown
	// reasons - probably it waits too long or whatever. Pretty shaky this stuff...
	glGetError();
	
	// Ok, now rewrite the double-buffered (latched) register with our "good" value for keeping
	// the 10 bpc framebuffer online:
	
	// Map windows screen to gfx-headid aka register subset. TODO : We'll need something better,
	// more generic, abstracted out for the future, but as a starter this will do:
	screenId = windowRecord->screenNumber;
	headid = PsychScreenToHead(screenId);
	ctlreg = (headid == 0) ? RADEON_D1GRPH_CONTROL : RADEON_D2GRPH_CONTROL;
	
	// One-liner read-modify-write op, which simply sets bit 8 of the register - the "Enable 2101010 mode" bit:
	PsychOSKDWriteRegister(screenId, ctlreg, (0x1 << 8) | PsychOSKDReadRegister(screenId, ctlreg, NULL), NULL);
	
	// Debug output, if wanted:
	if (PsychPrefStateGet_Verbosity() > 5) printf("PTB-DEBUG: PsychFixupNative10BitFramebufferEnableAfterEndOfSceneMarker(): ARGB2101010 bit set on screen %i, head %i.\n", screenId, headid);

#endif

	// Done.
	return;
}

/* Stores content of GPU's surface address registers of the surfaces that
 * correspond to the windowRecords frontBuffers. Only called inside
 * PsychFlipWindowBuffers() immediately before triggering a double-buffer
 * swap. The values are used as reference values: If another readout of
 * these registers shows values different from the ones stored preflip,
 * then that is a certain indicator that bufferswap has happened.
 */
void PsychStoreGPUSurfaceAddresses(PsychWindowRecordType* windowRecord)
{

#if PSYCH_SYSTEM == PSYCH_OSX || PSYCH_SYSTEM == PSYCH_LINUX

	// If we are called, we know that 'windowRecord' is an onscreen window.
	int screenId = windowRecord->screenNumber;

	// Just need to check if GPU low-level access is supported:
	if (!PsychOSIsKernelDriverAvailable(screenId)) return;
	
	// Driver is online: Read the registers:
	windowRecord->gpu_preflip_Surfaces[0] = PsychOSKDReadRegister(screenId, (PsychScreenToHead(screenId) <= 0) ? RADEON_D1GRPH_PRIMARY_SURFACE_ADDRESS : RADEON_D2GRPH_PRIMARY_SURFACE_ADDRESS, NULL);
	windowRecord->gpu_preflip_Surfaces[1] = PsychOSKDReadRegister(screenId, (PsychScreenToHead(screenId) <= 0) ? RADEON_D1GRPH_SECONDARY_SURFACE_ADDRESS : RADEON_D2GRPH_SECONDARY_SURFACE_ADDRESS, NULL);

#endif

	return;
}

/*  PsychWaitForBufferswapPendingOrFinished()
 *  Waits until a bufferswap for window windowRecord has either already happened or
 *  bufferswap is certain.
 *  Input values:
 *  windowRecord struct of onscreen window to monitor.
 *  timestamp    = Deadline for abortion of flip detection at input.
 *
 *  Return values:
 *  timestamp    = System time at polling loop exit.
 *  beamposition = Beamposition at polling loop exit.
 *
 *  Return value: FALSE if swap happened already, TRUE if swap is imminent.
 */
psych_bool PsychWaitForBufferswapPendingOrFinished(PsychWindowRecordType* windowRecord, double* timestamp, int *beamposition)
{
#if PSYCH_SYSTEM == PSYCH_OSX || PSYCH_SYSTEM == PSYCH_LINUX
    CGDirectDisplayID displayID;
	unsigned int primarySurface, secondarySurface;
	unsigned int updateStatus;
	double deadline = *timestamp;

	// If we are called, we know that 'windowRecord' is an onscreen window.
	int screenId = windowRecord->screenNumber;

    // Retrieve display id and screen size spec that is needed later...
    PsychGetCGDisplayIDFromScreenNumber(&displayID, screenId);

#define RADEON_D1GRPH_UPDATE	0x6144
#define RADEON_D2GRPH_UPDATE	0x6944
#define RADEON_SURFACE_UPDATE_PENDING 4
#define RADEON_SURFACE_UPDATE_TAKEN   8

	// Just need to check if GPU low-level access is supported:
	if (!PsychOSIsKernelDriverAvailable(screenId)) return;
	
	// Driver is online. Enter polling loop:
	while (TRUE) {
		// Read surface address registers:
		primarySurface   = PsychOSKDReadRegister(screenId, (PsychScreenToHead(screenId) <= 0) ? RADEON_D1GRPH_PRIMARY_SURFACE_ADDRESS : RADEON_D2GRPH_PRIMARY_SURFACE_ADDRESS, NULL);
		secondarySurface = PsychOSKDReadRegister(screenId, (PsychScreenToHead(screenId) <= 0) ? RADEON_D1GRPH_SECONDARY_SURFACE_ADDRESS : RADEON_D2GRPH_SECONDARY_SURFACE_ADDRESS, NULL);

		// Read update status registers:
		updateStatus     = PsychOSKDReadRegister(screenId, (PsychScreenToHead(screenId) <= 0) ? RADEON_D1GRPH_UPDATE : RADEON_D2GRPH_UPDATE, NULL);

		PsychGetAdjustedPrecisionTimerSeconds(timestamp);

		if (primarySurface!=windowRecord->gpu_preflip_Surfaces[0] || secondarySurface!=windowRecord->gpu_preflip_Surfaces[1] || (updateStatus & (RADEON_SURFACE_UPDATE_PENDING | RADEON_SURFACE_UPDATE_TAKEN)) || (*timestamp > deadline)) {
			// Abort condition: Exit loop.
			break;
		}
		
		if (PsychPrefStateGet_Verbosity() > 9) {
			printf("PTB-DEBUG: Head %i: primarySurface=%p : secondarySurface=%p : updateStatus=%i\n", PsychScreenToHead(screenId), primarySurface, secondarySurface, updateStatus);
		}

		// Sleep slacky at least 200 microseconds, then retry:
		PsychYieldIntervalSeconds(0.0002);
	};
	
	// Take timestamp and beamposition:
	*beamposition = PsychGetDisplayBeamPosition(displayID, screenId);
	PsychGetAdjustedPrecisionTimerSeconds(timestamp);

	// Exit due to timeout?
	if (*timestamp > deadline) {
		// Mark timestamp as invalid due to timeout:
		*timestamp = -1;
	}
	
	// Return FALSE if bufferswap happened already, TRUE if swap is still pending:
	return((updateStatus & RADEON_SURFACE_UPDATE_PENDING) ? TRUE : FALSE);
#else
	// On Windows, we always return "swap happened":
	return(FALSE);
#endif
}

/* PsychGetNVidiaGPUType()
 *
 * Decodes hw register of NVidia GPU into GPU core id / chip family:
 * Returns 0 for unknown card, otherwise xx for NV_xx:
 */
unsigned int PsychGetNVidiaGPUType(PsychWindowRecordType* windowRecord)
{
#if PSYCH_SYSTEM == PSYCH_OSX || PSYCH_SYSTEM == PSYCH_LINUX
	psych_uint32 chipset, card_type;

	// Get hardware id code from gpu register:
	psych_uint32 reg0 = PsychOSKDReadRegister((windowRecord) ? windowRecord->screenNumber : 0, NV03_PMC_BOOT_0, NULL);
	
	/* We're dealing with >=NV10 */
	if ((reg0 & 0x0f000000) > 0) {
		/* Bit 27-20 contain the architecture in hex */
		chipset = (reg0 & 0xff00000) >> 20;
		/* NV04 or NV05 */
	} else if ((reg0 & 0xff00fff0) == 0x20004000) {
		if (reg0 & 0x00f00000)
			chipset = 0x05;
		else
			chipset = 0x04;
	} else
		chipset = 0xff;
	
	switch (chipset & 0xf0) {
		case 0x00:
			// NV_04/05: RivaTNT , RivaTNT2
			card_type = 0x04;
			break;
		case 0x10:
		case 0x20:
		case 0x30:
			// NV30 or earlier: GeForce-5 / GeForceFX and earlier:
			card_type = chipset & 0xf0;
			break;
		case 0x40:
		case 0x60:
			// NV40: GeForce6/7 series:
			card_type = 0x40;
			break;
		case 0x50:
		case 0x80:
		case 0x90:
		case 0xa0:
			// NV50: GeForce8/9/Gxxx
			card_type = 0x50;
			break;
		case 0xc0:
			// Curie: GTX-400 and later:
			card_type = 0xc0;
			break;
		default:
			printf("PTB-DEBUG: Unknown NVidia chipset 0x%08x \n", reg0);
			card_type = 0x00;
	}
	
	return(card_type);
#else
	return(0);
#endif
}

/* PsychScreenToHead() - Map PTB screenId to GPU headId (aka pipeId): */
int	PsychScreenToHead(int screenId)
{
	return(displayScreensToPipes[screenId]);
}

/* PsychSetScreenToHead() - Change mapping of a PTB screenId to GPU headId: */
void PsychSetScreenToHead(int screenId, int headId)
{
	displayScreensToPipes[screenId] = headId;
}

/* PsychInitScreenToHeadMappings() - Setup initial mapping for 'numDisplays' displays:
 *
 * Called from end of InitCGDisplayIDList() during os-specific display initialization.
 *
 * 1. Starts with an identity mapping screen 0 -> head 0, screen 1 -> head 1 ...
 *
 * 2. Allows override of mapping via environment variable "PSYCHTOOLBOX_PIPEMAPPINGS",
 * Format is: One character (a number between "0" and "9") for each screenid,
 * e.g., "021" would map screenid 0 to pipe 0, screenid 1 to pipe 2 and screenid 2 to pipe 1.
 *
 * 3. This mapping can be overriden via Screen('Preference', 'ScreenToHead') setting.
 *
 */
void PsychInitScreenToHeadMappings(int numDisplays)
{
    int i;
	char* ptbpipelines = NULL;
    
    // Setup default identity one-to-one mapping:
    for(i = 0; i < kPsychMaxPossibleDisplays; i++){
		displayScreensToPipes[i] = i;

		// We also setup beamposition bias values to "neutral defaults":
		screenBeampositionBias[i] = 0;
		screenBeampositionVTotal[i] = 0;
    }
	
	// Did user provide an override for the screenid --> pipeline mapping?
	ptbpipelines = getenv("PSYCHTOOLBOX_PIPEMAPPINGS");
	if (ptbpipelines) {
		// The default is "012...", ie screen 0 = pipe 0, 1 = pipe 1, 2 =pipe 2, n = pipe n
		for (i = 0; (i < strlen(ptbpipelines)) && (i < numDisplays); i++) {
			displayScreensToPipes[i] = (((ptbpipelines[i] - 0x30) >=0) && ((ptbpipelines[i] - 0x30) < kPsychMaxPossibleDisplays)) ? (ptbpipelines[i] - 0x30) : 0;
		}
	}
}

/* PsychGetBeamposCorrection() -- Get corrective beamposition values.
 * Some GPU's and drivers don't return the true vertical scanout position on
 * query, but a value that is offset by a constant value (for a give display mode).
 * This function returns corrective values to apply to the GPU returned values
 * to get the "true scanoutposition" for timestamping etc.
 *
 * Proper values are setup via PsychSetBeamposCorrection() from high-level startup code
 * if needed. Otherwise they are set to (0,0), so the correction is an effective no-op.
 *
 * truebeampos = measuredbeampos - *vblbias;
 * if (truebeampos < 0) truebeampos = *vbltotal + truebeampos;
 *
 */
void PsychGetBeamposCorrection(int screenId, int *vblbias, int *vbltotal)
{
	*vblbias  = screenBeampositionBias[screenId];
	*vbltotal = screenBeampositionVTotal[screenId];
}

/* PsychSetBeamposCorrection() -- Set corrective beamposition values.
 * Called from high-level setup/calibration code at onscreen window open time.
 */
void PsychSetBeamposCorrection(int screenId, int vblbias, int vbltotal)
{
	screenBeampositionBias[screenId] = vblbias;
	screenBeampositionVTotal[screenId] = vbltotal;
}
