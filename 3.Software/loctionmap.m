function loctionmap(rs,x_l,y_1)
plot(0,0,'ro')
hold on;
title('实时声源定位系统演示','FontSize',42);
line([-10 10], [0,0], 'Color', 'red','linewidth',5);
line([0 0], [0,10], 'Color', 'red','linewidth',5);
text(-16,-8,'麦克风阵列','FontSize',15);
axis([-200 200 -200 200]);
set(gca, 'XTick', x_l);
set(gca, 'YTick', y_1);
set(gca,'ydir','reverse')
grid on
plot(rs(1),rs(2),'ro','MarkerFaceColor','b','Markersize',20,'MarkerEdgeColor',[0,1,0],'LineWidth',3);
text(rs(1)+5,rs(2)+5,num2str(rs(1),'(%.2fcm,'),'FontSize',12);
text(rs(1)+35,rs(2)+5,num2str(rs(2),'%.2fcm)'),'FontSize',12);
fprintf("声源坐标：(%.2fcm,%.2fcm)\n",rs(1),rs(2));
hold off;