clc;
clear; close all;

% SPL heatmap from BEM field — 52 Hz only (verification)
data = readtable('52Hz.xlsx');

X  = data{:, 2};   % X1
Z  = data{:, 4};   % X3
Pr = data{:, 6};   % Pressure real
Pi = data{:, 7};   % Pressure Imaginary

Pm   = sqrt(Pr.^2 + Pi.^2);
Pref = 20e-6;
SPL  = 20 * log10(Pm / Pref);

Xn = unique(X);
Zn = unique(Z);
[Xm, Zm] = meshgrid(Xn, Zn);

valid = isfinite(X) & isfinite(Z) & isfinite(SPL);
X1   = X(valid);
Z1   = Z(valid);
SPL1 = SPL(valid);

SPLm = griddata(X1, Z1, SPL1, Xm, Zm);

figure;
pcolor(Xm, Zm, SPLm);
shading interp;
axis equal tight;
xlabel('X (m)');
ylabel('Z (m)');
title('BEM SPL Heatmap — 52 Hz');
colorbar;
colormap(jet);
