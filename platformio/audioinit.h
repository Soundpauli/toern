#include "src/mixer_limited.h"
// Audio Processing Nodes
// Audio graph routing, small mixers and CH11 DSP state are hot every block.
// Keep their state in DTCM; only large sample and delay storage stays in PSRAM.
// CH11 oscillator/mixer/ladder/envelope state is hot on every audio block.
// Keep these small objects in internal DTCM: PSRAM cache misses multiply with
// three-note stereo chords. Large sample/delay buffers remain in EXTMEM.
AudioSynthWaveform              waveform11_1;   //xy=405,1269
AudioSynthWaveform              waveform11_2;   //xy=405,1310
AudioEffectEnvelope             envelope11;     //xy=412,1351
AudioSynthWaveform              waveform13_1;   //xy=409,1582
AudioSynthWaveform              waveform13_2;   //xy=408,1626
AudioSynthWaveform              waveform14_1;   //xy=421,1766
AudioSynthWaveform              waveform14_2;   //xy=427,1810
AudioSynthWaveform              Swaveform2_2; //xy=495,2275
AudioSynthWaveform              Swaveform3_0; //xy=475,2345
AudioSynthWaveform              Swaveform3_1; //xy=490,2400
AudioSynthWaveform              Swaveform3_2; //xy=490,2460
AudioSynthWaveformDc            Sdc1_0; //xy=460,2550
AudioSynthWaveformDc            Sdc1_1; //xy=465,2605
AudioSynthWaveformDc            Sdc1_2; //xy=460,2665
AudioSynthWaveform              Swaveform1_0; //xy=520,1970
AudioSynthWaveform              Swaveform1_1; //xy=525,2030
AudioSynthWaveform              Swaveform1_2; //xy=525,2085
AudioSynthWaveform              Swaveform2_0; //xy=505,2150
AudioSynthWaveform              Swaveform2_1; //xy=515,2215
AudioMixer4                     mixer_waveform11; //xy=636,1304
AudioMixer4                     mixer_waveform13; //xy=641,1611
AudioMixer4                     mixer_waveform14; //xy=658,1781
AudioEffectEnvelope             SenvelopeFilter1_0; //xy=675,2560
AudioEffectEnvelope             SenvelopeFilter1_1; //xy=680,2620
AudioEffectEnvelope             SenvelopeFilter1_2; //xy=685,2670
AudioPlayArrayResmp             sound8;         //xy=990,745
AudioEffectEnvelope             envelope13;     //xy=1073,1610
AudioEffectEnvelope             envelope14;     //xy=1082,1784
AudioMixer4                     Smixer1_0; //xy=1130,2055
AudioMixer4                     Smixer1_1; //xy=1130,2140
AudioMixer4                     Smixer1_2; //xy=1130,2215
AudioMixer4                     Smixer2_0; //xy=1135,2370
AudioMixer4                     Smixer2_1; //xy=1135,2455
AudioMixer4                     Smixer2_2; //xy=1140,2565
AudioEffectBitcrusher           bitcrusher11;   //xy=1249,1294
AudioEffectBitcrusher           bitcrusher13;   //xy=1251,1612
AudioEffectBitcrusher           bitcrusher14;   //xy=1271,1782
AudioFilterLadder               Sladder1_0; //xy=1365,2105
AudioFilterLadder               Sladder1_1; //xy=1365,2175
AudioFilterLadder               Sladder1_2; //xy=1370,2245
AudioFilterLadder               Sladder2_0; //xy=1360,2430
AudioFilterLadder               Sladder2_1; //xy=1365,2510
AudioFilterLadder               Sladder2_2; //xy=1365,2580
AudioAmplifier                  amp11;          //xy=1421,1295
AudioAmplifier                  amp13;          //xy=1409,1612
AudioAmplifier                  amp14;          //xy=1439,1783
AudioPlayArrayResmp             sound1;         //xy=1560,395
AudioPlayArrayResmp             sound2;         //xy=1570,560
AudioPlayArrayResmp             sound3;         //xy=1555,685
AudioFilterStateVariable        filter11;       //xy=1581,1302
AudioFilterStateVariable        filter13;       //xy=1562,1616
AudioFilterStateVariable        filter14;       //xy=1584,1790
AudioEffectEnvelope             Senvelope1_0; //xy=1555,2115
AudioEffectEnvelope             Senvelope1_1; //xy=1560,2175
AudioEffectEnvelope             Senvelope1_2; //xy=1560,2240
AudioEffectEnvelope             Senvelope2_0; //xy=1545,2460
AudioEffectEnvelope             Senvelope2_1; //xy=1545,2525
AudioEffectEnvelope             Senvelope2_2; //xy=1555,2585
AudioPlayArrayResmp             sound4;         //xy=1715,795
AudioPlayArrayResmp             sound5;         //xy=1720,865
AudioPlayArrayResmp             sound6;         //xy=1720,935
AudioPlayArrayResmp             sound7;         //xy=1750,1005
AudioMixer4                     filtermixer11;  //xy=1750,1309
AudioMixer4                     filtermixer13;  //xy=1736,1622
AudioMixer4                     filtermixer14;  //xy=1746,1796
AudioMixer4                     SmixerL4; //xy=1785,2195

AudioMixer4                     SmixerR4; //xy=1815,2530
AudioEffectEnvelope             envelope1;      //xy=1927,516
AudioEffectEnvelope             envelope2;      //xy=1928,626
AudioEffectEnvelope             envelope3;      //xy=1926,717
AudioEffectEnvelope             envelope4;      //xy=1927,794
AudioEffectEnvelope             envelope5;      //xy=1926,864
AudioEffectEnvelope             envelope6;      //xy=1923,942
AudioEffectEnvelope             envelope7;      //xy=1924,1025
AudioEffectEnvelope             envelope8;      //xy=1922,1102
AudioEffectTreeverb             freeverb11;     //xy=1922,1384
// DLAY 0 = off; N = N/32 of a beat (1,2,4,8,16,32 land on even divisions). Line ~1.3 s.
#define VOICE_DELAY_SAMPLES 57600u
#define VOICE_DELAY_COUNT 11
EXTMEM int16_t voiceDelayStore[VOICE_DELAY_COUNT][VOICE_DELAY_SAMPLES];
AudioEffectDelayExternal voiceDelay[VOICE_DELAY_COUNT];
AudioMixer4Ramp voiceDelayFb[VOICE_DELAY_COUNT];
AudioMixer4Ramp voiceDelayMix[VOICE_DELAY_COUNT];
// Independent post-FX trim/drive, sample voices 1–8 only.
AudioAmplifier sampleFilterGain[8];
static inline int voiceDelayIndex(int ch) {
  if (ch >= 1 && ch <= 8) return ch - 1;
  if (ch == 11) return 8;
  if (ch == 13) return 9;
  if (ch == 14) return 10;
  return -1;
}
static inline bool channelHasVoiceDelay(int ch) { return voiceDelayIndex(ch) >= 0; }

AudioMixer4                     CHMixer11; //xy=1945,2340
AudioEffectBitcrusher           bitcrusher4;    //xy=2098,793
AudioEffectBitcrusher           bitcrusher5;    //xy=2098,864
AudioEffectBitcrusher           bitcrusher6;    //xy=2093,942
AudioEffectBitcrusher           bitcrusher7;    //xy=2093,1025
AudioEffectBitcrusher           bitcrusher8;    //xy=2094,1102
AudioEffectBitcrusher           bitcrusher1;    //xy=2100,516
AudioEffectBitcrusher           bitcrusher2;    //xy=2100,626
AudioEffectBitcrusher           bitcrusher3;    //xy=2103,718
AudioMixer4                     synthmixer11;   //xy=2120,1326
AudioMixer4                     synthmixer13;   //xy=2119,1623
AudioMixer4                     synthmixer14;   //xy=2114,1802
AudioPlayArrayResmp             sound0;         //xy=2255,365
AudioAmplifier                  amp1;           //xy=2279,515
AudioAmplifier                  amp2;           //xy=2278,628
AudioAmplifier                  amp3;           //xy=2274,718
AudioAmplifier                  amp4;           //xy=2273,794
AudioAmplifier                  amp5;           //xy=2273,865
AudioAmplifier                  amp6;           //xy=2272,943
AudioAmplifier                  amp7;           //xy=2269,1024
AudioAmplifier                  amp8;           //xy=2268,1101
AudioFilterStateVariable        filter1;        //xy=2432,521
AudioFilterStateVariable        filter2;        //xy=2437,633
AudioFilterStateVariable        filter3;        //xy=2437,724
AudioFilterStateVariable        filter4;        //xy=2436,801
AudioFilterStateVariable        filter5;        //xy=2437,875
AudioFilterStateVariable        filter6;        //xy=2434,950
AudioFilterStateVariable        filter7;        //xy=2434,1029
AudioFilterStateVariable        filter8;        //xy=2431,1107
AudioMixer4                     filtermixer1;   //xy=2604,528
AudioMixer4                     filtermixer2;   //xy=2611,639
AudioMixer4                     filtermixer3;   //xy=2611,730
AudioMixer4                     filtermixer4;   //xy=2608,807
AudioMixer4                     filtermixer5;   //xy=2609,883
AudioMixer4                     filtermixer6;   //xy=2608,958
AudioMixer4                     filtermixer7;   //xy=2601,1034
AudioMixer4                     filtermixer8;   //xy=2603,1111
AudioEffectTreeverb             freeverb1;      //xy=2750,488
AudioEffectTreeverb             freeverb2;      //xy=2761,627
AudioEffectTreeverb             freeverb4;      //xy=2759,998
AudioEffectTreeverb             freeverb6;      //xy=2765,108
AudioEffectTreeverb             freeverb7;      //xy=2759,998
AudioEffectTreeverb             freeverb8;      //xy=2765,1083
AudioMixer4Limited                     mixersynth_end; //xy=2889,1562
AudioMixer4                     freeverbmixer1; //xy=2964,513
AudioMixer4                     freeverbmixer2; //xy=2978,657
AudioMixer4                     freeverbmixer4; //xy=2970,912
AudioMixer4                     freeverbmixer6; //xy=2970,1004
AudioMixer4                     freeverbmixer7; //xy=2970,1016
AudioMixer4                     freeverbmixer8; //xy=2970,1108
AudioMixer4Limited                     mixer1;         //xy=3200,763
AudioMixer4Limited                     mixer2;         //xy=3200,948
AudioEffectEnvelope             envelope0;      //xy=3524,426
AudioPlaySdWav                  playSdWav1;     //xy=3524,474
AudioMixer4Limited                     mixer_end;      //xy=3518,1059
AudioInputI2S                   audioInput;     //xy=3556,1189
AudioAmplifier                  audioInputAmp;  //xy=3650,1189
AudioFilterBiquad               recInputHpf;    // library DC block; codec ADC HPF stays off
AudioMixer4                     recInputHpfMix; // in0 dry, in1 high-pass
AudioMixer4                     mixer0;         //xy=3722,478
AudioAnalyzePeak                peak1;          //xy=3727,541
// mixerPlay removed: mixer_end now feeds mixer_stereoR/L directly
AudioRecordQueue                queue1;         //xy=3778,1164
AudioAnalyzePeak                peakRec;        //xy=3788,1215
AudioAnalyzePeak                peakOutput;     //xy=3788,1300  // For LED strip visualization
AudioMixer4Limited                     mixer_stereoR;  //xy=3932,748
AudioMixer4Limited                     mixer_stereoL;  //xy=3931,848
AudioOutputI2S                  i2s1;           //xy=4154,795
AudioOutputUSB                  usb1;           // host USB audio (same mix as headphones)

// Audio Connections (all connections (aka wires or links))
AudioConnection        patchCord13(waveform11_1, 0, mixer_waveform11, 0);
AudioConnection        patchCord14(waveform11_2, 0, mixer_waveform11, 1);
AudioConnection        patchCord15(envelope11, 0, mixer_waveform11, 2);
AudioConnection        patchCord16(waveform13_1, 0, mixer_waveform13, 0);
AudioConnection        patchCord17(waveform13_2, 0, mixer_waveform13, 1);
AudioConnection        patchCord18(waveform14_1, 0, mixer_waveform14, 0);
AudioConnection        patchCord19(waveform14_2, 0, mixer_waveform14, 1);
AudioConnection        patchCord20(Swaveform2_2, 0, Smixer1_2, 1);
AudioConnection        patchCord21(Swaveform2_2, 0, Smixer2_2, 1);
AudioConnection        patchCord22(Swaveform3_0, 0, Smixer1_0, 2);
AudioConnection        patchCord23(Swaveform3_0, 0, Smixer2_0, 2);
AudioConnection        patchCord24(Swaveform3_1, 0, Smixer1_1, 2);
AudioConnection        patchCord25(Swaveform3_1, 0, Smixer2_1, 2);
AudioConnection        patchCord26(Swaveform3_2, 0, Smixer1_2, 2);
AudioConnection        patchCord27(Swaveform3_2, 0, Smixer2_2, 2);
AudioConnection        patchCord28(Sdc1_0, 0, SenvelopeFilter1_0, 0);
AudioConnection        patchCord29(Sdc1_1, 0, SenvelopeFilter1_1, 0);
AudioConnection        patchCord30(Sdc1_2, 0, SenvelopeFilter1_2, 0);
AudioConnection        patchCord36(Swaveform1_0, 0, Smixer1_0, 0);
AudioConnection        patchCord37(Swaveform1_0, 0, Smixer2_0, 0);
AudioConnection        patchCord38(Swaveform1_1, 0, Smixer1_1, 0);
AudioConnection        patchCord39(Swaveform1_1, 0, Smixer2_1, 0);
AudioConnection        patchCord40(Swaveform1_2, 0, Smixer1_2, 0);
AudioConnection        patchCord41(Swaveform1_2, 0, Smixer2_2, 0);
AudioConnection        patchCord42(Swaveform2_0, 0, Smixer1_0, 1);
AudioConnection        patchCord43(Swaveform2_0, 0, Smixer2_0, 1);
AudioConnection        patchCord44(Swaveform2_1, 0, Smixer1_1, 1);
AudioConnection        patchCord45(Swaveform2_1, 0, Smixer2_1, 1);
AudioConnection        patchCord46(mixer_waveform11, 0, bitcrusher11, 0);
AudioConnection        patchCord47(mixer_waveform13, 0, envelope13, 0);
AudioConnection        patchCord48(mixer_waveform14, 0, envelope14, 0);
AudioConnection        patchCord49(SenvelopeFilter1_0, 0, Sladder1_0, 1);
AudioConnection        patchCord50(SenvelopeFilter1_0, 0, Sladder2_0, 1);
AudioConnection        patchCord51(SenvelopeFilter1_1, 0, Sladder1_1, 1);
AudioConnection        patchCord52(SenvelopeFilter1_1, 0, Sladder2_1, 1);
AudioConnection        patchCord53(SenvelopeFilter1_2, 0, Sladder1_2, 1);
AudioConnection        patchCord54(SenvelopeFilter1_2, 0, Sladder2_2, 1);
AudioConnection        patchCord55(Smixer1_0, 0, Sladder1_0, 0);
AudioConnection        patchCord56(Smixer1_1, 0, Sladder1_1, 0);
AudioConnection        patchCord57(Smixer1_2, 0, Sladder1_2, 0);
AudioConnection        patchCord58(Smixer2_0, 0, Sladder2_0, 0);
AudioConnection        patchCord59(Smixer2_1, 0, Sladder2_1, 0);
AudioConnection        patchCord60(Smixer2_2, 0, Sladder2_2, 0);
AudioConnection        patchCord61(sound8, 0, envelope8, 0);
AudioConnection        patchCord62(envelope13, 0, bitcrusher13, 0);
AudioConnection        patchCord63(envelope14, 0, bitcrusher14, 0);
AudioConnection        patchCord85(bitcrusher11, 0, amp11, 0);
AudioConnection        patchCord86(bitcrusher13, 0, amp13, 0);
AudioConnection        patchCord87(bitcrusher14, 0, amp14, 0);
AudioConnection        patchCord88(Sladder1_0, 0, Senvelope1_0, 0);
AudioConnection        patchCord89(Sladder1_1, 0, Senvelope1_1, 0);
AudioConnection        patchCord90(Sladder1_2, 0, Senvelope1_2, 0);
AudioConnection        patchCord91(Sladder2_0, 0, Senvelope2_0, 0);
AudioConnection        patchCord92(Sladder2_1, 0, Senvelope2_1, 0);
AudioConnection        patchCord93(Sladder2_2, 0, Senvelope2_2, 0);
AudioConnection        patchCord97(amp11, 0, filter11, 0);
AudioConnection        patchCord98(amp13, 0, filter13, 0);
AudioConnection        patchCord99(amp14, 0, filter14, 0);
AudioConnection        patchCord100(sound1, 0, envelope1, 0);
AudioConnection        patchCord101(sound2, 0, envelope2, 0);
AudioConnection        patchCord102(sound3, 0, envelope3, 0);
AudioConnection        patchCord103(filter11, 0, filtermixer11, 0);
AudioConnection        patchCord104(filter11, 1, filtermixer11, 1);
AudioConnection        patchCord105(filter11, 2, filtermixer11, 2);
AudioConnection        patchCord106(filter13, 0, filtermixer13, 0);
AudioConnection        patchCord107(filter13, 1, filtermixer13, 1);
AudioConnection        patchCord108(filter13, 2, filtermixer13, 2);
AudioConnection        patchCord109(filter14, 0, filtermixer14, 0);
AudioConnection        patchCord110(filter14, 1, filtermixer14, 1);
AudioConnection        patchCord111(filter14, 2, filtermixer14, 2);
AudioConnection        patchCord112(Senvelope1_0, 0, SmixerL4, 0);
AudioConnection        patchCord113(Senvelope1_1, 0, SmixerL4, 1);
AudioConnection        patchCord114(Senvelope1_2, 0, SmixerL4, 2);
AudioConnection        patchCord115(Senvelope2_0, 0, SmixerR4, 0);
AudioConnection        patchCord116(Senvelope2_1, 0, SmixerR4, 1);
AudioConnection        patchCord117(Senvelope2_2, 0, SmixerR4, 2);
AudioConnection        patchCord121(sound4, 0, envelope4, 0);
AudioConnection        patchCord122(sound5, 0, envelope5, 0);
AudioConnection        patchCord123(sound6, 0, envelope6, 0);
AudioConnection        patchCord124(sound7, 0, envelope7, 0);
AudioConnection        patchCord125(filtermixer11, 0, freeverb11, 0);
AudioConnection        patchCord126(filtermixer11, 0, synthmixer11, 3);
//EXTMEM AudioConnection        patchCord128(filtermixer13, 0, freeverb13, 0);
AudioConnection        patchCord129(filtermixer13, 0, synthmixer13, 3);
//EXTMEM AudioConnection        patchCord131(filtermixer14, 0, freeverb14, 0);
AudioConnection        patchCord132(filtermixer14, 0, synthmixer14, 3);
AudioConnection        patchCord134(SmixerL4, 0, CHMixer11, 0);
//EXTMEM AudioConnection        patchCord135(freeverb13, 0, synthmixer13, 0);
AudioConnection        patchCord136(SmixerR4, 0, CHMixer11, 1);
AudioConnection        patchCord137(envelope1, 0, bitcrusher1, 0);
AudioConnection        patchCord138(envelope2, 0, bitcrusher2, 0);
AudioConnection        patchCord139(envelope3, 0, bitcrusher3, 0);
AudioConnection        patchCord140(envelope4, 0, bitcrusher4, 0);
AudioConnection        patchCord141(envelope5, 0, bitcrusher5, 0);
AudioConnection        patchCord142(envelope6, 0, bitcrusher6, 0);
AudioConnection        patchCord143(envelope7, 0, bitcrusher7, 0);
AudioConnection        patchCord144(envelope8, 0, bitcrusher8, 0);
AudioConnection        patchCord145(freeverb11, 0, synthmixer11, 0);
//EXTMEM AudioConnection        patchCord146(freeverb14, 0, synthmixer14, 0);
AudioConnection        patchCord147(CHMixer11, 0, mixer_waveform11, 3);
AudioConnection        patchCord148(bitcrusher4, 0, amp4, 0);
AudioConnection        patchCord149(bitcrusher5, 0, amp5, 0);
AudioConnection        patchCord150(bitcrusher6, 0, amp6, 0);
AudioConnection        patchCord151(bitcrusher7, 0, amp7, 0);
AudioConnection        patchCord152(bitcrusher8, 0, amp8, 0);
AudioConnection        patchCord153(bitcrusher1, 0, amp1, 0);
AudioConnection        patchCord154(bitcrusher2, 0, amp2, 0);
AudioConnection        patchCord155(bitcrusher3, 0, amp3, 0);
AudioConnection        patchCord156a(synthmixer11, 0, voiceDelayFb[8], 0);
AudioConnection        patchCord156fb(voiceDelay[8], 0, voiceDelayFb[8], 1);
AudioConnection        patchCord156in(voiceDelayFb[8], 0, voiceDelay[8], 0);
AudioConnection        patchCord156b(voiceDelay[8], 0, voiceDelayMix[8], 0);
AudioConnection        patchCord156c(synthmixer11, 0, voiceDelayMix[8], 3);
AudioConnection        patchCord156(voiceDelayMix[8], 0, mixersynth_end, 0);
AudioConnection        patchCord157a(synthmixer13, 0, voiceDelayFb[9], 0);
AudioConnection        patchCord157fb(voiceDelay[9], 0, voiceDelayFb[9], 1);
AudioConnection        patchCord157in(voiceDelayFb[9], 0, voiceDelay[9], 0);
AudioConnection        patchCord157b(voiceDelay[9], 0, voiceDelayMix[9], 0);
AudioConnection        patchCord157c(synthmixer13, 0, voiceDelayMix[9], 3);
AudioConnection        patchCord157(voiceDelayMix[9], 0, mixersynth_end, 2);
AudioConnection        patchCord158a(synthmixer14, 0, voiceDelayFb[10], 0);
AudioConnection        patchCord158fb(voiceDelay[10], 0, voiceDelayFb[10], 1);
AudioConnection        patchCord158in(voiceDelayFb[10], 0, voiceDelay[10], 0);
AudioConnection        patchCord158b(voiceDelay[10], 0, voiceDelayMix[10], 0);
AudioConnection        patchCord158c(synthmixer14, 0, voiceDelayMix[10], 3);
AudioConnection        patchCord158(voiceDelayMix[10], 0, mixersynth_end, 3);
AudioConnection        patchCord159(sound0, 0, envelope0, 0);
AudioConnection        patchCord160(amp1, 0, filter1, 0);
AudioConnection        patchCord161(amp2, 0, filter2, 0);
AudioConnection        patchCord162(amp3, 0, filter3, 0);
AudioConnection        patchCord163(amp4, 0, filter4, 0);
AudioConnection        patchCord164(amp5, 0, filter5, 0);
AudioConnection        patchCord165(amp6, 0, filter6, 0);
AudioConnection        patchCord166(amp7, 0, filter7, 0);
AudioConnection        patchCord167(amp8, 0, filter8, 0);
AudioConnection patchSampleDry1(amp1, 0, filtermixer1, 3);
AudioConnection patchSampleDry2(amp2, 0, filtermixer2, 3);
AudioConnection patchSampleDry3(amp3, 0, filtermixer3, 3);
AudioConnection patchSampleDry4(amp4, 0, filtermixer4, 3);
AudioConnection patchSampleDry5(amp5, 0, filtermixer5, 3);
AudioConnection patchSampleDry6(amp6, 0, filtermixer6, 3);
AudioConnection patchSampleDry7(amp7, 0, filtermixer7, 3);
AudioConnection patchSampleDry8(amp8, 0, filtermixer8, 3);
//test granular
//AudioEffectGranular      granular1;      //xy=504,155
//EXTMEM AudioConnection        patchCord167a(amp8, 0, granular1, 0);
//EXTMEM AudioConnection        patchCord167b(granular1, 0, filter8, 0);

AudioConnection        patchCord168(filter1, 0, filtermixer1, 0);
AudioConnection        patchCord169(filter1, 1, filtermixer1, 1);
AudioConnection        patchCord170(filter1, 2, filtermixer1, 2);
AudioConnection        patchCord171(filter2, 0, filtermixer2, 0);
AudioConnection        patchCord172(filter2, 1, filtermixer2, 1);
AudioConnection        patchCord173(filter2, 2, filtermixer2, 2);
AudioConnection        patchCord174(filter3, 0, filtermixer3, 0);
AudioConnection        patchCord175(filter3, 1, filtermixer3, 1);
AudioConnection        patchCord176(filter3, 2, filtermixer3, 2);
AudioConnection        patchCord177(filter4, 0, filtermixer4, 0);
AudioConnection        patchCord178(filter4, 1, filtermixer4, 1);
AudioConnection        patchCord179(filter4, 2, filtermixer4, 2);
AudioConnection        patchCord180(filter5, 0, filtermixer5, 0);
AudioConnection        patchCord181(filter5, 1, filtermixer5, 1);
AudioConnection        patchCord182(filter5, 2, filtermixer5, 2);
AudioConnection        patchCord183(filter6, 0, filtermixer6, 0);
AudioConnection        patchCord184(filter6, 1, filtermixer6, 1);
AudioConnection        patchCord185(filter6, 2, filtermixer6, 2);
AudioConnection        patchCord186(filter7, 0, filtermixer7, 0);
AudioConnection        patchCord187(filter7, 1, filtermixer7, 1);
AudioConnection        patchCord188(filter7, 2, filtermixer7, 2);
AudioConnection        patchCord189(filter8, 0, filtermixer8, 0);
AudioConnection        patchCord190(filter8, 1, filtermixer8, 1);
AudioConnection        patchCord191(filter8, 2, filtermixer8, 2);
AudioConnection        patchCord192(filtermixer1, 0, freeverb1, 0);
AudioConnection        patchCord193(filtermixer1, 0, freeverbmixer1, 3);
AudioConnection        patchCord194(filtermixer2, 0, freeverb2, 0);
AudioConnection        patchCord195(filtermixer2, 0, freeverbmixer2, 3);
AudioConnection        patchCord196a(filtermixer3, 0, voiceDelayFb[2], 0);
AudioConnection        patchCord196fb(voiceDelay[2], 0, voiceDelayFb[2], 1);
AudioConnection        patchCord196in(voiceDelayFb[2], 0, voiceDelay[2], 0);
AudioConnection        patchCord196b(voiceDelay[2], 0, voiceDelayMix[2], 0);
AudioConnection        patchCord196c(filtermixer3, 0, voiceDelayMix[2], 3);
AudioConnection        patchCord196(voiceDelayMix[2], 0, sampleFilterGain[2], 0);
AudioConnection        patchCord196Gain(sampleFilterGain[2], 0, mixer1, 2);
AudioConnection        patchCord197a(freeverbmixer4, 0, voiceDelayFb[3], 0);
AudioConnection        patchCord197fb(voiceDelay[3], 0, voiceDelayFb[3], 1);
AudioConnection        patchCord197in(voiceDelayFb[3], 0, voiceDelay[3], 0);
AudioConnection        patchCord197b(voiceDelay[3], 0, voiceDelayMix[3], 0);
AudioConnection        patchCord197c(freeverbmixer4, 0, voiceDelayMix[3], 3);
AudioConnection        patchCord197(voiceDelayMix[3], 0, sampleFilterGain[3], 0);
AudioConnection        patchCord197Gain(sampleFilterGain[3], 0, mixer1, 3);
AudioConnection        patchCord198(filtermixer4, 0, freeverb4, 0);
AudioConnection        patchCord198b(filtermixer4, 0, freeverbmixer4, 3);
AudioConnection        patchCord199(filtermixer6, 0, freeverb6, 0);
AudioConnection        patchCord199b(filtermixer6, 0, freeverbmixer6, 3);
AudioConnection        patchCord200(filtermixer7, 0, freeverb7, 0);
AudioConnection        patchCord201(filtermixer7, 0, freeverbmixer7, 3);
AudioConnection        patchCord202(filtermixer8, 0, freeverb8, 0);
AudioConnection        patchCord203(filtermixer8, 0, freeverbmixer8, 3);
AudioConnection        patchCord204(freeverb1, 0, freeverbmixer1, 0);
AudioConnection        patchCord205(freeverb2, 0, freeverbmixer2, 0);
AudioConnection        patchCord205b(freeverb4, 0, freeverbmixer4, 0);
AudioConnection        patchCord205c(freeverb6, 0, freeverbmixer6, 0);
AudioConnection        patchCord206(freeverb7, 0, freeverbmixer7, 0);
AudioConnection        patchCord207(freeverb8, 0, freeverbmixer8, 0);
AudioConnection        patchCord208(mixersynth_end, 0, mixer_end, 2);
AudioConnection        patchCord209a(freeverbmixer1, 0, voiceDelayFb[0], 0);
AudioConnection        patchCord209fb(voiceDelay[0], 0, voiceDelayFb[0], 1);
AudioConnection        patchCord209in(voiceDelayFb[0], 0, voiceDelay[0], 0);
AudioConnection        patchCord209b(voiceDelay[0], 0, voiceDelayMix[0], 0);
AudioConnection        patchCord209c(freeverbmixer1, 0, voiceDelayMix[0], 3);
AudioConnection        patchCord209(voiceDelayMix[0], 0, sampleFilterGain[0], 0);
AudioConnection        patchCord209Gain(sampleFilterGain[0], 0, mixer1, 0);
AudioConnection        patchCord210a(freeverbmixer2, 0, voiceDelayFb[1], 0);
AudioConnection        patchCord210fb(voiceDelay[1], 0, voiceDelayFb[1], 1);
AudioConnection        patchCord210in(voiceDelayFb[1], 0, voiceDelay[1], 0);
AudioConnection        patchCord210b2(voiceDelay[1], 0, voiceDelayMix[1], 0);
AudioConnection        patchCord210c2(freeverbmixer2, 0, voiceDelayMix[1], 3);
AudioConnection        patchCord210(voiceDelayMix[1], 0, sampleFilterGain[1], 0);
AudioConnection        patchCord210Gain(sampleFilterGain[1], 0, mixer1, 1);
AudioConnection        patchCord210b(filtermixer5, 0, voiceDelayFb[4], 0);
AudioConnection        patchCordDly5fb(voiceDelay[4], 0, voiceDelayFb[4], 1);
AudioConnection        patchCordDly5in(voiceDelayFb[4], 0, voiceDelay[4], 0);
AudioConnection        patchCordDly5w(voiceDelay[4], 0, voiceDelayMix[4], 0);
AudioConnection        patchCordDly5d(filtermixer5, 0, voiceDelayMix[4], 3);
AudioConnection        patchCordDly5o(voiceDelayMix[4], 0, sampleFilterGain[4], 0);
AudioConnection        patchCordDly5oGain(sampleFilterGain[4], 0, mixer2, 0);
AudioConnection        patchCord210c(freeverbmixer6, 0, voiceDelayFb[5], 0);
AudioConnection        patchCordDly6fb(voiceDelay[5], 0, voiceDelayFb[5], 1);
AudioConnection        patchCordDly6in(voiceDelayFb[5], 0, voiceDelay[5], 0);
AudioConnection        patchCordDly6w(voiceDelay[5], 0, voiceDelayMix[5], 0);
AudioConnection        patchCordDly6d(freeverbmixer6, 0, voiceDelayMix[5], 3);
AudioConnection        patchCordDly6o(voiceDelayMix[5], 0, sampleFilterGain[5], 0);
AudioConnection        patchCordDly6oGain(sampleFilterGain[5], 0, mixer2, 1);
AudioConnection        patchCord211(freeverbmixer7, 0, voiceDelayFb[6], 0);
AudioConnection        patchCordDly7fb(voiceDelay[6], 0, voiceDelayFb[6], 1);
AudioConnection        patchCordDly7in(voiceDelayFb[6], 0, voiceDelay[6], 0);
AudioConnection        patchCordDly7w(voiceDelay[6], 0, voiceDelayMix[6], 0);
AudioConnection        patchCordDly7d(freeverbmixer7, 0, voiceDelayMix[6], 3);
AudioConnection        patchCordDly7o(voiceDelayMix[6], 0, sampleFilterGain[6], 0);
AudioConnection        patchCordDly7oGain(sampleFilterGain[6], 0, mixer2, 2);
AudioConnection        patchCord212(freeverbmixer8, 0, voiceDelayFb[7], 0);
AudioConnection        patchCordDly8fb(voiceDelay[7], 0, voiceDelayFb[7], 1);
AudioConnection        patchCordDly8in(voiceDelayFb[7], 0, voiceDelay[7], 0);
AudioConnection        patchCordDly8w(voiceDelay[7], 0, voiceDelayMix[7], 0);
AudioConnection        patchCordDly8d(freeverbmixer8, 0, voiceDelayMix[7], 3);
AudioConnection        patchCordDly8o(voiceDelayMix[7], 0, sampleFilterGain[7], 0);
AudioConnection        patchCordDly8oGain(sampleFilterGain[7], 0, mixer2, 3);
AudioConnection        patchCord213(mixer1, 0, mixer_end, 0);
AudioConnection        patchCord214(mixer2, 0, mixer_end, 1);
AudioConnection        patchCord215(envelope0, 0, mixer0, 0);
AudioAmplifier         ampPreview;
AudioEffectFade              sdPreviewFade;
AudioConnection        patchCord216(playSdWav1, 0, sdPreviewFade, 0);
AudioConnection        patchCord216fade(sdPreviewFade, 0, ampPreview, 0);
AudioConnection        patchCord216b(ampPreview, 0, mixer0, 1);
AudioConnection        patchCord217(playSdWav1, 0, peak1, 0);
// patchCord218/219 removed (mixer_end -> mixerPlay)
AudioConnection        patchCord220(audioInput, 0, audioInputAmp, 0);
AudioConnection        patchRecHpfIn(audioInputAmp, 0, recInputHpf, 0);
AudioConnection        patchRecHpfDry(audioInputAmp, 0, recInputHpfMix, 0);
AudioConnection        patchRecHpfHp(recInputHpf, 0, recInputHpfMix, 1);
AudioConnection        patchCord221(recInputHpfMix, 0, queue1, 0);
AudioConnection        patchCord222(recInputHpfMix, 0, peakRec, 0);
AudioConnection        patchCord222b(mixer_end, 0, peakOutput, 0);  // LED strip audio level
AudioConnection        patchCord223(mixer0, 0, mixer_stereoR, 1);
AudioConnection        patchCord224(mixer0, 0, mixer_stereoL, 1);
AudioConnection        patchCord225(mixer_end, 0, mixer_stereoR, 0);
AudioConnection        patchCord226(mixer_end, 0, mixer_stereoL, 0);
// Direct connections for L+R mode: mixer1 (ch1-4) and mixer2 (ch5-8) to stereo mixers
AudioConnection        patchCord230(mixer1, 0, mixer_stereoL, 2);  // ch1-4 -> L
AudioConnection        patchCord231(mixer2, 0, mixer_stereoR, 2);  // ch5-8 -> R
AudioConnection        patchCord227(mixer_stereoR, 0, i2s1, 0);
AudioConnection        patchCord228(mixer_stereoL, 0, i2s1, 1);
// USB mix copies the headphone buses, with preview (mixer0 / voice 0) gated.
AudioMixer4Limited            usbMixR;
AudioMixer4Limited            usbMixL;
AudioConnection        patchUsbMainR(mixer_end, 0, usbMixR, 0);
AudioConnection        patchUsbPrevR(mixer0, 0, usbMixR, 1);
AudioConnection        patchUsbBusR(mixer2, 0, usbMixR, 2);
AudioConnection        patchUsbMainL(mixer_end, 0, usbMixL, 0);
AudioConnection        patchUsbPrevL(mixer0, 0, usbMixL, 1);
AudioConnection        patchUsbBusL(mixer1, 0, usbMixL, 2);
AudioConnection        patchCordUsb0(usbMixR, 0, usb1, 0);
AudioConnection        patchCordUsb1(usbMixL, 0, usb1, 1);
AudioConnection        patchCord229(recInputHpfMix, 0, mixer_end, 3);




// Control Nodes (all control nodes (no inputs or outputs))
AudioControlSGTL5000     sgtl5000_1;     //xy=3827,1361



// TeensyAudioDesign: end automatically generated code

