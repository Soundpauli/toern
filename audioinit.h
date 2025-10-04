
// Audio Processing Nodes
AudioMixer4                     SNchaosMix;     //xy=164,617
AudioSynthWaveformDc            BDpitchAmt;     //xy=227,398
AudioMixer4                     HHchaosMix;     //xy=215,806
AudioEffectEnvelope             BDpitchEnv;     //xy=392,397
AudioSynthWaveformModulated     SNtone;         //xy=322,599
AudioSynthWaveformModulated     SNtone2;        //xy=322,649
AudioSynthWaveformModulated     HHtone;         //xy=376,788
AudioSynthWaveformModulated     HHtone2;        //xy=372,837
AudioSynthWaveform              waveform11_1;   //xy=405,1269
AudioSynthWaveform              waveform11_2;   //xy=405,1310
AudioEffectEnvelope             envelope11;     //xy=412,1351
AudioSynthWaveform              waveform13_1;   //xy=409,1582
AudioSynthWaveform              waveform13_2;   //xy=408,1626
AudioSynthWaveform              waveform14_1;   //xy=421,1766
AudioSynthWaveform              waveform14_2;   //xy=427,1810
EXTMEM AudioSynthWaveform              Swaveform2_2; //xy=495,2275
EXTMEM AudioSynthWaveform              Swaveform3_0; //xy=475,2345
EXTMEM AudioSynthWaveform              Swaveform3_1; //xy=490,2400
EXTMEM AudioSynthWaveform              Swaveform3_2; //xy=490,2460
AudioSynthWaveformDc            Sdc1_0; //xy=460,2550
AudioSynthWaveformDc            Sdc1_1; //xy=465,2605
AudioSynthWaveformDc            Sdc1_2; //xy=460,2665
AudioMixer4                     BDchaosMix;     //xy=580,415
AudioMixer4                     SNtoneMix;      //xy=500,623
AudioMixer4                     HHtoneMix;      //xy=539,824
EXTMEM AudioSynthWaveform              Swaveform1_0; //xy=520,1970
EXTMEM AudioSynthWaveform              Swaveform1_1; //xy=525,2030
EXTMEM AudioSynthWaveform              Swaveform1_2; //xy=525,2085
EXTMEM AudioSynthWaveform              Swaveform2_0; //xy=505,2150
EXTMEM AudioSynthWaveform              Swaveform2_1; //xy=515,2215
AudioSynthNoiseWhite            SNnoise;        //xy=666,607
AudioEffectEnvelope             SNtoneEnv;      //xy=657,651
AudioMixer4                     mixer_waveform11; //xy=636,1304
AudioMixer4                     mixer_waveform13; //xy=641,1611
AudioMixer4                     mixer_waveform14; //xy=658,1781
AudioEffectEnvelope             SenvelopeFilter1_0; //xy=675,2560
AudioEffectEnvelope             SenvelopeFilter1_1; //xy=680,2620
AudioEffectEnvelope             SenvelopeFilter1_2; //xy=685,2670
AudioSynthWaveformModulated     BDsine;         //xy=747,399
AudioSynthWaveformModulated     BDsaw;          //xy=749,452
AudioSynthNoiseWhite            HHnoise;        //xy=714,803
AudioEffectEnvelope             HHtoneEnv;      //xy=710,841
AudioMixer4                     SNMix;          //xy=838,621
AudioMixer4                     HHMix;          //xy=881,829
AudioMixer4                     BDMix;          //xy=922,427
AudioEffectEnvelope             SNenv;          //xy=976,621
AudioPlayArrayResmp             sound8;         //xy=990,745
AudioEffectEnvelope             BDenv;          //xy=1084,439
AudioEffectEnvelope             HHenv;          //xy=1064,816
AudioEffectEnvelope             envelope13;     //xy=1073,1610
AudioEffectEnvelope             envelope14;     //xy=1082,1784
AudioFilterStateVariable        SNfilt;         //xy=1113,564
EXTMEM AudioMixer4                     Smixer1_0; //xy=1130,2055
EXTMEM AudioMixer4                     Smixer1_1; //xy=1130,2140
EXTMEM AudioMixer4                     Smixer1_2; //xy=1130,2215
EXTMEM AudioMixer4                     Smixer2_0; //xy=1135,2370
EXTMEM AudioMixer4                     Smixer2_1; //xy=1135,2455
EXTMEM AudioMixer4                     Smixer2_2; //xy=1140,2565
AudioFilterStateVariable        BDfilter;       //xy=1234,453
AudioFilterStateVariable        SnFilter;       //xy=1273,611
AudioFilterStateVariable        HHfilt;         //xy=1243,731
AudioFilterStateVariable        HhFilter;       //xy=1221,838
AudioEffectBitcrusher           bitcrusher11;   //xy=1249,1294
AudioEffectBitcrusher           bitcrusher13;   //xy=1251,1612
AudioEffectBitcrusher           bitcrusher14;   //xy=1271,1782
EXTMEM AudioFilterLadder               Sladder1_0; //xy=1365,2105
EXTMEM AudioFilterLadder               Sladder1_1; //xy=1365,2175
EXTMEM AudioFilterLadder               Sladder1_2; //xy=1370,2245
EXTMEM AudioFilterLadder               Sladder2_0; //xy=1360,2430
EXTMEM AudioFilterLadder               Sladder2_1; //xy=1365,2510
EXTMEM AudioFilterLadder               Sladder2_2; //xy=1365,2580
AudioMixer4                     BdFilterMix;    //xy=1429,456
AudioMixer4                     SnFilterMix;    //xy=1438,621
AudioMixer4                     HhFilterMix;    //xy=1408,835
AudioAmplifier                  amp11;          //xy=1421,1295
AudioAmplifier                  amp13;          //xy=1409,1612
AudioAmplifier                  amp14;          //xy=1439,1783
AudioPlayArrayResmp             sound1;         //xy=1560,395
AudioPlayArrayResmp             sound2;         //xy=1570,560
AudioPlayArrayResmp             sound3;         //xy=1555,685
AudioFilterStateVariable        filter11;       //xy=1581,1302
AudioFilterStateVariable        filter13;       //xy=1562,1616
AudioFilterStateVariable        filter14;       //xy=1584,1790
EXTMEM AudioEffectEnvelope             Senvelope1_0; //xy=1555,2115
EXTMEM AudioEffectEnvelope             Senvelope1_1; //xy=1560,2175
EXTMEM AudioEffectEnvelope             Senvelope1_2; //xy=1560,2240
EXTMEM AudioEffectEnvelope             Senvelope2_0; //xy=1545,2460
EXTMEM AudioEffectEnvelope             Senvelope2_1; //xy=1545,2525
EXTMEM AudioEffectEnvelope             Senvelope2_2; //xy=1555,2585
AudioMixer4                     BDMixer;        //xy=1680,473
AudioMixer4                     SNMixer;        //xy=1693,614
AudioMixer4                     HHMixer;        //xy=1700,717
AudioPlayArrayResmp             sound4;         //xy=1715,795
AudioPlayArrayResmp             sound5;         //xy=1720,865
AudioPlayArrayResmp             sound6;         //xy=1720,935
AudioPlayArrayResmp             sound7;         //xy=1750,1005
AudioMixer4                     filtermixer11;  //xy=1750,1309
AudioMixer4                     filtermixer13;  //xy=1736,1622
AudioMixer4                     filtermixer14;  //xy=1746,1796
EXTMEM AudioMixer4                     SmixerL4; //xy=1785,2195

EXTMEM AudioMixer4                     SmixerR4; //xy=1815,2530
AudioEffectEnvelope             envelope1;      //xy=1927,516
AudioEffectEnvelope             envelope2;      //xy=1928,626
AudioEffectEnvelope             envelope3;      //xy=1926,717
AudioEffectEnvelope             envelope4;      //xy=1927,794
AudioEffectEnvelope             envelope5;      //xy=1926,864
AudioEffectEnvelope             envelope6;      //xy=1923,942
AudioEffectEnvelope             envelope7;      //xy=1924,1025
AudioEffectEnvelope             envelope8;      //xy=1922,1102
AudioEffectFreeverb             freeverb11;     //xy=1922,1384

EXTMEM AudioMixer4                     CHMixer11; //xy=1945,2340
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
AudioEffectFreeverb             freeverb1;      //xy=2750,488
AudioEffectFreeverb             freeverb2;      //xy=2761,627
AudioEffectFreeverb             freeverb5;      //xy=2759,998
AudioEffectFreeverb             freeverb6;      //xy=2765,108
AudioEffectFreeverb             freeverb7;      //xy=2759,998
AudioEffectFreeverb             freeverb8;      //xy=2765,1083
AudioMixer4                     mixersynth_end; //xy=2889,1562
AudioMixer4                     freeverbmixer1; //xy=2964,513
AudioMixer4                     freeverbmixer2; //xy=2978,657
AudioMixer4                     freeverbmixer5; //xy=2970,912
AudioMixer4                     freeverbmixer6; //xy=2970,1004
AudioMixer4                     freeverbmixer7; //xy=2970,1016
AudioMixer4                     freeverbmixer8; //xy=2970,1108
AudioMixer4                     mixer1;         //xy=3200,763
AudioMixer4                     mixer2;         //xy=3200,948
AudioEffectEnvelope             envelope0;      //xy=3524,426
AudioPlaySdWav                  playSdWav1;     //xy=3524,474
AudioMixer4                     mixer_end;      //xy=3518,1059
AudioInputI2S                   audioInput;     //xy=3556,1189
AudioAmplifier                  audioInputAmp;  //xy=3650,1189
AudioMixer4                     mixer0;         //xy=3722,478
AudioAnalyzePeak                peak1;          //xy=3727,541
AudioMixer4                     mixerPlay;      //xy=3714,1048
AudioRecordQueue                queue1;         //xy=3778,1164
AudioAnalyzePeak                peakRec;        //xy=3788,1215
AudioMixer4                     mixer_stereoR;  //xy=3932,748
AudioMixer4                     mixer_stereoL;  //xy=3931,848
AudioOutputI2S                  i2s1;           //xy=4154,795

// Audio Connections (all connections (aka wires or links))
AudioConnection        patchCord1(SNchaosMix, 0, SNtone, 0);
AudioConnection        patchCord2(SNchaosMix, 0, SNtone2, 0);
AudioConnection        patchCord3(BDpitchAmt, 0, BDpitchEnv, 0);
AudioConnection        patchCord4(HHchaosMix, 0, HHtone, 0);
AudioConnection        patchCord5(HHchaosMix, 0, HHtone2, 0);
AudioConnection        patchCord6(BDpitchEnv, 0, BDchaosMix, 0);
AudioConnection        patchCord7(SNtone, 0, SNtoneMix, 0);
AudioConnection        patchCord8(SNtone2, 0, SNchaosMix, 0);
AudioConnection        patchCord9(SNtone2, 0, SNtoneMix, 1);
AudioConnection        patchCord10(HHtone, 0, HHtoneMix, 0);
AudioConnection        patchCord11(HHtone2, 0, HHchaosMix, 0);
AudioConnection        patchCord12(HHtone2, 0, HHtoneMix, 1);
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
AudioConnection        patchCord31(BDchaosMix, 0, BDsine, 0);
AudioConnection        patchCord32(BDchaosMix, 0, BDsaw, 0);
AudioConnection        patchCord33(BDchaosMix, 0, BDsaw, 1);
AudioConnection        patchCord34(SNtoneMix, 0, SNtoneEnv, 0);
AudioConnection        patchCord35(HHtoneMix, 0, HHtoneEnv, 0);
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
AudioConnection        patchCord46(SNnoise, 0, SNMix, 0);
AudioConnection        patchCord47(SNtoneEnv, 0, SNMix, 1);
AudioConnection        patchCord48(mixer_waveform11, 0, bitcrusher11, 0);
AudioConnection        patchCord49(mixer_waveform13, 0, envelope13, 0);
AudioConnection        patchCord50(mixer_waveform14, 0, envelope14, 0);
AudioConnection        patchCord51(SenvelopeFilter1_0, 0, Sladder1_0, 1);
AudioConnection        patchCord52(SenvelopeFilter1_0, 0, Sladder2_0, 1);
AudioConnection        patchCord53(SenvelopeFilter1_1, 0, Sladder1_1, 1);
AudioConnection        patchCord54(SenvelopeFilter1_1, 0, Sladder2_1, 1);
AudioConnection        patchCord55(SenvelopeFilter1_2, 0, Sladder1_2, 1);
AudioConnection        patchCord56(SenvelopeFilter1_2, 0, Sladder2_2, 1);
AudioConnection        patchCord57(BDsine, 0, BDMix, 0);
AudioConnection        patchCord58(BDsaw, 0, BDchaosMix, 1);
AudioConnection        patchCord59(BDsaw, 0, BDMix, 1);
AudioConnection        patchCord60(HHnoise, 0, HHMix, 0);
AudioConnection        patchCord61(HHtoneEnv, 0, HHMix, 1);
AudioConnection        patchCord62(SNMix, 0, SNenv, 0);
AudioConnection        patchCord63(HHMix, 0, HHenv, 0);
AudioConnection        patchCord64(BDMix, 0, BDenv, 0);
AudioConnection        patchCord65(SNenv, 0, SNfilt, 0);
AudioConnection        patchCord66(sound8, 0, envelope8, 0);
AudioConnection        patchCord67(BDenv, 0, BDfilter, 0);
AudioConnection        patchCord68(HHenv, 0, HHfilt, 0);
AudioConnection        patchCord69(envelope13, 0, bitcrusher13, 0);
AudioConnection        patchCord70(envelope14, 0, bitcrusher14, 0);
AudioConnection        patchCord71(SNfilt, 1, SnFilter, 0);
AudioConnection        patchCord72(Smixer1_0, 0, Sladder1_0, 0);
AudioConnection        patchCord73(Smixer1_1, 0, Sladder1_1, 0);
AudioConnection        patchCord74(Smixer1_2, 0, Sladder1_2, 0);
AudioConnection        patchCord75(Smixer2_0, 0, Sladder2_0, 0);
AudioConnection        patchCord76(Smixer2_1, 0, Sladder2_1, 0);
AudioConnection        patchCord77(Smixer2_2, 0, Sladder2_2, 0);
AudioConnection        patchCord78(BDfilter, 0, BdFilterMix, 0);
AudioConnection        patchCord79(BDfilter, 2, BdFilterMix, 1);
AudioConnection        patchCord80(SnFilter, 0, SnFilterMix, 0);
AudioConnection        patchCord81(SnFilter, 2, SnFilterMix, 1);
AudioConnection        patchCord82(HHfilt, 1, HhFilter, 0);
AudioConnection        patchCord83(HhFilter, 0, HhFilterMix, 0);
AudioConnection        patchCord84(HhFilter, 2, HhFilterMix, 1);
AudioConnection        patchCord85(bitcrusher11, 0, amp11, 0);
AudioConnection        patchCord86(bitcrusher13, 0, amp13, 0);
AudioConnection        patchCord87(bitcrusher14, 0, amp14, 0);
AudioConnection        patchCord88(Sladder1_0, 0, Senvelope1_0, 0);
AudioConnection        patchCord89(Sladder1_1, 0, Senvelope1_1, 0);
AudioConnection        patchCord90(Sladder1_2, 0, Senvelope1_2, 0);
AudioConnection        patchCord91(Sladder2_0, 0, Senvelope2_0, 0);
AudioConnection        patchCord92(Sladder2_1, 0, Senvelope2_1, 0);
AudioConnection        patchCord93(Sladder2_2, 0, Senvelope2_2, 0);
AudioConnection        patchCord94(BdFilterMix, 0, BDMixer, 1);
AudioConnection        patchCord95(SnFilterMix, 0, SNMixer, 1);
AudioConnection        patchCord96(HhFilterMix, 0, HHMixer, 1);
AudioConnection        patchCord97(amp11, 0, filter11, 0);
AudioConnection        patchCord98(amp13, 0, filter13, 0);
AudioConnection        patchCord99(amp14, 0, filter14, 0);
AudioConnection        patchCord100(sound1, 0, BDMixer, 0);
AudioConnection        patchCord101(sound2, 0, SNMixer, 0);
AudioConnection        patchCord102(sound3, 0, HHMixer, 0);
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
AudioConnection        patchCord118(BDMixer, 0, envelope1, 0);
AudioConnection        patchCord119(SNMixer, 0, envelope2, 0);
AudioConnection        patchCord120(HHMixer, 0, envelope3, 0);
AudioConnection        patchCord121(sound4, 0, envelope4, 0);
AudioConnection        patchCord122(sound5, 0, envelope5, 0);
AudioConnection        patchCord123(sound6, 0, envelope6, 0);
AudioConnection        patchCord124(sound7, 0, envelope7, 0);
AudioConnection        patchCord125(filtermixer11, 0, freeverb11, 0);
AudioConnection        patchCord126(filtermixer11, 0, synthmixer11, 3);
//AudioConnection        patchCord128(filtermixer13, 0, freeverb13, 0);
AudioConnection        patchCord129(filtermixer13, 0, synthmixer13, 3);
//AudioConnection        patchCord131(filtermixer14, 0, freeverb14, 0);
AudioConnection        patchCord132(filtermixer14, 0, synthmixer14, 3);
AudioConnection        patchCord134(SmixerL4, 0, CHMixer11, 0);
//AudioConnection        patchCord135(freeverb13, 0, synthmixer13, 0);
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
//AudioConnection        patchCord146(freeverb14, 0, synthmixer14, 0);
AudioConnection        patchCord147(CHMixer11, 0, mixer_waveform11, 3);
AudioConnection        patchCord148(bitcrusher4, 0, amp4, 0);
AudioConnection        patchCord149(bitcrusher5, 0, amp5, 0);
AudioConnection        patchCord150(bitcrusher6, 0, amp6, 0);
AudioConnection        patchCord151(bitcrusher7, 0, amp7, 0);
AudioConnection        patchCord152(bitcrusher8, 0, amp8, 0);
AudioConnection        patchCord153(bitcrusher1, 0, amp1, 0);
AudioConnection        patchCord154(bitcrusher2, 0, amp2, 0);
AudioConnection        patchCord155(bitcrusher3, 0, amp3, 0);
AudioConnection        patchCord156(synthmixer11, 0, mixersynth_end, 0);
AudioConnection        patchCord157(synthmixer13, 0, mixersynth_end, 2);
AudioConnection        patchCord158(synthmixer14, 0, mixersynth_end, 3);
AudioConnection        patchCord159(sound0, 0, envelope0, 0);
AudioConnection        patchCord160(amp1, 0, filter1, 0);
AudioConnection        patchCord161(amp2, 0, filter2, 0);
AudioConnection        patchCord162(amp3, 0, filter3, 0);
AudioConnection        patchCord163(amp4, 0, filter4, 0);
AudioConnection        patchCord164(amp5, 0, filter5, 0);
AudioConnection        patchCord165(amp6, 0, filter6, 0);
AudioConnection        patchCord166(amp7, 0, filter7, 0);
AudioConnection        patchCord167(amp8, 0, filter8, 0);
//test granular
//AudioEffectGranular      granular1;      //xy=504,155
//AudioConnection        patchCord167a(amp8, 0, granular1, 0);
//AudioConnection        patchCord167b(granular1, 0, filter8, 0);

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
AudioConnection        patchCord196(filtermixer3, 0, mixer1, 2);
AudioConnection        patchCord197(filtermixer4, 0, mixer1, 3);
AudioConnection        patchCord198(filtermixer5, 0, freeverb5, 0);
AudioConnection        patchCord198b(filtermixer5, 0, freeverbmixer5, 3);
AudioConnection        patchCord199(filtermixer6, 0, freeverb6, 0);
AudioConnection        patchCord199b(filtermixer6, 0, freeverbmixer6, 3);
AudioConnection        patchCord200(filtermixer7, 0, freeverb7, 0);
AudioConnection        patchCord201(filtermixer7, 0, freeverbmixer7, 3);
AudioConnection        patchCord202(filtermixer8, 0, freeverb8, 0);
AudioConnection        patchCord203(filtermixer8, 0, freeverbmixer8, 3);
AudioConnection        patchCord204(freeverb1, 0, freeverbmixer1, 0);
AudioConnection        patchCord205(freeverb2, 0, freeverbmixer2, 0);
AudioConnection        patchCord205b(freeverb5, 0, freeverbmixer5, 0);
AudioConnection        patchCord205c(freeverb6, 0, freeverbmixer6, 0);
AudioConnection        patchCord206(freeverb7, 0, freeverbmixer7, 0);
AudioConnection        patchCord207(freeverb8, 0, freeverbmixer8, 0);
AudioConnection        patchCord208(mixersynth_end, 0, mixer_end, 2);
AudioConnection        patchCord209(freeverbmixer1, 0, mixer1, 0);
AudioConnection        patchCord210(freeverbmixer2, 0, mixer1, 1);
AudioConnection        patchCord210b(freeverbmixer5, 0, mixer2, 0);
AudioConnection        patchCord210c(freeverbmixer6, 0, mixer2, 1);
AudioConnection        patchCord211(freeverbmixer7, 0, mixer2, 2);
AudioConnection        patchCord212(freeverbmixer8, 0, mixer2, 3);
AudioConnection        patchCord213(mixer1, 0, mixer_end, 0);
AudioConnection        patchCord214(mixer2, 0, mixer_end, 1);
AudioConnection        patchCord215(envelope0, 0, mixer0, 0);
AudioConnection        patchCord216(playSdWav1, 0, mixer0, 1);
AudioConnection        patchCord217(playSdWav1, 0, peak1, 0);
AudioConnection        patchCord218(mixer_end, 0, mixerPlay, 0);
AudioConnection        patchCord219(mixer_end, 0, mixerPlay, 1);
AudioConnection        patchCord220(audioInput, 0, audioInputAmp, 0);
AudioConnection        patchCord221(audioInputAmp, 0, queue1, 0);
AudioConnection        patchCord222(audioInputAmp, 0, peakRec, 0);
AudioConnection        patchCord223(mixer0, 0, mixer_stereoR, 1);
AudioConnection        patchCord224(mixer0, 0, mixer_stereoL, 1);
AudioConnection        patchCord225(mixerPlay, 0, mixer_stereoR, 0);
AudioConnection        patchCord226(mixerPlay, 0, mixer_stereoL, 0);
AudioConnection        patchCord227(mixer_stereoR, 0, i2s1, 0);
AudioConnection        patchCord228(mixer_stereoL, 0, i2s1, 1);
AudioConnection        patchCord229(audioInputAmp, 0, mixer_end, 3);




// Control Nodes (all control nodes (no inputs or outputs))
AudioControlSGTL5000     sgtl5000_1;     //xy=3827,1361



// TeensyAudioDesign: end automatically generated code

