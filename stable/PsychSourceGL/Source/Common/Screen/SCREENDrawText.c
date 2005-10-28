/*
    SCREENDrawText.c	
  
    AUTHORS:
    
		Allen.Ingling@nyu.edu		awi 
  
    PLATFORMS:
		
		Only OS X for now.
    
    HISTORY:
	
		11/17/03	awi		Spun off from SCREENTestTexture which also used Quartz and Textures to draw text but did not match the 'DrawText' specifications.
		10/12/04	awi		In useString: changed "SCREEN" to "Screen", and moved commas to inside [].
		2/25/05		awi		Added call to PsychUpdateAlphaBlendingFactorLazily().  Drawing now obeys settings by Screen('BlendFunction').
                5/08/05         mk              Bugfix for "Descenders of letters get cut/eaten away" bug introduced in PTB 1.0.5
                10/12/05        mk              Fix crash in DrawText caused by removing glFinish() while CLIENT_STORAGE is enabled!
                                                -> Disabling CLIENT_STORAGE and removing glFinish() is the proper solution...
 
    DESCRIPTION:
  
    REFERENCES:
	
		http://oss.sgi.com/projects/ogl-sample/registry/APPLE/client_storage.txt
		http://developer.apple.com/samplecode/Sample_Code/Graphics_3D/TextureRange.htm
	
  
    TO DO:
  
		- Set the alpha channel value in forground and background text correctly so that we only overwrite the portions of the target window where the text goes.
	
		- If we fail to set the font before calling this function we crash.  Fix that. 
	
		- Drawing White text works but othewise the colors don't seem to map onto the componenets correctlty.  Need to fix that.
	
		- By the way, there is something wrong with FillRect and the alpha channel does not clear the screen.
		
		- Accept 16-bit characters
	
		And remember:
	
		Destroy the shadow window after we are done with it. 
	
		Fix the color bug  See RBGA not in code below. 
		
		 
		UPDATE 3/9/05
		
		quick fix:		-Make the texture the nearest power of two which encloses the window, 
						-Set source alpha to 1 and target to 1-source alpha.
						-Use a rectangular subtexture of the full texture
		
		more complicated fix: 
		
						-Use size of bounding text box to determine texture size.  
						-(Optionally isolate rotine for bounding box to match text width of OS 9)
						-Obey alpha blending rules.  This makes sense when we only overwrite with a bounding box.
							
*/


#include "Screen.h"

#define CHAR_TO_UNICODE_LENGTH_FACTOR		4			//Apple recommends 3 or 4 even though 2 makes more sense.
#define USE_ATSU_TEXT_RENDER				1

// If you change useString then also change the corresponding synopsis string in ScreenSynopsis.
static char useString[] = "[newX,newY]=Screen('DrawText', windowPtr, text [,x] [,y] [,color] [,backgroundColor]);";
//                                                        1          2      3    4    5        6       
static char synopsisString[] = 
    "Draw text. \"text\" may include two-byte (16 bit) characters (e.g. Chinese). "
    "Default \"x\" \"y\" is current pen location. \"color\" is the CLUT index (scalar or [r "
    "g b] triplet) that you want to poke into each pixel; default produces black with "
    "the standard CLUT for this window's pixelSize. \"newX, newY\" return the final pen "
    "location.";
static char seeAlsoString[] = "";


//Specify arguments to glTexImage2D when creating a texture to be held in client RAM. The choices are dictated  by our use of Apple's 
//GL_UNPACK_CLIENT_STORAGE_APPLE constant, an argument to glPixelStorei() which specifies the packing format of pixel the data passed to 
//glTexImage2D().  
#define texImage_target				GL_TEXTURE_2D
#define texImage_level				0
#define	texImage_internalFormat		GL_RGBA
#define texImage_sourceFormat		GL_BGRA
#define texImage_sourceType			GL_UNSIGNED_INT_8_8_8_8_REV

//Specify arguments to CGBitmapContextCreate() when creating a CG context which matches the pixel packing of the texture stored in client memory.
//The choice of values is dictated by our use arguments to    
#define cg_RGBA_32_BitsPerPixel		32
#define cg_RGBA_32_BitsPerComponent	8

PsychError SCREENDrawText(void) 
{
    PsychWindowRecordType 	*winRec;
	double				invertedY;
    CGContextRef		cgContext;
    unsigned int		memoryTotalSizeBytes, memoryRowSizeBytes;
    UInt32				*textureMemory;
    GLuint				myTexture;
    CGColorSpaceRef		cgColorSpace;
	PsychRectType		windowRect;
//	double				textureSizeX, textureSizeY;
	char				*textString;
	Boolean				doSetColor, doSetBackgroundColor;
	PsychColorType			colorArg, backgroundColorArg;
//	CGImageAlphaInfo	quartzAlphaMode, correctQuartzAlphaMode, correctQuartzAlphaModeMaybe;
	CGRect				quartzRect;
	GLdouble			backgroundColorVector[4];
	//for creating the layout object and setting the run attributes.  (we can get rid of these after we unify parts of DrawText and TextBounds).
	char					*textCString;
	int						stringLengthChars;
	Str255					textPString;
	int						uniCharBufferLengthElements, uniCharBufferLengthChars, uniCharBufferLengthBytes;
	UniChar					*textUniString;
	TextEncoding			textEncoding;
	OSStatus				callError;
	TextToUnicodeInfo		textToUnicodeInfo;
	ByteCount				uniCharStringLengthBytes;
	ATSUStyle				atsuStyle;
	ATSUTextLayout			textLayout;				//layout is a pointer to an opaque struct.
	
	Rect					textBoundsQRect;
	double					textBoundsPRect[4], textBoundsPRectOrigin[4], textureRect[4];
	double					textureWidth, textureHeight, textHeight, textWidth, textureTextFractionY, textureTextFractionXLeft,textureTextFractionXRight;
	double					quadLeft, quadRight, quadTop, quadBottom;
	int						psychColorSize;
	GLenum					normalSourceBlendFactor, normalDestinationBlendFactor;
	
	//for layout attributes.  (not the same as run style attributes set by PsychSetATSUTStyleAttributes or line attributes which we do not set.) 	
	ATSUAttributeTag		saTags[] =  {kATSUCGContextTag };
	ByteCount				saSizes[] = {sizeof(CGContextRef)};
	ATSUAttributeValuePtr   saValue[] = {&cgContext};
    
    //all subfunctions should have these two lines.  
    PsychPushHelp(useString, synopsisString, seeAlsoString);
    if(PsychIsGiveHelp()){PsychGiveHelp();return(PsychError_none);};
    
    //Get the window structure for the onscreen window.  It holds the onscreein GL context which we will need in the
    //final step when we copy the texture from system RAM onto the screen.
    PsychErrorExit(PsychCapNumInputArgs(6));   	
    PsychErrorExit(PsychRequireNumInputArgs(2)); 	
    PsychErrorExit(PsychCapNumOutputArgs(1));  
    PsychAllocInWindowRecordArg(1, TRUE, &winRec);
//    if(!PsychIsOnscreenWindow(winRec))
//        PsychErrorExitMsg(PsychError_user, "Onscreen window pointer required");
        
	//Get the dimensions of the target window
	PsychGetRectFromWindowRecord(windowRect, winRec);
//	textureSizeX=PsychGetWidthFromRect(windowRect);
//	textureSizeY=PsychGetHeightFromRect(windowRect);


	
	//Get the text string (it is required)
	PsychAllocInCharArg(2, kPsychArgRequired, &textString);
	
	//Get the X and Y positions.
	PsychCopyInDoubleArg(3, kPsychArgOptional, &(winRec->textAttributes.textPositionX));
	PsychCopyInDoubleArg(4, kPsychArgOptional, &(winRec->textAttributes.textPositionY));
	
	//Get the color
    //Get the new color record, coerce it to the correct mode, and store it.  
    doSetColor=PsychCopyInColorArg(5, kPsychArgOptional, &colorArg);
	if(doSetColor)
		PsychSetTextColorInWindowRecord(&colorArg,  winRec);
		
    doSetBackgroundColor=PsychCopyInColorArg(6, kPsychArgOptional, &backgroundColorArg);
	if(doSetBackgroundColor)
		PsychSetTextBackgroundColorInWindowRecord(&backgroundColorArg,  winRec);
	
		

		
	/////////////common to TextBounds and DrawText:create the layout object////////////////// 
	//read in the string and get its length and convert it to a unicode string.
	PsychAllocInCharArg(2, kPsychArgRequired, &textCString);
	stringLengthChars=strlen(textCString);
	if(stringLengthChars > 255)
		PsychErrorExitMsg(PsychError_unimplemented, "Cut corners and TextBounds will not accept a string longer than 255 characters");
	CopyCStringToPascal(textCString, textPString);
	uniCharBufferLengthChars= stringLengthChars * CHAR_TO_UNICODE_LENGTH_FACTOR;
	uniCharBufferLengthElements= uniCharBufferLengthChars + 1;		
	uniCharBufferLengthBytes= sizeof(UniChar) * uniCharBufferLengthElements;
	textUniString=(UniChar*)malloc(uniCharBufferLengthBytes);
	//Using a TextEncoding type describe the encoding of the text to be converteed.  
	textEncoding=CreateTextEncoding(kTextEncodingMacRoman, kMacRomanDefaultVariant, kTextEncodingDefaultFormat);
	//Create a structure holding conversion information from the text encoding type we just created.
	callError=CreateTextToUnicodeInfoByEncoding(textEncoding,&textToUnicodeInfo);
	//Convert the text to a unicode string
	callError=ConvertFromPStringToUnicode(textToUnicodeInfo, textPString, (ByteCount)uniCharBufferLengthBytes,	&uniCharStringLengthBytes,	textUniString);
	//create the text layout object
	callError=ATSUCreateTextLayout(&textLayout);			
	//associate our unicode text string with the text layout object
	callError=ATSUSetTextPointerLocation(textLayout, textUniString, kATSUFromTextBeginning, kATSUToTextEnd, (UniCharCount)stringLengthChars);
	//create an ATSU style object and tie it to the layout object in a style run.
	callError=ATSUCreateStyle(&atsuStyle);
	callError=ATSUClearStyle(atsuStyle);
	PsychSetATSUStyleAttributesFromPsychWindowRecord(atsuStyle, winRec);
	callError=ATSUSetRunStyle(textLayout, atsuStyle, (UniCharArrayOffset)0, (UniCharCount)stringLengthChars);
	/////////////end common to TextBounds and DrawText//////////////////
	
	//*********
	
	//Get the bounds for our text so that and create a texture of sufficient size to containt it. 
//	callError=ATSUMeasureTextImage(textLayout,  kATSUFromTextBeginning, kATSUToTextEnd, (ATSUTextMeasurement)0, (ATSUTextMeasurement)0, &textBoundsQRect);
	ATSUTextMeasurement mleft, mright, mtop, mbottom;
        callError=ATSUGetUnjustifiedBounds(textLayout,  kATSUFromTextBeginning, kATSUToTextEnd, &mleft, &mright, &mbottom, &mtop);
	
        textBoundsPRect[kPsychLeft]=Fix2X(mleft);
	textBoundsPRect[kPsychTop]=0;
	textBoundsPRect[kPsychRight]=Fix2X(mright);
	textBoundsPRect[kPsychBottom]=(fabs(Fix2X(mbottom)) + fabs(Fix2X(mtop)));
        //printf("Top %lf x Bottom %lf :: ",textBoundsPRect[kPsychTop], textBoundsPRect[kPsychBottom]); 
	PsychNormalizeRect(textBoundsPRect, textBoundsPRectOrigin);
	textWidth=PsychGetWidthFromRect(textBoundsPRectOrigin);
	textHeight=PsychGetHeightFromRect(textBoundsPRectOrigin);
	PsychFindEnclosingTextureRect(textBoundsPRectOrigin, textureRect);
	
	//Allocate memory the size of the texture.  The CG context is the same size.  It could be smaller, because Core Graphics surfaces don't have the power-of-two
	//constraint requirement.   
	textureWidth=PsychGetWidthFromRect(textureRect);
	textureHeight=PsychGetHeightFromRect(textureRect);
        memoryRowSizeBytes=sizeof(UInt32) * textureWidth;
    memoryTotalSizeBytes= memoryRowSizeBytes * textureHeight;
    textureMemory=(UInt32 *)valloc(memoryTotalSizeBytes);
    if(!textureMemory)
            PsychErrorExitMsg(PsychError_internal, "Failed to allocate surface memory\n");
			
	//Create the Core Graphics bitmap graphics context.  We can tell CoreGraphics to use the same memory storage format as will our GL texture, and in fact use
	//  the idential memory for both.   
	cgColorSpace=CGColorSpaceCreateDeviceRGB();
	//there is a bug here.  the format constant should be ARGB not RBGA to agree with the texture format.   
	CGSize cgsize;
        cgsize.width = PsychGetWidthFromRect(winRec->rect);
        cgsize.height = PsychGetHeightFromRect(winRec->rect);
        
        cgContext= CGBitmapContextCreate(textureMemory, textureWidth, textureHeight, 8, memoryRowSizeBytes, cgColorSpace, kCGImageAlphaPremultipliedFirst);
	if(!cgContext){
		free((void *)textureMemory);
		PsychErrorExitMsg(PsychError_internal, "Failed to allocate CG Bimap Context\n");
	}
	CGContextSetFillColorSpace (cgContext,cgColorSpace);

	//check that we are in the correct alpha mode (for the debugger... should change this to exit with error)
	//correctQuartzAlphaMode= kCGImageAlphaPremultipliedLast;
	//correctQuartzAlphaModeMaybe=kCGImageAlphaPremultipliedFirst;
	//quartzAlphaMode=CGBitmapContextGetAlphaInfo(cgContext);

	//fill in the text background.  It's stored in the Window record in PsychColor format.  We convert it to an OpenGL color vector then into a quartz 
	// vector	
	quartzRect.origin.x=(float)0;
	quartzRect.origin.y=(float)0;
	quartzRect.size.width=(float)textureWidth;
	quartzRect.size.height=(float)textureHeight;
	psychColorSize=PsychGetColorSizeFromWindowRecord(winRec);
	PsychCoerceColorModeWithDepthValue(kPsychRGBAColor, psychColorSize, &(winRec->textAttributes.textBackgroundColor));
	PsychConvertColorAndColorSizeToDoubleVector(&(winRec->textAttributes.textBackgroundColor), psychColorSize, backgroundColorVector);
	//by default override the background alpha value, setting alpha transparecy. Only when DrawText obeys the alpha blend function setting do we need or want the the alpha argument for the background.
	if(!PsychPrefStateGet_TextAlphaBlending())
		backgroundColorVector[3]=0;

	CGContextSetRGBFillColor(cgContext, (float)(backgroundColorVector[0]), (float)(backgroundColorVector[1]), (float)(backgroundColorVector[2]), (float)(backgroundColorVector[3])); 
	CGContextFillRect(cgContext, quartzRect);
	
    //now draw the text and close up the CoreGraphics shop before we proceed to textures.
	//associate the core graphics context with text layout object holding our unicode string.
	//callError=QDBeginCGContext (CGrafPtr inPort, &cgContext);
	callError=ATSUSetLayoutControls (textLayout, 1, saTags, saSizes, saValue);
	//CGContextRotateCTM (myCGContext, myAngle);
	invertedY=PsychGetHeightFromRect(winRec->rect) - winRec->textAttributes.textPositionY;
	ATSUDrawText(textLayout, kATSUFromTextBeginning, kATSUToTextEnd, Long2Fix((long)0), Long2Fix((long)(textureHeight-textHeight)));
	CGContextFlush(cgContext); 	//this might not be necessary but do it just in case.
	//free  stuff
	free((void*)textUniString);
	callError=ATSUDisposeStyle(atsuStyle);
	//TO DO: we need to get the new text location and put it back into the window record's X and Y locations.
	//Remove references from Core Graphics to the texture memory.  CG and OpenGL can share concurrently, but we don't won't need this anymore.
	CGColorSpaceRelease (cgColorSpace);
	CGContextRelease(cgContext);	

    //Convert the CG graphics bitmap into a GL texture.  
    PsychSetGLContext(winRec);
	if(!PsychPrefStateGet_TextAlphaBlending()){
		PsychGetAlphaBlendingFactorsFromWindow(winRec, &normalSourceBlendFactor, &normalDestinationBlendFactor);
		PsychStoreAlphaBlendingFactorsForWindow(winRec, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	PsychUpdateAlphaBlendingFactorLazily(winRec);
	
        // Explicitely disable Apple's Client storage extensions. For now they are not really useful to us.
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
        
	glDisable(GL_TEXTURE_RECTANGLE_EXT);
	glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &myTexture);				//create an index "name" for our texture
    glBindTexture(GL_TEXTURE_2D, myTexture);	//instantiate a texture of type associated with the index and set it to be the target for subsequent gl texture operators.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);		//tell gl how to unpack from our memory when creating a surface, namely don't really unpack it but use it for texture storage.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	//specify interpolation scaling rule for copying from texture.  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  //specify interpolation scaling rule from copying from texture.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	PsychTestForGLErrors();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  (GLsizei)textureWidth, (GLsizei)textureHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, textureMemory);
	PsychTestForGLErrors();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBindTexture(GL_TEXTURE_2D, myTexture);			//instantiate a texture of type associated with the index and set it to be the target for subsequent gl texture operators.

   // printf("%f x %f - %f:: ", textureWidth, textureHeight, textHeight);
   // printf("%i x %i :: ", (int) textureWidth, (int) textureHeight);
    
	//The texture holding the text is >=  the bounding rect for the text because of the requirement for textures that they have sides 2^(integer)x.  
	//Therefore we texture only from the region of the texture which contains the text, not the entire texture.  Therefore the rect which we texture is the dimensions
	//of the text, not the dimensions of the texture.
	
	//texture locations
	textureTextFractionXLeft=Fix2X(mleft)/textureWidth;
	textureTextFractionXRight=(Fix2X(mleft) + textWidth)/ textureWidth;
	// MK-changed: Use textBoundsQRect.bottom + textHeight instead of textHeight, so descenders of letters don't get cut away...
        textureTextFractionY=(textBoundsPRect[kPsychBottom] + textHeight)/textureHeight; 
	//textured quad positions
	quadLeft=winRec->textAttributes.textPositionX;
	quadRight=winRec->textAttributes.textPositionX + textWidth;
	quadTop=winRec->textAttributes.textPositionY;
	quadBottom=winRec->textAttributes.textPositionY + textHeight;
	 
	glBegin(GL_QUADS);
         glTexCoord2d(textureTextFractionXLeft, 0.0);											glVertex2d(quadLeft, quadTop);
         glTexCoord2d(textureTextFractionXRight, 0.0 );											glVertex2d(quadRight, quadTop);
         glTexCoord2d(textureTextFractionXRight, textureTextFractionY);							glVertex2d(quadRight, quadBottom);
         glTexCoord2d(textureTextFractionXLeft, textureTextFractionY);							glVertex2d(quadLeft, quadBottom);
         glEnd();
	
	PsychTestForGLErrors();
	
	if(!PsychPrefStateGet_TextAlphaBlending())
		PsychStoreAlphaBlendingFactorsForWindow(winRec, normalSourceBlendFactor, normalDestinationBlendFactor);
	//we do not call PsychUpdateAlphaBlendingFactorLazily() here becuase it is always the responsibility of drawing function to do that before drawing.

	
    //Close  up shop.  Unlike with normal textures is important to release the context before deallocating the memory which glTexImage2D() was given. 
    //First release the GL context, then the CG context, then free the memory.
	glDeleteTextures(1, &myTexture);	//Remove references from gl to the texture memory  & free gl's associated resources 
    // MK-changed: glFinish();
    // We don't use glFinish() here because it degrades performance. Instead we disable Apple client storage
    // (see call glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE); above)). This way we can free() the
    // textureMemory without need for glFinish()...
    free((void *)textureMemory);	//Free the memory

    return(PsychError_none);
}

