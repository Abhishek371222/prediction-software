clc; clearvars; close all;

%% ── Parameters ────────────────────────────────────────────────────────────
rho    = 1.225;          % kg/m³  air density at 15 °C, sea level
Co     = 343;            % m/s    speed of sound
f      = 200;            % Hz
Vm     = 1;              % m/s    piston velocity amplitude
lambda = Co / f;         % m      wavelength
k      = 2*pi / lambda;  % rad/m  wavenumber
d      = 1.0;            % m      centre-to-centre speaker spacing
a      = 0.13;           % m      speaker (piston) radius
ka     = k * a;          % dimensionless wavenumber-radius product

%% ── Spatial grid ──────────────────────────────────────────────────────────
x = linspace(-1, 4, 700);
y = linspace(-2, 2, 700);
[X, Y] = meshgrid(x, y);

%% ── Speaker positions (both face +x direction) ────────────────────────────
x1 = 0;  y1 = -d/2;   % S1 — lower
x2 = 0;  y2 =  d/2;   % S2 — upper

%% ── Distances from each speaker ───────────────────────────────────────────
r1 = sqrt((X - x1).^2 + (Y - y1).^2);
r2 = sqrt((X - x2).^2 + (Y - y2).^2);
% Clamp at piston radius — prevents non-physical 1/r blow-up inside cone.
% Using a (not 0.05) because 0.05 m < a = 0.13 m, so the old clamp left
% an unphysical region between 5 cm and 13 cm from each speaker.
r1(r1 < a) = a;
r2(r2 < a) = a;

%% ── Angles from each speaker's boresight (+x axis) ───────────────────────
theta1 = atan2(Y - y1, X - x1);   % range (−π, π]
theta2 = atan2(Y - y2, X - x2);

%% ── Piston directivity  D(θ) = 2 J₁(ka sinθ) / (ka sinθ) ────────────────
xD1 = ka * sin(theta1);
xD2 = ka * sin(theta2);

D1 = 2 * besselj(1, xD1) ./ xD1;
D2 = 2 * besselj(1, xD2) ./ xD2;
% L'Hôpital limit:  lim_{x→0} 2J₁(x)/x = 1
D1(abs(xD1) < 1e-10) = 1;
D2(abs(xD2) < 1e-10) = 1;

%% ── Rear-hemisphere taper ─────────────────────────────────────────────────
% Logistic rolloff: ≈ 1 in front half-space, ≈ 0 behind ±90°.
% sigma scales sharpness with frequency (more directive at high ka).
sigma = max(1, ka);

rear_taper1 = 1 ./ (1 + exp(sigma * (abs(theta1) - pi/2)));
rear_taper2 = 1 ./ (1 + exp(sigma * (abs(theta2) - pi/2)));

D1 = abs(D1) .* rear_taper1;
D2 = abs(D2) .* rear_taper2;

% ── On-axis normalisation ──────────────────────────────────────────────────
% At theta = 0, rear_taper = 1/(1 + exp(−σπ/2)) < 1, so D_onaxis < 1.
% Dividing by this value restores D = 1 exactly on the boresight axis.
onaxis_taper = 1 / (1 + exp(-sigma * pi/2));
D1 = D1 / onaxis_taper;
D2 = D2 / onaxis_taper;

%% ── Pressure field  (far-field baffled-piston formula) ────────────────────
% p(r,θ) = (ρ c k a² Vm) / (2r) · D(θ) · e^{−jkr}
% The amplitude factor already contains the 1/r spherical spreading —
% no additional division by r is needed.
P1 = (rho*Co*k*a^2*Vm) ./ (2*r1) .* exp(-1j*k*r1) .* D1;
P2 = (rho*Co*k*a^2*Vm) ./ (2*r2) .* exp(-1j*k*r2) .* D2;
P  = P1 + P2;

%% ── Intensity (normalised, dB) ────────────────────────────────────────────
I   = abs(P).^2;
I   = I / max(I(:));          % normalise to peak = 0 dB
IdB = 10 * log10(I);
IdB(IdB < -40) = -40;         % floor at −40 dB

%% ── Plot ──────────────────────────────────────────────────────────────────
figure('Color','w','Position',[100 100 820 520])

contourf(X, Y, IdB, 50, 'LineColor','none')
colormap(jet)
clim([-40 0])                  % pin colour axis to match dB floor

hold on
plot(x1, y1, 'ko', 'MarkerFaceColor','w', 'MarkerSize',12, 'LineWidth',1.5)
plot(x2, y2, 'ko', 'MarkerFaceColor','w', 'MarkerSize',12, 'LineWidth',1.5)
text(x1 - 0.12, y1, 'S1', 'HorizontalAlignment','right', 'FontSize',10)
text(x2 - 0.12, y2, 'S2', 'HorizontalAlignment','right', 'FontSize',10)
hold off

axis equal
xlim([-1 4])
ylim([-2 2])                   % explicit y-limits — axis equal alone does not pin them
xlabel('x  (m)',  'FontSize',12)
ylabel('y  (m)',  'FontSize',12)
title(sprintf('Two-speaker radiation  |  f = %g Hz,  d = %.2f m,  a = %.3f m', ...
              f, d, a), 'FontSize',12)

cb = colorbar;
cb.Label.String = 'Intensity  (dB, normalised)';
cb.FontSize = 10;
