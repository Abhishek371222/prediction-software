clc; clearvars; close all;

File_1='Frequency_30_0.5Horizantal.xlsx';
File_2='Frequency_30_1Horizantal.xlsx';
File_3='Frequency_30_2Horizantal.xlsx';
raw_1=readmatrix(File_1);
raw_2=readmatrix(File_2);
raw_3=readmatrix(File_3);
% Sort angle data
[ang_sorted_1, idx_1] = sort(raw_1(:,1));
[ang_sorted_2, idx_2] = sort(raw_2(:,1));
[ang_sorted_3, idx_3] = sort(raw_3(:,1));
spl_sorted_1 = raw_1(idx_1,2);
spl_sorted_2 = raw_2(idx_2,2);
spl_sorted_3 = raw_3(idx_3,2);
% Normalize to 0 dB peak
spl_sorted_1 = spl_sorted_1 - max(spl_sorted_1);
spl_sorted_2 = spl_sorted_2 - max(spl_sorted_2);
spl_sorted_3 = spl_sorted_3 - max(spl_sorted_3);
r1 = 10.^(spl_sorted_1/20);
r2 = 10.^(spl_sorted_2/20);
r3 = 10.^(spl_sorted_3/20);
theta_1 = deg2rad(ang_sorted_1);
theta_2 = deg2rad(ang_sorted_2);
theta_3 = deg2rad(ang_sorted_3);
figure
hold on
polarplot(theta_1, abs(spl_sorted_1),'LineWidth',2)
polarplot(theta_2, abs(spl_sorted_2),'LineWidth',2)
polarplot(theta_3, abs(spl_sorted_3),'LineWidth',2)

title('80 Hz Radiation Pattern')
legend('Horizontal','Vertical')
rlim([0 40])       % radial limit (adjust)
% 
% figure
% hold on
% polarplot(theta_1,r1,'LineWidth',2)
% polarplot(theta_2,r1,'LineWidth',2)
% polarplot(theta_3,r1,'LineWidth',2)
% title('Horizontal Radiation Pattern')