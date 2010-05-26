function cal = CalibrateFitGamma(cal,nInputLevels)
% cal = CalibrateFitGamma(cal,[nInputLevels])
%
% Fit the gamma function to the calibration measurements.  Options for field
% cal.describe.gamma.fitType are:
%    simplePower
%    crtLinear
%    crtPolyLinear
%    crtGamma
%    crtSumPow
%    sigmoid
%    weibull
%
% Underlying fit routine is FitGamma for functional forms originally supported,
% and these rely on the optimization toolbox.
%
% Newer functions (e.g, crtSumPow) use the curvefit toolbox and that's just
% done locally in this routine.  Much less cumbersome.
%
% See also PsychGamma.
%
% 3/26/02  dhb  Pulled out of CalibrateMonDrvr.
% 11/14/06 dhb  Define nInputLevels and pass to underlying fit routine.
% 07/22/07 dhb  Add simplePower fitType.
% 08/02/07 dhb  Optional pass of nInputLevels.
%          dhb  Don't allow a long string of zeros at the start.
%          dhb  Reduce redundant code for higher order terms by pulling out of switch
% 08/03/07 dhb  Debug.  Add call to MakeMonotonic for first three components.
% 11/19/09 dhb  Added crtSumPow option, coded to [0-1] world and using curve fit toolbox.
% 3/07/10  dhb  Cosmetic to make m-lint happier, including some "|" -> "||"
% 3/07/10  dhb  Added crtLinear option.
%          dhb  contrasthThresh and fitBreakThresh values only set if not already in struct.
%          dhb  Call MakeGammaMonotonic rather than MakeMonotonic where appropriate.
%          dhb  Use linear interpolation for higher order linear model weights, rather than
%               a polynomial.  I now think that ringing is worse than not smoothing enough.
% 3/08/10  dhb  Update list of options in comment above.
% 5/26/10   dhb Allow values_in to be either a single column or a matrix with same number of columns as values_out.


% Set nInputLevels
if (nargin < 2 || isempty(nInputLevels))
    nInputLevels = 1024;
end

% Fit gamma functions.
switch(cal.describe.gamma.fitType)
    
     case 'simplePower',
        mGammaMassaged = cal.rawdata.rawGammaTable(:,1:cal.nDevices);
        for i = 1:cal.nDevices
            mGammaMassaged(:,i) = MakeGammaMonotonic(HalfRect(mGammaMassaged(:,i)));
        end
        fitType = 1;
        [mGammaFit1a,cal.gammaInput,mGammaCommenta] = FitDeviceGamma(...
            mGammaMassaged,cal.rawdata.rawGammaInput,fitType,nInputLevels);
        mGammaFit1 = mGammaFit1a;
    
    case 'crtLinear'
        % Set to zero the raw data we believe to be below reliable measurement
        % threshold, and then fit the rest by linear interpolation.  Force answer
        % to be monotonic.
        if (~isfield(cal.describe.gamma,'contrastThresh'))
            cal.describe.gamma.contrastThresh = 0.001;
        end
        mGammaMassaged = cal.rawdata.rawGammaTable(:,1:cal.nDevices);
        massIndex = find(mGammaMassaged < cal.describe.gamma.contrastThresh);
        mGammaMassaged(massIndex) = zeros(length(massIndex),1);
        for i = 1:cal.nDevices
            mGammaMassaged(:,i) = MakeGammaMonotonic(HalfRect(mGammaMassaged(:,i)));
        end
       
        fitType = 6;
        [mGammaFit1,cal.gammaInput,mGammaCommenta] = FitDeviceGamma(...
            mGammaMassaged,cal.rawdata.rawGammaInput,fitType,nInputLevels);
        
    case 'crtPolyLinear',
        % For fitting, we set to zero the raw data we
        % believe to be below reliable measurement threshold (contrastThresh).
        % Currently we are fitting both with polynomial and a linear interpolation,
        % using the latter for low measurement values.  The fit break point is
        % given by fitBreakThresh.   This technique was developed
        % through bitter experience and is not theoretically driven.
        if (~isfield(cal.describe.gamma,'contrastThresh'))
            cal.describe.gamma.contrastThresh = 0.001;
        end
        if (~isfield(cal.describe.gamma,'fitBreakThresh'))
            cal.describe.gamma.fitBreakThresh = 0.02;
        end
        mGammaMassaged = cal.rawdata.rawGammaTable(:,1:cal.nDevices);
        massIndex = find(mGammaMassaged < cal.describe.gamma.contrastThresh);
        mGammaMassaged(massIndex) = zeros(length(massIndex),1);
        for i = 1:cal.nDevices
            mGammaMassaged(:,i) = MakeGammaMonotonic(HalfRect(mGammaMassaged(:,i)));
        end
        fitType = 7;
        [mGammaFit1a,cal.gammaInput,mGammaCommenta] = FitDeviceGamma(...
            mGammaMassaged,cal.rawdata.rawGammaInput,fitType,nInputLevels);
        fitType = 6;
        [mGammaFit1b,cal.gammaInput,mGammaCommentb] = FitDeviceGamma(...
            mGammaMassaged,cal.rawdata.rawGammaInput,fitType,nInputLevels);
        mGammaFit1 = mGammaFit1a;
        for i = 1:cal.nDevices
            indexLin = find(mGammaMassaged(:,i) < cal.describe.gamma.fitBreakThresh);
            if (~isempty(indexLin))
                breakIndex = max(indexLin);
                breakInput = cal.rawdata.rawGammaInput(breakIndex);
                inputIndex = find(cal.gammaInput <= breakInput);
                if (~isempty(inputIndex))
                    mGammaFit1(inputIndex,i) = mGammaFit1b(inputIndex,i);
                end
            end
        end
        
    case 'crtGamma',
        mGammaMassaged = cal.rawdata.rawGammaTable(:,1:cal.nDevices);
        for i = 1:cal.nDevices
            mGammaMassaged(:,i) = MakeGammaMonotonic(HalfRect(mGammaMassaged(:,i)));
        end
        fitType = 2;
        [mGammaFit1a,cal.gammaInput,mGammaCommenta] = FitDeviceGamma(...
            mGammaMassaged,cal.rawdata.rawGammaInput,fitType,nInputLevels);
        mGammaFit1 = mGammaFit1a;
        
    case 'crtSumPow',
        if (~exist('fit','file'))
            error('Fitting with the sum of exponentials requires the curve fitting toolbox\n');
        end
        if (max(cal.rawdata.rawGammaInput) > 1)
            error('crtSumPower option assumes [0-1] specification of input\n');
        end
        mGammaMassaged = cal.rawdata.rawGammaTable(:,1:cal.nDevices);
        for i = 1:cal.nDevices
            mGammaMassaged(:,i) = MakeGammaMonotonic(HalfRect(mGammaMassaged(:,i)));
        end
        
        fitEqStr = 'a*x^b + (1-a)*x^c';
        a = 1;
        b = 2;
        c = 0;
        startPoint = [a b c];
        lowerBounds = [0 0.1 0.01];
        upperBounds = [1 10 10];
      
        % Fit and predictions
        fOptions = fitoptions('Method','NonlinearLeastSquares','Robust','on');
        fOptions1 = fitoptions(fOptions,'StartPoint',startPoint,'Lower',lowerBounds,'Upper',upperBounds);
        for i = 1:cal.nDevices
            if (size(cal.rawdata.rawGammaInput,2) == 1)
                fitstruct = fit(cal.rawdata.rawGammaInput,mGammaMassaged(:,i),fitEqStr,fOptions1);
            else
                fitstruct = fit(cal.rawdata.rawGammaInput(:,i),mGammaMassaged(:,i),fitEqStr,fOptions1);
            end
            mGammaFit1a(:,i) = feval(fitstruct,linspace(0,1,nInputLevels));
        end
        mGammaFit1 = mGammaFit1a;
        
    case 'sigmoid',
        mGammaMassaged = cal.rawdata.rawGammaTable(:,1:cal.nDevices);
        for i = 1:cal.nDevices
            mGammaMassaged(:,i) = MakeGammaMonotonic(HalfRect(mGammaMassaged(:,i)));
        end
        fitType = 3;
        [mGammaFit1a,cal.gammaInput,mGammaCommenta] = FitDeviceGamma(...
            mGammaMassaged,cal.rawdata.rawGammaInput,fitType,nInputLevels);
        mGammaFit1 = mGammaFit1a;
        
    case 'weibull',
        mGammaMassaged = cal.rawdata.rawGammaTable(:,1:cal.nDevices);
        for i = 1:cal.nDevices
            mGammaMassaged(:,i) = MakeGammaMonotonic(HalfRect(mGammaMassaged(:,i)));
        end
        fitType = 4;
        [mGammaFit1a,cal.gammaInput,mGammaCommenta] = FitDeviceGamma(...
            mGammaMassaged,cal.rawdata.rawGammaInput,fitType,nInputLevels);
        mGammaFit1 = mGammaFit1a;

    otherwise
        error('Unsupported gamma fit string passed');

end

% Fix contingous zeros at start problem
mGammaFit1 = FixZerosAtStart(mGammaFit1);
for j = 1:size(mGammaFit1,2)
    mGammaFit1(:,j) = MakeGammaMonotonic(mGammaFit1(:,j));
end

% Handle higher order terms, which are just fit with a polynomial
if (cal.nPrimaryBases > 1)
    [m,n] = size(mGammaFit1);
    mGammaFit2 = zeros(m,cal.nDevices*(cal.nPrimaryBases-1));
    
    OLDFIT = 0;
    if (OLDFIT)
        for j = 1:cal.nDevices*(cal.nPrimaryBases-1)
            mGammaFit2(:,j) = ...
                FitGammaPolyR(cal.rawdata.rawGammaInput,cal.rawdata.rawGammaTable(:,cal.nDevices+j), ...
                cal.gammaInput);
        end
    else
        for j = 1:cal.nDevices*(cal.nPrimaryBases-1)
            mGammaFit2(:,j) = interp1([0 ; cal.rawdata.rawGammaInput],[0 ; cal.rawdata.rawGammaTable(:,cal.nDevices+j)],cal.gammaInput,'linear');
        end
    end
        
    mGammaFit = [mGammaFit1 , mGammaFit2];
else
    mGammaFit = mGammaFit1;
end
        
% Save information in form for calibration routines.
cal.gammaFormat = 0;
cal.gammaTable = mGammaFit;

return

% output = FixZerosAtStart(input)
%
% The OS/X routines need the fit gamma function to be monotonically
% increasing.  One way that sometimes fails is when a whole bunch of
% entries at the start are zero.  This routine fixes that up.
function output = FixZerosAtStart(input,nPrimaryBases)

output = input;
for j = 1:size(input,2)
    for i = 1:size(input,1)
        if (input(i,j) > 0)
            break;
        end
    end
    if (i == size(input,1))
        error('Entire passed gamma function is zero');
    end
    output(1:i,j) = linspace(0,min([0.0001 input(i+1,j)/2]),i)';
end

return


