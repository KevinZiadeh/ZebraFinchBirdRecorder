T = readtable('birdsongYT.csv','PreserveVariableNames',true);
T = T{:,:};
time = T(:, 1);
time = time + 16.384;
signal = T(:, 2);
[sz, col] = size(signal);

% signal = highpass(signal, 500, 8000);


a0 = 0.8948577513857248;
a1 = -1.7897155027714495;
a2 = 0.8948577513857248;
b1 = -1.7786300789392977;
b2 = 0.8008009266036016;

var = 0;
firfiltered = zeros(sz,1);

for i = 3:1:sz
    var = a0*signal(i,:) + a1*signal(i-1,:) + a2*signal(i-2,:) - b1*firfiltered(i-1,:) - b2*firfiltered(i-2,:);
    firfiltered(i,:) = var;
end

signal = firfiltered;

% ma_filtered = zeros(sz, 1);
% ma_filtered(1) = signal(1,:);
% EMA_a = 0.9;
% 
% for i = 2:1:sz
%     temp = (EMA_a*signal(i,:)) + ((1-EMA_a)*ma_filtered(i-1,:));
%     ma_filtered(i) = signal(i) - temp;
% end
% 
% signal = ma_filtered;

Ts = time(2) - time(1);
Fs = 1 / Ts;

k = 0:9999;
freq = k/(10000*Ts);    % Frequency vector in Hz (10000 frequencies from 0Hz to Sampling Frequency)

lengthinSecs = 1;
lengthinSamples = lengthinSecs * Fs; % Window length in samples from that in seconds

imax = (5*sz/lengthinSamples) - 1;   % maximum value of i which gives us the last block

status = zeros(sz, 1);  % The status vector which tells us if vocalization is NO VOC '0' or VOC '1'
nbPeaks = status;              

for i = 0:0.25:imax                 % Change interval jump i to select overlapping blocks of signal (0.5 for 50%, 0.1 for 90%)
    l = 1 + (lengthinSamples/5)*i;
    r = (lengthinSamples/5)*(1 + i);

    fftvf = fft(signal(l:r,1), 10000);         % Fourier of the ith block
    fftvfm = abs(fftvf);                    % Magnitude of the FT
    % fftvfm_eff = fftvfm(2000:5000);
    
    overall = sum(fftvfm);                  % Sum of all values of the FT
    percentage = fftvfm./overall;           % FT in percentage of total
    regOfInterest = percentage(2000:5000, :); % Only taking region of interest for peak calculation

    I = find(regOfInterest > 0.0005);        % Find all values above a threshold (experimentally determined 0.004)
    [impPeak, cols] = size(I);              % Get the number of peaks
    
    if (impPeak > 1)
        status(l:r, 1) = 1;                 
    else
        status(l:r, 1) = 0;                 
    end
end

figure(1)
subplot(3,1,1)
spectrogram(signal, blackman(256),250, 256, 8000, 'yaxis')
% plot(time, signal)
% title('Complete time signal')
% xlabel('time (s)')
% ylabel('magnitude')

subplot(3,1,2)
plot(time, signal)
title('Complete time signal')
xlabel('time (s)')
ylabel('magnitude')

subplot(3,1,3)
plot(time, status, 'r')
title('Detector')
xlabel('time(s)')
ylabel('ON/OFF')

%% If needed use this code to look into the ith block time and frequency content

i = 73;
l = 1 + (lengthinSamples/5)*i;
r = (lengthinSamples/5)*(1 + i);

fftvf = fft(signal(l:r,1), 10000);         % Fourier of the ith block
fftvfm = abs(fftvf);                    % Magnitude of the FT
% fftvfm_eff = fftvfm(2000:5000);
    
% overall = sum(fftvfm_eff);                  % Sum of all values of the FT
% percentage = fftvfm_eff./overall;           % FT in percentage of total

overall = sum(fftvfm);                  % Sum of all values of the FT
percentage = fftvfm./overall;           % FT in percentage of total


figure(2)
subplot(3,1,1)
plot(time(l:r, 1), signal(l:r, 1))
title('Windowed Accelerometer Signal')
xlabel('time (s)')
ylabel('amplitude')

subplot(3,1,2)
plot(freq(1, 1:6000), fftvfm(1:6000, 1))
title('DTFT of Windowed Accelerometer Signal')
xlabel('frequency (Hz)')
ylabel('Magnitude')

subplot(3,1,3)
plot(freq(1, 1:6000), percentage(1:6000, 1))
title('Percentage DTFT of Windowed Accelerometer Signal')
xlabel('frequency (Hz)')
ylabel('Magnitude Ratio')

