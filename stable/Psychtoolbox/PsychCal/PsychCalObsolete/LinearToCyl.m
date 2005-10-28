function cyl = LinearToCyl(cal,linear)
% cyl = LinearToCyl(cal,linear)
%
% Convert from cylindrical to linear coordinates.
%
% We use the conventions of the CIE Lxx color spaces
% for angle.
%
% LinearToCyl has been renamed "SensorToCyl".  The old
% name, "LinearToCyl", is still provided for compatability 
% with existing scripts but will disappear from future releases 
% of the Psychtoolbox.  Please use SensorToCyl instead.
%
% See Also: PsychCal, PsychCalObsolete, SensorToCyl

% 10/17/93		dhb   Wrote it by converting CAP C code.
% 2/20/94			jms   Added single argument case to avoid cData.
% 4/5/02      dhb, ly  Call through new interface.
% 4/11/02   awi   Added help comment to use SensorToCyl instead.
%                 Added "See Also"


if (nargin == 1)
	cyl = SensorToCyl(linear);
else
	cyl = SensorToCyl(cal,linear);
end
