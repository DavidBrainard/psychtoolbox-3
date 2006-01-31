/*
  ScreenPreferenceState.c		
  
  AUTHORS:
  Allen.Ingling@nyu.edu		awi 
  
  PLATFORMS:	Mac OS X MATLAB only.
    

  HISTORY:
  2/28/04  awi		Created.   
  5/30/05  mk           New preference setting screenSkipSyncTests: If set to TRUE, self-test and calibration will be as
                        short and sloppy as possible and some problematic conditions will not cause an abort.
                        One can trade-off correctness and accuracy of stimulus presentation against level of annoyance
                        during development/debugging of new experiments.
  9/30/05  mk           new setting VisualDebugLevel: Defines how much visual feedback PTB should give about errors and
                        state: 0=none, 1=only errors, 2=also warnings, 3=also infos, 4=also blue bootup screen, 5=also visual test sheets.
  DESCRIPTION:
  
  This file holds state for the Screen Preference command.
  
   
  
*/


#include "Screen.h"


//PsychTable preference state
static int								psychTableVersion;				//there is no psych table yet, this is provided for the future. 
static char								PsychTableCreator[]="Screen";   //there is no psych table yet, this is provided for the future.
//Text preference state
#define MAX_DEFAULT_FONT_NAME_LENGTH    256
#define INITIAL_DEFAULT_FONT_NAME		"Courier"
static char								defaultFontName[MAX_DEFAULT_FONT_NAME_LENGTH];
static int								defaultTextSize=12;
static int								defaultTextStyle=0; // 0=normal,1=bold,2=italic,4=underline,8=outline,32=condense,64=extend
static Boolean                                                          textAlphaBlending=FALSE;
static Boolean                                                          screenSkipSyncTests=FALSE;
//Debug preference state
static Boolean                                                          TimeMakeTextureFlag=FALSE;
static int								screenVisualDebugLevel=6;
static int                                                              screenConserveVRAM=0;
// If EmulateOldPTB is set to true, then try to behave like the old OS-9 PTB:
static Boolean                                                          EmulateOldPTB=FALSE;
// Support for real 3D rendering enabled?
static Boolean                                                          Enable_3d_gfx=FALSE;
//All state checking goes through accessors located in this file.  
void PrepareScreenPreferences(void)
{
	//set the fake psych table version
	psychTableVersion=20;
	
}


//Accessors


//****************************************************************************************************************
//PsychTable preferences

/*
preference: PsychTableVersion
*/
int PsychPrefStateGet_PsychTableVersion(void)
{
	return(psychTableVersion);
}

/*
preference: PsychTableCreator
*/
const char *PsychPrefStateGet_PsychTableCreator(void)
{
	return(PsychTableCreator);
}

//****************************************************************************************************************
//Text and Font preferences

/*
preference: DefaultFontName
*/
void PsychPrefStateGet_DefaultFontName(const char **fontName )
{
	static Boolean  firstTime=TRUE;
	
	if(firstTime){
		strcpy(defaultFontName, INITIAL_DEFAULT_FONT_NAME);
		firstTime=TRUE;
	}
	*fontName=defaultFontName;
}


void PsychPrefStateSet_DefaultFontName(const char *newName)
{
	if(strlen(newName)+1 > MAX_DEFAULT_FONT_NAME_LENGTH)
		PsychErrorExitMsg(PsychError_user, "Attempt to set a default font name using a string >255 characters");
	strcpy(defaultFontName, newName);
}

/*
preference: DefaultTextSize
*/
int PsychPrefStateGet_DefaultTextSize(void)
{
	return(defaultTextSize);
}

void PsychPrefStateSet_DefaultTextSize(int textSize)
{
	defaultTextSize=textSize;
}

/*
preference: DefaultTextStyle
*/
int PsychPrefStateGet_DefaultTextStyle(void)
{
	return(defaultTextStyle);
}

void PsychPrefStateSet_DefaultTextStyle(int textStyle)
{
	defaultTextStyle=textStyle;
}

/*
preference: TextAlphaBlending
*/
Boolean PsychPrefStateGet_TextAlphaBlending(void)
{
	return(textAlphaBlending);
}

void PsychPrefStateSet_TextAlphaBlending(Boolean enableFlag)
{
	textAlphaBlending=enableFlag;
}

// Screen self-test and calibration preferences for syncing to VBL and such...  
Boolean PsychPrefStateGet_SkipSyncTests(void)
{
    return(screenSkipSyncTests);
}

void PsychPrefStateSet_SkipSyncTests(Boolean setFlag)
{
    screenSkipSyncTests = setFlag;
}

// Screen debug level for output of visual feedback signals...  
int PsychPrefStateGet_VisualDebugLevel(void)
{
    return(screenVisualDebugLevel);
}

void PsychPrefStateSet_VisualDebugLevel(int level)
{
    screenVisualDebugLevel=level;
}

// Settings for conserving VRAM usage by disabling certain features.
// Current constants (defined in ScreenTypes.h) that can be or'ed together:
// kPsychDisableAUXBuffers = Don't use AUX buffers, disable associated features.
// kPsychDontCacheTextures = Use Client storage for RAM->DMA texturing instead of VRAM texturing.
// Defaults to zero = Make full use of VRAM.
int PsychPrefStateGet_ConserveVRAM(void)
{
    return(screenConserveVRAM);
}

void PsychPrefStateSet_ConserveVRAM(int level)
{
    screenConserveVRAM = level;
}

// If EmulateOldPTB is set to true (default is false) at startup, then we try to
// behave like the old OS-9 and Windows Psychtoolboxes. We don't use double-buffering
// and SCREEN('Flip') for stimulus onset- offset and timing, but good'ol
// Screen('WaitBlanking') in conjunction with drawing commands that execute
// immediately on invocation (glFinish()) and a single-buffered onscreen window.
// Offscreen windows are also available in a compatible way.
//
// Note: Internally we still use a double-buffered context, but the front buffer is
// the drawing/reading target for all commands and the backbuffer is used as a
// scratchpad buffer for Offscreen window handling.
Boolean PsychPrefStateGet_EmulateOldPTB(void)
{
    return(EmulateOldPTB);
}

void PsychPrefStateSet_EmulateOldPTB(Boolean level)
{
    EmulateOldPTB = level;
}

// Enable switch for 3D graphics support. If set to true, PTB will allocate stencil-
// and depth-buffers additionally to the AUX and Colorbuffers and perform additional
// bookkeeping to make sure we can do real 3D rendering and interface to external
// OpenGL mexfiles like, e.g., moglcore...
Boolean PsychPrefStateGet_3DGfx(void)
{
    return(Enable_3d_gfx);
}

void PsychPrefStateSet_3DGfx(Boolean level)
{
    Enable_3d_gfx = level;
}

//****************************************************************************************************************
//Debug preferences

Boolean PsychPrefStateGet_DebugMakeTexture(void)
{
	return(TimeMakeTextureFlag);
}

void PsychPrefStateSet_DebugMakeTexture(Boolean setFlag)
{
	TimeMakeTextureFlag=setFlag;
}





