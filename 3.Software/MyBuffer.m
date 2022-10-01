classdef MyBuffer<handle
    %MYBUFFER Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        data;
        RNum; %data的行数目,记录个数
        CNum; %data的列数目(固定)，记录维度
        length; %当前data中有效数据（行）的数目,每行为一个记录
    end
    
    methods
        function self=MyBuffer(rowsNum, columnNum)
            self.data=zeros(rowsNum, columnNum);
            self.RNum=rowsNum;
            self.CNum=columnNum;
            self.length=0;
        end
        
        function obj=put(obj, Observation)
            [h,l]=size(Observation);
            if l~=obj.CNum
                error('Column dose not match to buffer');
            else
                if obj.length+h<=obj.RNum
                    obj.data(obj.length+1:obj.length+h,:)=Observation;
                    obj.length=obj.length+h;
                elseif h<obj.RNum
                    obj.data(1:obj.RNum-h,:)=obj.data(obj.length-(obj.RNum-h)+1:obj.length,:);
                    obj.data(obj.RNum-h+1:obj.RNum,:)=Observation;
                    obj.length=obj.RNum;
                else
                    obj.data(:,:)=Observation(h-obj.RNum+1:h,:);
                    obj.length=obj.RNum;
                end
            end
        end
        
        function len=getLength(obj)
            len=obj.length;
        end
        
        function fullFlag=isFull(obj)
            if obj.length==obj.RNum
                fullFlag=1;
            else
                fullFlag=0;
            end
        end
        
        function emptyFlag=isEmpty(obj)
            if obj.length==0
                emptyFlag=1;
            else
                emptyFlag=0;
            end
        end
        
        function Data=getData(obj)
            Data=obj.data(1:obj.length,:);
        end
        
        function Data=getPartialData(obj, from, to)
            Data=obj.data(from:to, :);
        end
        
        function clear(obj)
            obj.data=zeros(obj.RNum, obj.CNum);
            obj.length=0;
        end
        
    end
    
end

