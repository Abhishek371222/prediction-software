clc;
clear;
close all;

%% ================================================================
% Read BEM data
% Columns:
% 1 = Param
% 2 = x
% 3 = y
% 4 = z
% 5 = Real pressure
% 6 = Imaginary pressure
%% ================================================================

data = readtable('1057 Hz.xlsx');

x  = data{:,2};
y  = data{:,3};
z  = data{:,4};

Pr = data{:,5};
Pi = data{:,6};

%% ================================================================
% Calculate pressure magnitude and SPL
%% ================================================================

Pm = sqrt(Pr.^2 + Pi.^2);

Pref = 20e-6;

SPL = 20*log10(Pm/Pref);

%% ================================================================
% Remove invalid values
%% ================================================================

valid = isfinite(x) & ...
        isfinite(z) & ...
        isfinite(SPL);

x1   = x(valid);
z1   = z(valid);
SPL1 = SPL(valid);

%% ================================================================
% Create X-Z grid
%% ================================================================

x_unique = unique(x1);
z_unique = unique(z1);

[Xm, Zm] = meshgrid(x_unique, z_unique);

%% ================================================================
% Interpolate SPL onto regular grid
%% ================================================================

SPLm = griddata(x1, z1, SPL1, Xm, Zm, 'natural');

%% ================================================================
% Plot
%% ================================================================

figure;

pcolor(Xm, Zm, SPLm);

shading interp;

axis equal tight;

xlabel('X (m)');
ylabel('Z (m)');

title('BEM SPL Heatmap at 64 Hz');

colorbar;

colormap(jet);

%% ================================================================
% Dynamic color range
% Display 40 dB below maximum SPL
%% ================================================================

clim([max(SPL1)-40 max(SPL1)]);

%% ================================================================
% Optional: make axes readable
%% ================================================================

set(gca, 'FontSize', 12);