function rmse = ComputeRMSE(data,predict)
% rmse = ComputeRMSE(data,predict)
%
% Compute fractional RMSE between data and prediction.
% Inputs should be column vectors.
% Actual code is:
%		diff = predict-data;
%		rmse = sqrt((diff'*diff)/(data'*data));
%
% 2/3/96		dhb		Added improved comments.

diff = predict-data;
rmse = sqrt((diff'*diff)/(data'*data));
