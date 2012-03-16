/*
	PsychtoolboxGL/Source/Common/PsychHID/PsychHIDKbQueueCreate.c		
  
	PROJECTS: 
	
		PsychHID only.
  
	PLATFORMS:  
	
		All.  
  
	AUTHORS:
	
		rwoods@ucla.edu		rpw 
        mario.kleiner@tuebingen.mpg.de      mk
      
	HISTORY:
		8/19/07  rpw		Created.
		8/23/07  rpw        Added PsychHIDQueueFlush to documentation
		12/17/09 rpw		Added support for keypads
  
	NOTES:
	
		The routines PsychHIDKbQueueCreate, PsychHIDKbQueueStart, PsychHIDKbQueueCheck, PsychHIDKbQueueStop
		and PsychHIDKbQueueRelease comprise a replacement for PsychHIDKbCheck, providing the following
		advantages:
		
			1) Brief key presses that would be missed by PsychHIDKbCheck are reliably detected
			2) The times of key presses are recorded more accurately
			3) Key releases are also recorded
		
		Requires Mac OS X 10.3 or later. The Matlab wrapper functions (KbQueueCreate, KbQueueStart,
		KbQueueCheck, KbQueueStop and KbQueueRelease screen away Mac OS X 10.2 and earlier, and the C code 
		does nothing to verify the Mac OS X version.
		
		Only a single device can be monitored at any given time. The deviceNumber can be specified only
		in the call to PsychHIDKbQueueCreate. The other routines then relate to that specified device. If
		deviceNumber is not specified, the first device is the default (like PyschHIDKbCheck). If
		PsychHIDKbQueueCreate has not been called first, the other routines will generate an error 
		message. Likewise, if PsychHIDKbQueueRelease has been called more recently than PsychHIDKbQueueCreate,
		the other routines will generate error messages.
		
		It is acceptable to cal PsychHIDKbQueueCreate at any time (e.g., to switch to a new device) without
		calling PsychKbQueueRelease.
		
		PsychHIDKbQueueCreate:
			Creates the queue for the specified (or default) device number
			No events are delivered to the queue until PsychHIDKbQueueStart is called
			Can be called again at any time
			
		PsychHIDKbQueueStart:
			Starts delivering keyboard or keypad events from the specified device to the queue
			
		PsychHIDKbQueueStop:
			Stops delivery of new keyboard or keypad events from the specified device to the queue.
			Data regarding events already queued is not cleared and can be recovered by PsychHIDKbQueueCheck
			
		PsychHIDKbQueueCheck:
			Obtains data about keypresses on the specified device since the most recent call to
			this routine or to PsychHIDKbQueueStart
			
			Clears all currently scored events (unscored events may still be in the queue)
			
		PsychHIDKbQueueFlush:
			Flushes unscored events from the queue and zeros all previously scored events
			
		PsychHIDKbQueueRelease:
			Releases queue-associated resources; once called, PsychHIDKbQueueCreate must be invoked
			before using any of the other routines
			
			This routine is called automatically at clean-up and can be omitted at the potential expense of
			keeping memory allocated unnecesarily
				

		---

*/

#include "PsychHID.h"

static char useString[]= "PsychHID('KbQueueCreate', [deviceNumber], [keyFlags])";
static char synopsisString[] = 
        "Creates a queue for events generated by an input device (keyboard, keypad, mouse, ...).\n"
        "By default the first keyboard device (the one with the lowest device number) is "
        "used. If no keyboard is found, the first keypad device is used, followed by other "
        "devices, e.g., mice.  Optionally, the deviceNumber of any keyboard or HID device may be specified.\n"
        "On OS/X only one input device queue is allowed at a time.\n"
        "On MS-Windows XP and later, it is currently not possible to enumerate different keyboards and mice "
        "separately. Therefore the 'deviceNumber' argument is mostly useless for keyboards and mice. Usually you can "
        "only check the system keyboard or mouse.\n";

static char seeAlsoString[] = "KbQueueStart, KbQueueStop, KbQueueCheck, KbQueueFlush, KbQueueRelease";

PsychError PSYCHHIDKbQueueCreate(void) 
{
	int deviceIndex = -1;
	int numScankeys = 0;
	int* scanKeys = NULL;

	PsychPushHelp(useString, synopsisString, seeAlsoString);
	if(PsychIsGiveHelp()){PsychGiveHelp();return(PsychError_none);};

	PsychErrorExit(PsychCapNumInputArgs(2)); // Optionally specifies the deviceNumber of the keyboard or keypad to scan and an array of keyCode indicators.

	// Get optional deviceIndex:
	PsychCopyInIntegerArg(1, FALSE, &deviceIndex);

	// Get optional scanKeys vector:
	PsychAllocInIntegerListArg(2, FALSE, &numScankeys, &scanKeys);

	// Perform actual, OS-dependent init and return its status code:
	return(PsychHIDOSKbQueueCreate(deviceIndex, numScankeys, scanKeys));
}


#if PSYCH_SYSTEM == PSYCH_OSX

#include "PsychHIDKbQueue.h"
#include <errno.h>

#define NUMDEVICEUSAGES 7

// Declare globally scoped variables that will be declared extern by other functions in this family
AbsoluteTime *psychHIDKbQueueFirstPress=NULL;
AbsoluteTime *psychHIDKbQueueFirstRelease=NULL;
AbsoluteTime *psychHIDKbQueueLastPress=NULL;
AbsoluteTime *psychHIDKbQueueLastRelease=NULL;
HIDDataRef hidDataRef=NULL;
pthread_mutex_t psychHIDKbQueueMutex=PTHREAD_MUTEX_INITIALIZER;
CFRunLoopRef psychHIDKbQueueCFRunLoopRef=NULL;
pthread_t psychHIDKbQueueThread = NULL;

static void *PsychHIDKbQueueNewThread(void *value){
	// The new thread is started after the global variables are initialized

	// Get and retain the run loop associated with this thread
	psychHIDKbQueueCFRunLoopRef=(CFRunLoopRef) GetCFRunLoopFromEventLoop(GetCurrentEventLoop());
	CFRetain(psychHIDKbQueueCFRunLoopRef);
	
	// Put the event source into the run loop
	if(!CFRunLoopContainsSource(psychHIDKbQueueCFRunLoopRef, hidDataRef->eventSource, kCFRunLoopDefaultMode))
		CFRunLoopAddSource(psychHIDKbQueueCFRunLoopRef, hidDataRef->eventSource, kCFRunLoopDefaultMode);

	// Start the run loop, code execution will block here until run loop is stopped again by PsychHIDKbQueueRelease
	// Meanwhile, the run loop of this thread will be responsible for executing code below in PsychHIDKbQueueCalbackFunction
	CFRunLoopRun();
		
	// In case the CFRunLoop was interrupted while the mutex was locked, unlock it
	pthread_mutex_unlock(&psychHIDKbQueueMutex);
	
	// Destroy the mutex
	pthread_mutex_destroy(&psychHIDKbQueueMutex);
}

static void PsychHIDKbQueueCallbackFunction(void *target, IOReturn result, void *refcon, void *sender)
{
	// This routine is executed each time the queue transitions from empty to non-empty
	// The CFRunLoop of the thread in PsychHIDKbQueueNewThread is the thread that executes here
	HIDDataRef hidDataRef=(HIDDataRef)refcon;
	long keysUsage=-1;
	IOHIDEventStruct event;
	AbsoluteTime zeroTime= {0,0};
	
	result=kIOReturnError;
	if(!hidDataRef) return;	// Nothing we can do because we can't access queue, (shouldn't happen)
	
	while(1){
		// This function only gets called when queue transitions from empty to non-empty
		// Therefore, we must process all available events in this while loop before
		// it will be possible for this function to be notified again
		{
			// Get next event from queue
			result = (*hidDataRef->hidQueueInterface)->getNextEvent(hidDataRef->hidQueueInterface, &event, zeroTime, 0);
			if(kIOReturnSuccess!=result) return;
		}
		if ((event.longValueSize != 0) && (event.longValue != NULL)) free(event.longValue);
		{
			// Get element associated with event so we can get its usage page
			CFMutableDataRef element=NULL;
			{
				CFNumberRef number= CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &event.elementCookie);
				if (!number)  continue;
				element = (CFMutableDataRef)CFDictionaryGetValue(hidDataRef->hidElementDictionary, number);
				CFRelease(number);
			}
			if (!element) continue;
			{
				HIDElementRef tempHIDElement=(HIDElement *)CFDataGetMutableBytePtr(element);
				if(!tempHIDElement) continue;
				keysUsage=tempHIDElement->usage;
			}
		}
		// if(keysUsage<1 || keysUsage>255) continue;	// This is redundant since usage is checked when elements are added
		
		pthread_mutex_lock(&psychHIDKbQueueMutex);

		// Update records of first and latest key presses and releases
		if(event.value!=0){
			if(psychHIDKbQueueFirstPress){
				// First key press timestamp
				if(psychHIDKbQueueFirstPress[keysUsage-1].hi==0 && psychHIDKbQueueFirstPress[keysUsage-1].lo==0){
					psychHIDKbQueueFirstPress[keysUsage-1]=event.timestamp;
				}
			}
			if(psychHIDKbQueueLastPress){
				// Last key press timestamp
				psychHIDKbQueueLastPress[keysUsage-1]=event.timestamp;
			}
		}
		else{
			if(psychHIDKbQueueFirstRelease){
				// First key release timestamp
				if(psychHIDKbQueueFirstRelease[keysUsage-1].hi==0 && psychHIDKbQueueFirstRelease[keysUsage-1].lo==0) psychHIDKbQueueFirstRelease[keysUsage-1]=event.timestamp;
			}
			if(psychHIDKbQueueLastRelease){
				// Last key release timestamp
				psychHIDKbQueueLastRelease[keysUsage-1]=event.timestamp;
			}
		}
		pthread_mutex_unlock(&psychHIDKbQueueMutex);
	}
}

PsychError PsychHIDOSKbQueueCreate(int deviceIndex, int numScankeys, int* scanKeys)
{
	pRecDevice			deviceRecord;
	int					*psychHIDKbQueueKeyList;
	long				KbDeviceUsagePages[NUMDEVICEUSAGES]= {kHIDPage_GenericDesktop, kHIDPage_GenericDesktop, kHIDPage_GenericDesktop, kHIDPage_GenericDesktop, kHIDPage_GenericDesktop, kHIDPage_GenericDesktop, kHIDPage_GenericDesktop};
	long				KbDeviceUsages[NUMDEVICEUSAGES]={kHIDUsage_GD_Keyboard, kHIDUsage_GD_Keypad, kHIDUsage_GD_Mouse, kHIDUsage_GD_Pointer, kHIDUsage_GD_Joystick, kHIDUsage_GD_GamePad, kHIDUsage_GD_MultiAxisController};
	int					numDeviceUsages=NUMDEVICEUSAGES;
	
	HRESULT result;
	IOHIDDeviceInterface122** interface=NULL;	// This requires Mac OS X 10.3 or higher

	if(scanKeys && (numScankeys != 256)) {
		PsychErrorExitMsg(PsychError_user, "Second argument to KbQueueCreate must be a vector with 256 elements.");
	}

    psychHIDKbQueueKeyList = scanKeys;
    
	PsychHIDVerifyInit();
	
	if(psychHIDKbQueueCFRunLoopRef || hidDataRef || psychHIDKbQueueFirstPress || psychHIDKbQueueFirstRelease || psychHIDKbQueueLastPress || psychHIDKbQueueLastRelease){
		// We are reinitializing, so need to release prior initialization
		PsychHIDOSKbQueueRelease(deviceIndex);
	}
	
	// Find the requested device record
	{
		int deviceIndices[PSYCH_HID_MAX_KEYBOARD_DEVICES]; 
		pRecDevice deviceRecords[PSYCH_HID_MAX_KEYBOARD_DEVICES];
		int numDeviceIndices;
		int i;
		PsychHIDGetDeviceListByUsages(numDeviceUsages, KbDeviceUsagePages, KbDeviceUsages, &numDeviceIndices, deviceIndices, deviceRecords);  
		{
			// A negative device number causes the default device to be used:
			psych_bool isDeviceSpecified = (deviceIndex >= 0) ? TRUE : FALSE;

			if(isDeviceSpecified){
				psych_bool foundUserSpecifiedDevice;
				//make sure that the device number provided by the user is really a keyboard or keypad.
				for(i=0;i<numDeviceIndices;i++){
					if(foundUserSpecifiedDevice=(deviceIndices[i]==deviceIndex))
						break;
				}
				if(!foundUserSpecifiedDevice)
					PsychErrorExitMsg(PsychError_user, "Specified device number is not a keyboard or keypad device.");
			}
			else{ 
				// set the keyboard or keypad device to be the first keyboard device or, if no keyboard, the first keypad
				i=0;
				if(numDeviceIndices==0)
					PsychErrorExitMsg(PsychError_user, "No keyboard or keypad devices detected.");
			}
		}
		deviceRecord=deviceRecords[i]; 
	}

	// The queue key list is already assigned, if present at all, ie. non-NULL.
	// The list is a vector of 256 doubles, analogous to the outputs of KbQueueCheck
	// (or KbCheck) with each element of the vector corresponding to a particular
	// key. If the double in a particular position is zero, that key is not added
	// to the queue and events from that key will not be detected.
	// Note that psychHIDKbQueueList does not need to be freed because it is allocated
	// within PsychAllocInIntegerListArg using mxMalloc

	interface=deviceRecord->interface;
	if(!interface)
		PsychErrorExitMsg(PsychError_system, "Could not get interface to device.");
		
	hidDataRef=malloc(sizeof(HIDData));
	if(!hidDataRef)
		PsychErrorExitMsg(PsychError_system, "Could not allocate memory for queue.");
	bzero(hidDataRef, sizeof(HIDData));
	
	// Allocate for the queue
	hidDataRef->hidQueueInterface=(*interface)->allocQueue(interface);
	if(!hidDataRef->hidQueueInterface){
		free(hidDataRef);
		hidDataRef=NULL;
		PsychErrorExitMsg(PsychError_system, "Failed to allocate event queue for detecting key press.");
	}
	hidDataRef->hidDeviceInterface=interface;
	
	// Create the queue
	result = (*hidDataRef->hidQueueInterface)->create(hidDataRef->hidQueueInterface, 0, 30);	// The second number is the number of events can be stored before events are lost
												// Empirically, the lost events are the later events, despite my having seen claims to the contrary
												// Also, empirically, I get 11 events when specifying 8 ???
	if (kIOReturnSuccess != result){
		(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
		free(hidDataRef);
		hidDataRef=NULL;
		PsychErrorExitMsg(PsychError_system, "Failed to create event queue for detecting key press.");
	}
	{
		// Prepare dictionary then add appropriate device elements to dictionary and queue
		CFArrayRef elements;
		CFMutableDictionaryRef hidElements = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		if(!hidElements){
			(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
			(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
			free(hidDataRef);
			hidDataRef=NULL;
			PsychErrorExitMsg(PsychError_system, "Failed to create Dictionary for queue.");
		}
		{
			// Get a listing of all elements associated with the device
			// copyMatchinfElements requires IOHIDDeviceInterface122, thus Mac OS X 10.3 or higher
			// elements would have to be obtained directly from IORegistry for 10.2 or earlier
			IOReturn success=(*interface)->copyMatchingElements(interface, NULL, &elements);
			
			if(!elements){
				CFRelease(hidElements);
				(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
				(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
				free(hidDataRef);
				hidDataRef=NULL;
				PsychErrorExitMsg(PsychError_user, "No elements found on device.");
			}
		}
		{
			// Put all appropriate elements into the dictionary and into the queue
			HIDElement newElement;
			CFIndex i;
			for (i=0; i<CFArrayGetCount(elements); i++)
			{
				CFNumberRef number;
				CFDictionaryRef element= CFArrayGetValueAtIndex(elements, i);
				CFTypeRef object;
				
				if(!element) continue;
				bzero(&newElement, sizeof(HIDElement));
				//newElement.owner=hidDataRef;
				
				// Get usage page and make sure it is a keyboard or keypad or something with buttons.
				number = (CFNumberRef)CFDictionaryGetValue(element, CFSTR(kIOHIDElementUsagePageKey));
				if (!number) continue;
				CFNumberGetValue(number, kCFNumberSInt32Type, &newElement.usagePage );
				if((newElement.usagePage != kHIDPage_KeyboardOrKeypad) && (newElement.usagePage != kHIDPage_Button)) continue;

				// Get usage and make sure it is in range 1-256
				number = (CFNumberRef)CFDictionaryGetValue(element, CFSTR(kIOHIDElementUsageKey));
				if (!number) continue;
				CFNumberGetValue(number, kCFNumberSInt32Type, &newElement.usage );
				if(newElement.usage<1 || newElement.usage>256) continue;
				
				// Verify that it is on the queue key list (if specified).
				// Zero value indicates that key corresponding to position should be ignored
				if(psychHIDKbQueueKeyList){
					if(psychHIDKbQueueKeyList[newElement.usage-1]==0) continue;
				}
				
				// Get cookie
				number = (CFNumberRef)CFDictionaryGetValue(element, CFSTR(kIOHIDElementCookieKey));
				if (!number) continue;
				CFNumberGetValue(number, kCFNumberIntType, &(newElement.cookie) );
								
				{
					// Put this element into the hidElements Dictionary
					CFMutableDataRef newData = CFDataCreateMutable(kCFAllocatorDefault, sizeof(HIDElement));
					if (!newData) continue;
					bcopy(&newElement, CFDataGetMutableBytePtr(newData), sizeof(HIDElement));
						  
					number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &newElement.cookie);        
					if (!number) continue;
					CFDictionarySetValue(hidElements, number, newData);
					CFRelease(number);
					CFRelease(newData);
				}
				
				// Put the element cookie into the queue
				result = (*hidDataRef->hidQueueInterface)->addElement(hidDataRef->hidQueueInterface, newElement.cookie, 0);
				if (kIOReturnSuccess != result) continue;
				
				/*
				// Get element type (it should always be selector, but just in case it isn't and this proves pertinent...
				number = (CFNumberRef)CFDictionaryGetValue(element, CFSTR(kIOHIDElementTypeKey));
				if (!number) continue;
				CFNumberGetValue(number, kCFNumberIntType, &(newElement.type) );
				*/
			}
			CFRelease(elements);
		}
		// Make sure that the queue and dictionary aren't empty
		if (CFDictionaryGetCount(hidElements) == 0){
			CFRelease(hidElements);
			(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
			(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
			free(hidDataRef);
			hidDataRef=NULL;
			PsychErrorExitMsg(PsychError_system, "Failed to get any appropriate elements from the device.");
		}
		else{
			hidDataRef->hidElementDictionary = hidElements;
		}
	}

	hidDataRef->eventSource=NULL;
	result = (*hidDataRef->hidQueueInterface)->createAsyncEventSource(hidDataRef->hidQueueInterface, &(hidDataRef->eventSource));
	if (kIOReturnSuccess!=result)
	{
		CFRelease(hidDataRef->hidElementDictionary);
		(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
		(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
		free(hidDataRef);
		hidDataRef=NULL;
		PsychErrorExitMsg(PsychError_system, "Failed to get event source");
	}

	// Allocate memory for tracking keypresses
	psychHIDKbQueueFirstPress=malloc(256*sizeof(AbsoluteTime));
	psychHIDKbQueueFirstRelease=malloc(256*sizeof(AbsoluteTime));
	psychHIDKbQueueLastPress=malloc(256*sizeof(AbsoluteTime));
	psychHIDKbQueueLastRelease=malloc(256*sizeof(AbsoluteTime));
	
	if(!psychHIDKbQueueFirstPress || !psychHIDKbQueueFirstRelease || !psychHIDKbQueueLastPress || !psychHIDKbQueueLastRelease){
		CFRelease(hidDataRef->hidElementDictionary);
		(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
		(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
		free(hidDataRef);
		hidDataRef=NULL;
		PsychErrorExitMsg(PsychError_system, "Failed to allocate memory for tracking keypresses");
	}
	
	// Zero memory for tracking keypresses
	{
		int i;
		for(i=0;i<256;i++){
			psychHIDKbQueueFirstPress[i].hi=0;
			psychHIDKbQueueFirstPress[i].lo=0;
			
			psychHIDKbQueueFirstRelease[i].hi=0;
			psychHIDKbQueueFirstRelease[i].lo=0;
			
			psychHIDKbQueueLastPress[i].hi=0;
			psychHIDKbQueueLastPress[i].lo=0;
			
			psychHIDKbQueueLastRelease[i].hi=0;
			psychHIDKbQueueLastRelease[i].lo=0;
		}
	}

	{
		IOHIDCallbackFunction function=PsychHIDKbQueueCallbackFunction;
		result= (*hidDataRef->hidQueueInterface)->setEventCallout(hidDataRef->hidQueueInterface, function, NULL, hidDataRef);
		if (kIOReturnSuccess!=result)
		{
			free(psychHIDKbQueueFirstPress);
			free(psychHIDKbQueueFirstRelease);
			free(psychHIDKbQueueLastPress);
			free(psychHIDKbQueueLastRelease);
			CFRelease(hidDataRef->hidElementDictionary);
			(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
			(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
			free(hidDataRef);
			hidDataRef=NULL;
			PsychErrorExitMsg(PsychError_system, "Failed to add callout to queue");
		}
	}
	// Initializing an already initialized mutex has undefined results
	// Use pthread_mutex_trylock to determine whether initialization is needed
	{
		int returnCode;
		errno=0;
		returnCode=pthread_mutex_trylock(&psychHIDKbQueueMutex);
		if(returnCode){
			if(EINVAL==errno){
				// Mutex is invalid, so initialize it
				returnCode=pthread_mutex_init(&psychHIDKbQueueMutex, NULL);
				if(returnCode!=0){
				
					free(psychHIDKbQueueFirstPress);
					free(psychHIDKbQueueFirstRelease);
					free(psychHIDKbQueueLastPress);
					free(psychHIDKbQueueLastRelease);
					CFRelease(hidDataRef->hidElementDictionary);
					(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
					(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
					free(hidDataRef);
					hidDataRef=NULL;
					PsychErrorExitMsg(PsychError_system, "Failed to create mutex");
				}
			}
			else if(EBUSY==errno){
				// Mutex is already locked--not expected, but perhaps recoverable
				errno=0;
				returnCode=pthread_mutex_unlock(&psychHIDKbQueueMutex);
				if(returnCode!=0){
					if(EPERM==errno){
						// Some other thread holds the lock--not recoverable
						free(psychHIDKbQueueFirstPress);
						free(psychHIDKbQueueFirstRelease);
						free(psychHIDKbQueueLastPress);
						free(psychHIDKbQueueLastRelease);
						CFRelease(hidDataRef->hidElementDictionary);
						(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
						(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
						free(hidDataRef);
						hidDataRef=NULL;
						PsychErrorExitMsg(PsychError_system, "Another thread holds the lock on the mutex");
					}
				}
			}
		}
		else{
			// Attempt to lock was successful, so unlock
			pthread_mutex_unlock(&psychHIDKbQueueMutex);
		}
	}
	{
		int returnCode=pthread_create(&psychHIDKbQueueThread, NULL, PsychHIDKbQueueNewThread, NULL);
		if(returnCode!=0){
			free(psychHIDKbQueueFirstPress);
			free(psychHIDKbQueueFirstRelease);
			free(psychHIDKbQueueLastPress);
			free(psychHIDKbQueueLastRelease);
			CFRelease(hidDataRef->hidElementDictionary);
			(*hidDataRef->hidQueueInterface)->dispose(hidDataRef->hidQueueInterface);
			(*hidDataRef->hidQueueInterface)->Release(hidDataRef->hidQueueInterface);
			free(hidDataRef);
			hidDataRef=NULL;
			PsychErrorExitMsg(PsychError_system, "Failed to create thread");
		}
	}
	return(PsychError_none);	
}

#endif
