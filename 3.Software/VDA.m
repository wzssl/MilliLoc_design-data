function [x1,x2,amp,zcr] = VDA(x)
%幅度归一化到[-1,1]
x = x / max(abs(x));

%常数设置
FrameLen = 256; %帧长为960点
FrameInc = 128; %帧移为468点

amp1 = 0.4;  %初始短时能量高门限15
amp2 = 0.3;  %初始短时能量低门限2
zcr1 = 5;  %初始短时过零率高门限10
zcr2 = 2;  %初始短时过零率低门限5

maxsilence = 6;  %用静音的长度来判断语音是否结束
minlen  = 20;    %语音段的最短长度，若语音段长度小于此值，则认为其为一段噪音
status  = 0;     %记录语音段的状态,初始状态为静音状态
count   = 0;     %语音序列的长度,初始语音段长度为0
silence = 0;     %静音的长度,初始静音段长度为0

%计算过零率  
tmp1  = enframe(x(1:end-1), FrameLen, FrameInc);%分帧，所得矩阵为fix（（x(1:end-1)-FrameLen+FrameInc）/FrameInc）*FrameLen
tmp2  = enframe(x(2:end)  , FrameLen, FrameInc);%分帧，所得矩阵为fix（（x(2:end)-FrameLen+FrameInc）/FrameInc）*FrameLen
signs = (tmp1.*tmp2)<0;   %tmp1.*tmp2所得矩阵小于等于零的赋值为1，大于零的赋值为0
diffs = (tmp1 -tmp2)>0.02;%tmp1-tmp2所得矩阵小于0.02的赋值为0，大于等于0.02的赋值为1
zcr   = sum(signs.*diffs, 2); %得到了信号各帧的过零率值，放到zcr矩阵中
 
%计算短时能量
amp = sum((abs(enframe(x, FrameLen, FrameInc))).^2, 2); %求出x各帧的能量值
 
%调整能量门限
amp1 = min(amp1, max(amp)/4);
amp2 = min(amp2, max(amp)/8);
 
%开始端点检测
for n=1:length(zcr)             %length（zcr）得到的是整个信号的帧数
   switch status
   case {0,1}                   % 0 = 静音, 1 = 可能开始
      if amp(n) > amp1          % 高于能量最大门限值，确信进入语音段
         x1 = max(n-count-1,1);
         status  = 2;
         silence = 0;
         count   = count + 1;
      elseif amp(n) > amp2 || ...% 高于能量最小门限值或大于过零率阈值，可能处于语音段
             zcr(n) > zcr2
         status = 1;
         count  = count + 1;
      else                       % 静音状态
         status  = 0;
         count   = 0;
      end
   case 2                        % 2 = 语音段，进入语音段状态
      if amp(n) > amp2 || ...    % 保持在语音段
         zcr(n) > zcr2
         count = count + 1;
      else                       % 语音将结束
         silence = silence+1;
         if silence < maxsilence % 静音还不够长，尚未结束
            count  = count + 1;
         elseif count < minlen   % 语音长度太短，认为是噪声
            status  = 0;
            silence = 0;
            count   = 0;
         else                    % 语音结束
            status  = 3;
         end
      end
   case 3
      break;
   end
end  
count = count-silence/2;
x2 = x1 + count -1;


%%绘图
figure(3)
subplot(311)    %subplot(3,1,1)表示将图排成3行1列，最后的一个1表示下面要画第1幅图
plot(x)
axis([1 length(x) -1 1])    %函数中的四个参数分别表示xmin,xmax,ymin,ymax，即轴的范围
ylabel('Speech');
line([x1*FrameInc x1*FrameInc], [-1 1], 'Color', 'red');
%这里作用为用直线画出语音段的起点和终点，看起来更直观。第一个[]中的两个参数为线起止点的横坐标，
%第二个[]中的两个参数为线起止点的纵坐标。最后两个参数设置了线的颜色。
line([x2*FrameInc x2*FrameInc], [-1 1], 'Color', 'red');
subplot(312)   
plot(amp);
axis([1 length(amp) 0 max(amp)])
ylabel('Energy');
line([x1 x1], [min(amp),max(amp)], 'Color', 'red');
line([x2 x2], [min(amp),max(amp)], 'Color', 'red');
subplot(313)
plot(zcr);
axis([1 length(zcr) 0 max(zcr)])
ylabel('ZCR');
line([x1 x1], [min(zcr),max(zcr)], 'Color', 'red');
line([x2 x2], [min(zcr),max(zcr)], 'Color', 'red');
