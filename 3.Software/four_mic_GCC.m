% ����˷����
% ���ݲɼ�
clear;
clc; 
% dbstop if error;
warning('off');
        
%ϵͳ����
Fs=96000;         %����Ƶ�� Hz
Duration=0.4;     %�������һ�ε�ʱ�� S
BufferSize=Duration * Fs; %��������¼�����Ļ����С ������
ExcuteTime=96000; %����ִ��ʱ�䣬��λ��
v=34300;          %����cm/s

% ¼����
AR = audioDeviceReader('Device','��˷����� (RE SoundCard 446(A)W(96)4M)','ChannelMappingSource','Auto','NumChannels',4,'SamplesPerFrame',BufferSize,'SampleRate',Fs);%%%,'BitDepth ','16-bit integer'
Echo=zeros(Fs*Duration,4); %Ԥ������ز����� ReceiveData �ռ�
setup(AR);

%�ز�����
%BufferLen=BufferSize*4;
%EchoBuffer=MyBuffer(BufferLen, 4);
%Echo=zeros(BufferLen,4);
% Echo_filtered=Echo;
% t = 0:1/Fs:4*Duration-1/Fs;

%�ز��˲������˵�����
LBFreq=300;
UBFreq=3000;
[Filter_StaticFreq_b,Filter_StaticFreq_a]=ellip(5,0.1,95,[LBFreq UBFreq]/(Fs/2),'bandpass');
%fvtool(Filter_StaticFreq_b, Filter_StaticFreq_a);

%�������������
IterNum=0;

FrameLen=960;
inc=468;
threshold=0.2;
x_l = -200:50:200;
y_1 = -100:20:100;
n=-65535:1:65536;

%������ʼ����
while IterNum< ExcuteTime
      IterNum=IterNum+1;
   
   [Echo,nOverrun] = step(AR);
    
    if nOverrun > 0
       fprintf('Audio recorder queue was overrun by %d samples\n'...
           ,nOverrun);
    end

    %for i=1:4
    Echo = medfilt1(Echo,32); %��ֵ�˲�
    Echo = filtfilt(Filter_StaticFreq_b, Filter_StaticFreq_a, Echo); %��ͨ�˲�
    %end    
    %?figure?������󻯣�������Ҳ���Ŵ��ڱ�����Ӧ���
    scrsz=get(0,'ScreenSize');  %��Ϊ�˻����Ļ��С��Screensize��һ��4Ԫ������[left,bottom,?width,?height]
    set(gcf,'Position',scrsz);  %�û�õ�screensize��������figure��position���ԣ�ʵ����󻯵�Ŀ��  
        
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
        
    %��Ƶ�˵���
    envelope=abs(hilbert(Echo(:,1)));
    index=find(envelope>threshold,50);
    [x1,x2]=VDA(Echo(:,1));
    if (x1~=0)&&(x2~=0)&&(length(index) >= 50)&&(x2>x1)
        if (x1*inc >200)&&(x2*inc < 28600)  
            start = x1*inc; %�趨������ʼ��
            stop = x2*inc;
            figure(1)
            subplot(211)
            plot(Echo(:,1));  
            title('CH-1');
            hold on;
            line([start start], [-1,1], 'Color', 'red','linewidth',1.5);
            line([stop stop], [-1,1], 'Color', 'green','linewidth',1.5);
            hold off;
            
            %���廥���
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
            %fprintf("��Դ���꣺(%.2fcm,%.2fcm)\n",rs(1),rs(2));          

%             figure(2)
%             title('ʵʱ��Դ��λϵͳ��ʾ')
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
%             fprintf("��Դ���꣺(%.2fcm,%.2fcm)\n",rs(1),rs(2));
        end
    end
    
    pause(0.01);
    %hold off;
    %f = gcf;
    %cla(f.CurrentAxes);
end
