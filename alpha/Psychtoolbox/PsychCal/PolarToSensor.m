function [sensor] = PolarToSensor(cData,pol)
% [sensor] = PolarToSensor(cData,pol)
%
% Converts from polar sensor coordinates to
% rectangular sensor coordinates.
%
% Polar coordinates are defined as radius, azimuth, and elevation.
%
% 9/26/93    dhb   Added calData argument.
% 2/6/94     jms   Changed 'polar' to 'pol'
% 2/20/94    jms   Added single argument case to avoid cData.
% 4/5/02     dhb, ly  New calling interface.

if (nargin==1)
  pol=cData;
end

[n,m] = size(pol);
if (n ~= 3)
  error('Cannot handle polar coordinates with dimension other than 3');
end

sensor = zeros(n,m);
sensor(1,:) = pol(1,:).*cos(pol(3,:)).*cos(pol(2,:));
sensor(2,:) = pol(1,:).*cos(pol(3,:)).*sin(pol(2,:));
sensor(3,:) = pol(1,:).*sin(pol(3,:));
