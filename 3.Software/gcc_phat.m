function xgcc = gcc_phat(sig, refsig, Fs, fa, fb)

% if (57600>=length(sig))&&(length(sig)>=38400)
%     N = 65536; %2^17=131072 
% elseif length(sig)>=57600
%N = 131072; %2^17=131072 ;262144
% end
N = 2^nextpow2(length(sig));
    
f=(-N/2:N/2-1)*Fs/N; %真实频率序列

FA=fft(sig,N);    %快�?�傅里叶变换，其fft(x,n)中的n表示为返回n点的DFT
FB=fft(refsig,N);
FAB=FA.*conj(FB); %互（功率密度）谱，利用卷积定理求互相关，互相关的傅里叶变换是互功率谱（conj(Z)返回Z的元素的复共轭）

FAB=(FAB./max(abs(FAB)))';%�?值归�?�?,保持有效信号与噪声信号的幅�?�比
%
% FAB=(FAB./abs(FAB))';
% Shiftmag = fftshift(FAB);
%
Shiftmag = fftshift(FAB); %双边谱，移动零频点到频谱中心
% figure;
% plot(f,abs(Shiftmag));

%Mag=Shiftmag./abs(Shiftmag);
%有效信号频带内互谱加�?,互谱幅�?�被部分归一，相当于时域白化，但保留了相位信息，负半轴：-fb:-fa，正半轴：fa:fb
StartIndexP=round((N/2+1)+fa*N/Fs);
StopIndexP= round((N/2+1)+fb*N/Fs);
StartIndexN=round((N/2+1)-fb*N/Fs);
StopIndexN= round((N/2+1)-fa*N/Fs);
Shiftmag(StartIndexP:StopIndexP)=Shiftmag(StartIndexP:StopIndexP)./abs(Shiftmag(StartIndexP:StopIndexP)); %
Shiftmag(StartIndexN:StopIndexN)=Shiftmag(StartIndexN:StopIndexN)./abs(Shiftmag(StartIndexN:StopIndexN)); %

xcorr=ifft(Shiftmag);
xgcc = real(fftshift(xcorr)); 

%**********插�??***********%
% [~,DelayDifferAB]=max(R_ab);   %互相关最大�?�的横坐标位置体现了到达延迟�?
% 对互相关�?大�?�lag的左右范围进行样条插值，lag的间隔由1变到0.1
% ori_x = (DelayDifferAB-2):(DelayDifferAB+2);  
% up_x  = (DelayDifferAB-2):0.1:(DelayDifferAB+2);
% R_ab_sf_up = interp1(ori_x,R_ab_sf(ori_x),up_x,'spline');
% [~,DelayDifferAB_relative]=max(R_ab_sf_up);    %对峰值互相关的周围插值后求取峰�?�对应的lag,让lag取得更精细化
% diff_len = (up_x(DelayDifferAB_relative)-length(R_ab)/2)*vf;  %算得两组麦接收信号的时延差导致的距离�?

end