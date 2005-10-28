function bounds=TextBounds(w,text)
% bounds=TextBounds(window,string)
%
% Returns the smallest enclosing rect for the drawn text, relative to
% the current location. This bound is based on the actual pixels
% drawn, so it incorporates effects of text smoothing, etc. "text"
% may be a cell array or matrix of 1 or more strings. The strings are
% drawn one on top of another, at the same initial position, before
% the bounds are calculated. This returns the smallest box that will
% contain all the strings. The prior contents of the scratch window
% are lost. Usually it should be an offscreen window, so the user
% won't see it. The scratch window should be at least twice as wide
% as the text, because the text is drawn starting at the left-right
% center of the window to cope with uncertainties about text
% direction (e.g. Hebrew) and some unusual characters that extend
% greatly to the left of their nominal starting point. If you only
% know your nominal text size and number of characters, you might do
% this to create your scratch window:
% 
% textSize=24;
% string='Hello world.';
% if IsOSX
%     w=Screen('OpenWindow',0);
% end
% if IsOS9
%     w=Screen(-1,'OpenOffscreenWindow',[],[0 0 3*textSize*length(string) 2*textSize],1);
% end
% Screen(w,'TextFont','Arial');
% Screen(w,'TextSize',textSize);
% Screen(w,'TextStyle',1); % 0=plain (default for new window), 1=bold, etc.
% bounds=TextBounds(w,string);
% ...
% Screen(w,'Close');
% 
% The suggested window size in that call is generously large because there
% aren't any guarantees from the font makers about how big the text might
% be for a specified point size. The pixelSize of 1 (the last argument)
% minimizes the memory requirements. Set your window's font, size, and 
% (perhaps) style before calling TextBounds. 
% 
% Be warned that TextBounds and TextCenteredBounds are slow (taking many
% seconds) if the window is large. They use the whole window, so if the
% window is 1024x1204 they process a million pixels. The two slowest calls
% are Screen 'GetImage' and FIND. Their processing time is proportional to
% the number of pixels in the window. We haven't checked, but it's very
% likely that processing time is also proportional to pixelSize, so we
% suggest using a small pixelSize (e.g. 1 bit, using an offscreen
% window).
% 
% OSX: Also see Screen 'TextBounds'.
% 
% OS9: If you just want a quick and dirty "TextHeight", analogous to  
% the existing Screen 'TextWidth', you might follow Charles Collin's 
% (ccollin@dal.ca) advice, and approximate the height of text by the width 
% of two e's, which "usually works well enough."
% 
% textHeight=Screen(w,'TextWidth','ee'); % OS9
% 
% Screen 'TextWidth' calls Apple's TextWidth. The name 'TextWidth' is 
% slightly misleading. TextWidth does NOT tell you about the
% bounding box. It tells you how much the pen position is advanced by
% drawing the character. The character (e.g. a capital letter in a script
% font) may in fact extend to the left of the original starting point and
% to the right of the final end point.
% 
% The user interface would be cleaner if this function opened and closed
% its own offscreen window, instead of writing in the user's window. 
% Unfortunately the Mac OS takes on the order of a second to open and to 
% close an offscreen window, making the overhead prohibitive.
% 
% Also see TextCenteredBounds.

% 9/1/98   dgp wrote it.
% 3/19/00  dgp debugged it.
% 11/17/02 dgp Added fix, image1(:,:,1),  suggested by Keith Schneider to 
%              support 16 and 32 bit images.
% 9/16/04  dgp Suggest a pixelSize of 1.
% 12/16/04 dgp Fixed handling of cell array.
% 12/17/04 dgp Round x0 so bounds will always be integer. Add comment about speed.
% 1/18/05  dgp Added Charles Collin's two e suggestion for textHeight.
% 1/28/05  dgp Cosmetic.
% 2/4/05   dgp Support both OSX and OS9.

Screen(w,'FillRect',0);
r=Screen(w,'Rect');
x0=round((r(RectLeft)+r(RectRight))/2);
y0=round((r(RectTop)+2*r(RectBottom))/3);
if iscell(text)
	for i=1:length(text)
		string=char(text(i));
        if IsOS9
            Screen(w,'DrawText',string,x0,y0,255);
        end
        if IsOSX
            Screen('DrawText',w,string,x0,y0,255);
        end
	end
else
	for i=1:size(text,1)
		string=char(text(i,:));
        if IsOS9
            Screen(w,'DrawText',string,x0,y0,255);
        end
        if IsOSX
            Screen('DrawText',w,string,x0,y0,255);
        end
	end
end
% Screen(w,'DrawText','',x0,y0); % Set pen position to x0 y0.
image1=Screen(w,'GetImage');
[y,x]=find(image1(:,:,1));
if isempty(y) | isempty(x)
	bounds=[0 0 0 0];
else	
	bounds=SetRect(min(x)-1,min(y)-1,max(x),max(y));
	bounds=OffsetRect(bounds,-x0,-y0);
end
