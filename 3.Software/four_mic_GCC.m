% 四麦克风程序
% 数据采集
clear;
clc; 
% dbstop if error;
warning('off');
        
%系统参数
Fs=96000;         %采样频率 Hz
Duration=0.4;     %程序迭代一次的时间 S
BufferSize=Duration * Fs; %播放器和录音器的缓存大小 采样点
ExcuteTime=96000; %程序执行时间，单位秒
v=34300;          %声速cm/s

% 录音器
AR = audioDeviceReader('Device','麦克风阵列 (RE SoundCard 446(A)W(96)4M)','ChannelMappingSource','Auto','NumChannels',4,'SamplesPerFrame',BufferSize,'SampleRate',Fs);%%%,'BitDepth ','16-bit integer'
Echo=zeros(Fs*Duration,4); %预先申请回波数据 ReceiveData 空间
setup(AR);

%回波缓存
%BufferLen=BufferSize*4;
%EchoBuffer=MyBuffer(BufferLen, 4);
%Echo=zeros(BufferLen,4);
% Echo_filtered=Echo;
% t = 0:1/Fs:4*Duration-1/Fs;

%回波滤波器，滤掉噪音
LBFreq=300;
UBFreq=3000;
[Filter_StaticFreq_b,Filter_StaticFreq_a]=ellip(5,0.1,95,[LBFreq UBFreq]/(Fs/2),'bandpass');
%fvtool(Filter_StaticFreq_b, Filter_StaticFreq_a);

%主程序迭代次数
IterNum=0;

FrameLen=960;
inc=468;
threshold=0.2;
x_l = -200:50:200;
y_1 = -100:20:100;
n=-65535:1:65536;

%主程序开始运行
while IterNum< ExcuteTime
      IterNum=IterNum+1;
   
   [Echo,nOverrun] = step(AR);
    
    if nOverrun > 0
       fprintf('Audio recorder queue was overrun by %d samples\n'...
           ,nOverrun);
    end

    %for i=1:4
    Echo = medfilt1(Echo,32); %中值滤波
    Echo = filtfilt(Filter_StaticFreq_b, Filter_StaticFreq_a, Echo); %带通滤波
    %end    
    %?figure?窗口最大化，坐标轴也随着窗口变大而相应变大
    scrsz=get(0,'ScreenSize');  %是为了获得屏幕大小，Screensize是一个4元素向量[left,bottom,?width,?height]
    set(gcf,'Position',scrsz);  %用获得的screensize向量设置figure的position属性，实现最大化的目的  
        
%     subplot(411)
%     plot(Echo(:,1));
%     title('CH-1');
%     subplot(412)
%     plot(Echo(:,2));
%     title('CH-2');
%     subplot(413)
%     plot(Echo(:,3));
%     title('CH-3');        
%     subplot(414)
%     plot(Echo(:,4));
%     title('CH-4');
        
    %音频端点检测
    envelope=abs(hilbert(Echo(:,1)));
    index=find(envelope>threshold,50);
    [x1,x2]=VDA(Echo(:,1));
    if (x1~=0)&&(x2~=0)&&(length(index) >= 50)&&(x2>x1)
        if (x1*inc >200)&&(x2*inc < 28600)  
            start = x1*inc; %设定语音起始点
            stop = x2*inc;
            figure(1)
            subplot(211)
            plot(Echo(:,1));  
            title('CH-1');
            hold on;
            line([start start], [-1,1], 'Color', 'red','linewidth',1.5);
            line([stop stop], [-1,1], 'Color', 'green','linewidth',1.5);
            hold off;
            
            %广义互相关
            xgcc1 = gcc_phat(Echo(start:stop,1), Echo(start:stop,2),Fs,LBFreq,UBFreq);
            xgcc2 = gcc_phat(Echo(start:stop,1), Echo(start:stop,3),Fs,LBFreq,UBFreq);
            xgcc3 = gcc_phat(Echo(start:stop,1), Echo(start:stop,4),Fs,LBFreq,UBFreq);
            
            subplot(212)
            plot(n,xgcc1)
            title('GCC-PHAT');
            
            GCC1 = xgcc1(65537-47:65537+47);
            GCC2 = xgcc2(65537-47:65537+47);
            GCC3 = xgcc3(65537-47:65537+47);
            [pk1,loc1] = max(GCC1); 
            [pk2,loc2] = max(GCC2); 
            [pk3,loc3] = max(GCC3); 
            hold on;
            plot(loc1-47,pk1,'ro');
            hold off;
            
            d12 = (loc1-47)/Fs*v;
            d13 = (loc2-47)/Fs*v;
            d14 = (loc3-47)/Fs*v;
            
            fprintf("m12:%.2fcm\n",d12);
            fprintf("m13:%.2fcm\n",d13);
            fprintf("m14:%.2fcm\n",d14);
            
            rs = Leastsquarespos(d12,d13,d14);
            %fprintf("声源坐标：(%.2fcm,%.2fcm)\n",rs(1),rs(2));          

%             figure(2)
%             title('实时声源定位系统演示')
%             line([-16 16], [0,0], 'Color', 'red','linewidth',5);
%             line([0 0], [0,16], 'Color', 'red','linewidth',5);
%             hold on;
%             plot(0,0,'ro')
%             axis([-200 200 0 200]);
%             set(gca, 'XTick', x_l);
%             set(gca, 'YTick', y_1);
%             set(gca,'ydir','reverse')
%             grid on
%             plot(rs(1),rs(2),'ro','MarkerFaceColor','r','Markersize',10,'MarkerEdgeColor',[0,1,0],'LineWidth',1.5);
%             text(rs(1)+5,rs(2)+5,num2str(rs(1),'(%.2fcm,'),'FontSize',12);
%             text(rs(1)+35,rs(2)+5,num2str(rs(2),'%.2fcm)'),'FontSize',12);
%             fprintf("声源坐标：(%.2fcm,%.2fcm)\n",rs(1),rs(2));
        end
    end
    
    pause(0.01);
    %hold off;
    %f = gcf;
    %cla(f.CurrentAxes);
end
