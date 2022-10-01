function rs = Leastsquarespos(d12,d13,d14)
md = [d12,d13,d14]';
R = 12;       %麦克风间距，单位cm
b = 0.5.*[R^2-d12^2,R^2-d13^2,R^2-d14^2]'; %
S = [-R,0,0;0,-R,0;R,0,0]; %麦克风坐�?
Ps = eye(3) - S*pinv(S);
rs = pinv(S)*(eye(3) - (md*md'*Ps)/(md'*Ps*md))*b;