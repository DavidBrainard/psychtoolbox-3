function [device] = PolarToDevice(cal,pol)
% [device] = PolarToDevice(cal,pol)
%
% Convert from polar color space coordinates to linear device
% coordinates.
%
% PolarToDevice is obsolete.  Instead, use a combination
% of PolarToSensor and SensorToPrimary to achieve the same
% result.  For example, instead of: 
%
% device = PolarToDevice(cal,pol)
%
% use:
%
% linear = PolarToSensor(cal,pol);
% device = SensorToPrimary(cal,linear);
%
% See Also: PsychCal, PsychCalObsolete, PolarToSensor, SensorToPrimary

%
% This depends on the standard calibration globals.
%
% 9/26/93    dhb   Added cal argument.
% 2/6/94     jms   Changed 'polar' to 'pol'
% 4/11/02   awi   Added help comment to use PolarToSensor+SensorToPrimary instead.
%                 Added "See Also"


linear = PolarToLinear(cal,pol);
device = LinearToDevice(cal,linear);
