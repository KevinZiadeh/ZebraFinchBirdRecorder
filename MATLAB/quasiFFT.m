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

Ts = time(2) - time(1);
Fs = 1 / Ts;

k = 0:9999;
freq = k/(10000*Ts);    % Frequency vector in Hz (10000 frequencies from 0Hz to Sampling Frequency)

lengthinSecs = 1;
lengthinSamples = lengthinSecs * Fs; % Window length in samples from that in seconds

imax = (4*sz/lengthinSamples) - 1;   % maximum value of i which gives us the last block

status = zeros(sz, 1);  % The status vector which tells us if vocalization is NORMAL '0' or STUTTERING '1'
totalPowers = zeros(sz, 1);
notchedPowers = zeros(sz, 1);
referencePowers = zeros(sz, 1);
thresholds = zeros(sz, 1);

for i = 0:0.25:imax                 % Change interval jump i to select overlapping blocks of signal (0.5 for 50%, 0.1 for 90%)
    l = 1 + (lengthinSamples/4)*i;
    r_lim = 1 + (lengthinSamples/4)*(i + 0.25);
    r = (lengthinSamples/4)*(1 + i);
    firfiltered_temp = firfiltered(l:r, 1);
    [len, wid] = size(firfiltered_temp); 
    totalPower = sum(firfiltered_temp.^2);
    
    % Bandpass at 2.2 KHZ
    a0 = 0.4112132624576583;
    a1 = 0;
    a2 = -0.4112132624576583;
    b1 = 0.1842130766204379;
    b2 = 0.17757347508468332;
    
    var = 0;
    notchedSignal = zeros(len,1);

    for j = 3:1:len
        var = a0*firfiltered_temp(j,:) + a1*firfiltered_temp(j-1,:) + a2*firfiltered_temp(j-2,:) - b1*notchedSignal(j-1,:) - b2*notchedSignal(j-2,:);
        notchedSignal(j,:) = var;
    end
    
    notchedPower = sum(notchedSignal.^2);
    
    % Bandpass at 400 Hz
    a0 = 0.17932564232111428;
    a1 = 0;
    a2 = -0.17932564232111428;
    b1 = -1.5610153912536877;
    b2 = 0.6413487153577715;
    
    referenceSignal = zeros(len,1);

    for j = 3:1:len
        var = a0*firfiltered_temp(j,:) + a1*firfiltered_temp(j-1,:) + a2*firfiltered_temp(j-2,:) - b1*referenceSignal(j-1,:) - b2*referenceSignal(j-2,:);
        referenceSignal(j,:) = var;
    end
    
    referencePower = sum(referenceSignal.^2);
    
    % threshold  = abs(notchedPower - referencePower);
    threshold  = notchedPower/referencePower;
    
    totalPowers(l:r_lim, 1) = totalPower;
    notchedPowers(l:r_lim, 1) = notchedPower;
    referencePowers(l:r_lim, 1) = referencePower;
    thresholds(l:r_lim, 1) = threshold;
        
     if (threshold > 7)
         status(l:r_lim, 1) = 1;                 
     else
         status(l:r_lim, 1) = 0;                 
     end
end

%%
figure(1)
subplot(3,1,1)
spectrogram(firfiltered, blackman(256),250, 256, 8000, 'yaxis')
% plot(time, signal)
% title('Complete time signal')
% xlabel('time (s)')
% ylabel('magnitude')

subplot(3,1,2)
plot(time, firfiltered)
title('Complete time signal')
xlabel('time (s)')
ylabel('magnitude')

subplot(3,1,3)
plot(time, status, 'r')
title('Detector')
xlabel('time(s)')
ylabel('ON/OFF')

figure(2)
subplot(4, 1, 1)
plot(time, totalPowers)
xlabel('time(s)')
ylabel('total power')

subplot(4, 1, 2)
plot(time, notchedPowers)
xlabel('time(s)')
ylabel('notched power')

subplot(4, 1, 3)
plot(time, referencePowers)
xlabel('time(s)')
ylabel('REF power')

subplot(4, 1, 4)
plot(time, thresholds)
xlabel('time(s)')
ylabel('threshold')

figure(3)
subplot(3,1,1)
spectrogram(firfiltered, blackman(256),250, 256, 8000, 'yaxis')

subplot(3, 1, 2)
spectrogram(notchedSignal, blackman(256),250, 256, 8000, 'yaxis')

subplot(3, 1, 3)
spectrogram(referenceSignal, blackman(256),250, 256, 8000, 'yaxis')

figure(4)
subplot(3,1,1)
spectrogram(firfiltered, blackman(256),250, 256, 8000, 'yaxis')

subplot(3, 1, 2)
plot(time, thresholds)
xlabel('time(s)')
ylabel('threshold')

subplot(3,1,3)
plot(time, status, 'r')
title('Detector')
xlabel('time(s)')
ylabel('ON/OFF')
