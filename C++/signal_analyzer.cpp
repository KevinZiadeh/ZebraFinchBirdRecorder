// #include <stdint.h>
// #include <windows.h>
// #include <iostream>

// using namespace std;

// bool analyze_signal(double* dp_filteredSignal, int i_singleBufferSize){
//     // will return 1 if signal contains vocalization, 0 if it doesn't
//     // First step is to find the total power in the signal
//     double totalPower = 0;
//     for (int i=0; i<i_singleBufferSize; i++){
//         totalPower += dp_filteredSignal[i]*dp_filteredSignal[i];
//     }

//     // Now to find the power around a certain frequency (to be determined when gathering actual values)
//     // we need to pass a band pass filter
//     // Band Pass at 2.2 kHz (for now)
    
//     // Assuming 6 kHz sampling frequency (ESP ADC Frequency)
//     double a0 = 0.3444719716111889;
//     double a1 = 0;
//     double a2 = -0.3444719716111889;
//     double b1 = 0.8772677342420642;
//     double b2 = 0.3110560567776222;

//     // Assuming 8 kHz sampling frequency (Analog Discovery Tool acquired)
//     /*
//     double a0 = 0.4112132624576583;
//     double a1 = 0;
//     double a2 = -0.4112132624576583;
//     double b1 = 0.1842130766204379;
//     double b2 = 0.17757347508468332;
//     */

//     double* dp_notchedSignal = (double*)malloc(sizeof(double)*i_singleBufferSize);

//     // Taking in initial values for now, might need to implement crossover (like filtering)
//     dp_notchedSignal[0] = dp_filteredSignal[0];
//     dp_notchedSignal[1] = dp_filteredSignal[1];

//     for (int i=2; i<i_singleBufferSize; i++){
//         dp_notchedSignal[i] = a0*dp_filteredSignal[i] + a1*dp_filteredSignal[i-1] + a2*dp_filteredSignal[i-2] - b1*dp_notchedSignal[i-1] - b2*dp_notchedSignal[i-2];
//     }

//     // Find power in the filtered signal (i.e. Power at 2 kHz)
//     double notchedPower = 0;

//     for (int i=0; i<i_singleBufferSize; i++){
//         notchedPower += dp_notchedSignal[i]*dp_notchedSignal[i];
//     }


// //////////////////////////////////////////////////////////////////
//     // Now to find the power around another reference frequency (again to be determined when gathering actual values)
//     // as it is needed for comparison purposes
//     // Bandpass at 400 Hz (for now)

//     // Assuming 6 kHz sampling frequency (ESP ADC Frequency)
//     a0 = 0.22336671878312517;
//     a1 = 0;
//     a2 = -0.22336671878312517;
//     b1 = -1.4189796126194893;
//     b2 = 0.5532665624337496;

//     // Assuming 8 kHz sampling frequency (Analog Discovery Tool acquired)
//     /*
//     a0 = 0.17932564232111428;
//     a1 = 0;
//     a2 = -0.17932564232111428;
//     b1 = -1.5610153912536877;
//     b2 = 0.6413487153577715;
//     */
    
//     // Reusing/Resetting notchedSignal array as to not waste memory
//     dp_notchedSignal[0] = dp_filteredSignal[0];
//     dp_notchedSignal[1] = dp_filteredSignal[1];

//     for (int i=2; i<i_singleBufferSize; i++){
//         dp_notchedSignal[i] = a0*dp_filteredSignal[i] + a1*dp_filteredSignal[i-1] + a2*dp_filteredSignal[i-2] - b1*dp_notchedSignal[i-1] - b2*dp_notchedSignal[i-2];
//     }

//     // Find power in the reference notched signal (i.e. Power at 400 Hz)
//     double notchedReferencePower = 0;

//     for (int i=0; i<i_singleBufferSize; i++){
//         notchedReferencePower += dp_notchedSignal[i]*dp_notchedSignal[i];
//     }

//     double threshold = 0; // to be determined experimentally
//     double notchedRatio = notchedPower / totalPower;
//     double notchedReferenceRatio = notchedReferencePower / totalPower;
//     if(abs(notchedRatio - notchedReferenceRatio) >= threshold ){ return true;}
//     else{ return false;}

// }