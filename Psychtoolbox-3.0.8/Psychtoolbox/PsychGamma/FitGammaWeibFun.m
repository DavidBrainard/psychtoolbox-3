function [err,con] = FitGammaWeibFun(x,values,measurements)
% [err,con] = FitGammaWeibFun(x,values,measurements)
%
% 10/3/93   dhb   Created

predict = ComputeGammaWeib(x,values);
err = ComputeRMSE(measurements,predict);

con = [-1];
 




