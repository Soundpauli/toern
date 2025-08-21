/*



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
AudioSynthWaveform              Swaveform2_2; //xy=495,2275
AudioSynthWaveform              Swaveform3_0; //xy=475,2345
AudioSynthWaveform              Swaveform3_1; //xy=490,2400
AudioSynthWaveform              Swaveform3_2; //xy=490,2460
AudioSynthWaveformDc            Sdc1_0; //xy=460,2550
AudioSynthWaveformDc            Sdc1_1; //xy=465,2605
AudioSynthWaveformDc            Sdc1_2; //xy=460,2665
AudioMixer4                     BDchaosMix;     //xy=580,415
AudioMixer4                     SNtoneMix;      //xy=500,623
AudioMixer4                     HHtoneMix;      //xy=539,824
AudioSynthWaveform              Swaveform1_0; //xy=520,1970
AudioSynthWaveform              Swaveform1_1; //xy=525,2030
AudioSynthWaveform              Swaveform1_2; //xy=525,2085
AudioSynthWaveform              Swaveform2_0; //xy=505,2150
AudioSynthWaveform              Swaveform2_1; //xy=515,2215
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
AudioMixer4                     Smixer1_0; //xy=1130,2055
AudioMixer4                     Smixer1_1; //xy=1130,2140
AudioMixer4                     Smixer1_2; //xy=1130,2215
AudioMixer4                     Smixer2_0; //xy=1135,2370
AudioMixer4                     Smixer2_1; //xy=1135,2455
AudioMixer4                     Smixer2_2; //xy=1140,2565
AudioFilterStateVariable        BDfilter;       //xy=1234,453
AudioFilterStateVariable        SnFilter;       //xy=1273,611
AudioFilterStateVariable        HHfilt;         //xy=1243,731
AudioFilterStateVariable        HhFilter;       //xy=1221,838
AudioEffectBitcrusher           bitcrusher11;   //xy=1249,1294
AudioEffectBitcrusher           bitcrusher13;   //xy=1251,1612
AudioEffectBitcrusher           bitcrusher14;   //xy=1271,1782
AudioFilterLadder               Sladder1_0; //xy=1365,2105
AudioFilterLadder               Sladder1_1; //xy=1365,2175
AudioFilterLadder               Sladder1_2; //xy=1370,2245
AudioFilterLadder               Sladder2_0; //xy=1360,2430
AudioFilterLadder               Sladder2_1; //xy=1365,2510
AudioFilterLadder               Sladder2_2; //xy=1365,2580
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
AudioEffectEnvelope             Senvelope1_0; //xy=1555,2115
AudioEffectEnvelope             Senvelope1_1; //xy=1560,2175
AudioEffectEnvelope             Senvelope1_2; //xy=1560,2240
AudioEffectEnvelope             Senvelope2_0; //xy=1545,2460
AudioEffectEnvelope             Senvelope2_1; //xy=1545,2525
AudioEffectEnvelope             Senvelope2_2; //xy=1555,2585
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
AudioMixer4                     SmixerL4; //xy=1785,2195
AudioEffectFreeverb             freeverb13;     //xy=1898,1659
AudioMixer4                     SmixerR4; //xy=1815,2530
AudioEffectEnvelope             envelope1;      //xy=1927,516
AudioEffectEnvelope             envelope2;      //xy=1928,626
AudioEffectEnvelope             envelope3;      //xy=1926,717
AudioEffectEnvelope             envelope4;      //xy=1927,794
AudioEffectEnvelope             envelope5;      //xy=1926,864
AudioEffectEnvelope             envelope6;      //xy=1923,942
AudioEffectEnvelope             envelope7;      //xy=1924,1025
AudioEffectEnvelope             envelope8;      //xy=1922,1102
AudioEffectFreeverb             freeverb11;     //xy=1922,1384
AudioEffectFreeverb             freeverb14;     //xy=1914,1848
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
AudioEffectFreeverb             freeverb1;      //xy=2750,488
AudioEffectFreeverb             freeverb2;      //xy=2761,627
AudioEffectFreeverb             freeverb7;      //xy=2759,998
AudioEffectFreeverb             freeverb8;      //xy=2765,1083
AudioMixer4                     mixersynth_end; //xy=2889,1562
AudioMixer4                     freeverbmixer1; //xy=2964,513
AudioMixer4                     freeverbmixer2; //xy=2978,657
AudioMixer4                     freeverbmixer7; //xy=2970,1016
AudioMixer4                     freeverbmixer8; //xy=2970,1108
AudioMixer4                     mixer1;         //xy=3200,763
AudioMixer4                     mixer2;         //xy=3200,948
AudioEffectEnvelope             envelope0;      //xy=3524,426
AudioPlaySdWav                  playSdWav1;     //xy=3524,474
AudioMixer4                     mixer_end;      //xy=3518,1059
AudioInputI2S                   audioInput;     //xy=3556,1189
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
AudioConnection        patchCord127(filtermixer11, 0, synthmixer11, 0);
AudioConnection        patchCord128(filtermixer13, 0, freeverb13, 0);
AudioConnection        patchCord129(filtermixer13, 0, synthmixer13, 3);
AudioConnection        patchCord130(filtermixer13, 0, synthmixer13, 0);
AudioConnection        patchCord131(filtermixer14, 0, freeverb14, 0);
AudioConnection        patchCord132(filtermixer14, 0, synthmixer14, 3);
AudioConnection        patchCord133(filtermixer14, 0, synthmixer14, 0);
AudioConnection        patchCord134(SmixerL4, 0, CHMixer11, 0);
AudioConnection        patchCord135(freeverb13, 0, synthmixer13, 1);
AudioConnection        patchCord136(SmixerR4, 0, CHMixer11, 1);
AudioConnection        patchCord137(envelope1, 0, bitcrusher1, 0);
AudioConnection        patchCord138(envelope2, 0, bitcrusher2, 0);
AudioConnection        patchCord139(envelope3, 0, bitcrusher3, 0);
AudioConnection        patchCord140(envelope4, 0, bitcrusher4, 0);
AudioConnection        patchCord141(envelope5, 0, bitcrusher5, 0);
AudioConnection        patchCord142(envelope6, 0, bitcrusher6, 0);
AudioConnection        patchCord143(envelope7, 0, bitcrusher7, 0);
AudioConnection        patchCord144(envelope8, 0, bitcrusher8, 0);
AudioConnection        patchCord145(freeverb11, 0, synthmixer11, 1);
AudioConnection        patchCord146(freeverb14, 0, synthmixer14, 1);
AudioConnection        patchCord147(CHMixer11, 0, mixer_waveform11, 2);
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
AudioConnection        patchCord198(filtermixer5, 0, mixer2, 0);
AudioConnection        patchCord199(filtermixer6, 0, mixer2, 1);
AudioConnection        patchCord200(filtermixer7, 0, freeverb7, 0);
AudioConnection        patchCord201(filtermixer7, 0, freeverbmixer7, 3);
AudioConnection        patchCord202(filtermixer8, 0, freeverb8, 0);
AudioConnection        patchCord203(filtermixer8, 0, freeverbmixer8, 3);
AudioConnection        patchCord204(freeverb1, 0, freeverbmixer1, 0);
AudioConnection        patchCord205(freeverb2, 0, freeverbmixer2, 0);
AudioConnection        patchCord206(freeverb7, 0, freeverbmixer7, 0);
AudioConnection        patchCord207(freeverb8, 0, freeverbmixer8, 0);
AudioConnection        patchCord208(mixersynth_end, 0, mixer_end, 2);
AudioConnection        patchCord209(freeverbmixer1, 0, mixer1, 0);
AudioConnection        patchCord210(freeverbmixer2, 0, mixer1, 1);
AudioConnection        patchCord211(freeverbmixer7, 0, mixer2, 2);
AudioConnection        patchCord212(freeverbmixer8, 0, mixer2, 3);
AudioConnection        patchCord213(mixer1, 0, mixer_end, 0);
AudioConnection        patchCord214(mixer2, 0, mixer_end, 1);
AudioConnection        patchCord215(envelope0, 0, mixer0, 0);
AudioConnection        patchCord216(playSdWav1, 0, mixer0, 1);
AudioConnection        patchCord217(playSdWav1, 0, peak1, 0);
AudioConnection        patchCord218(mixer_end, 0, mixerPlay, 0);
AudioConnection        patchCord219(mixer_end, 0, mixerPlay, 1);
AudioConnection        patchCord220(audioInput, 0, queue1, 0);
AudioConnection        patchCord221(audioInput, 0, peakRec, 0);
AudioConnection        patchCord222(mixer0, 0, mixer_stereoR, 1);
AudioConnection        patchCord223(mixer0, 0, mixer_stereoL, 1);
AudioConnection        patchCord224(mixerPlay, 0, mixer_stereoR, 0);
AudioConnection        patchCord225(mixerPlay, 0, mixer_stereoL, 0);
AudioConnection        patchCord226(mixer_stereoR, 0, i2s1, 0);
AudioConnection        patchCord227(mixer_stereoL, 0, i2s1, 1);
AudioConnection        patchCord228(audioInput, 0, mixer_end, 3);

// Control Nodes (all control nodes (no inputs or outputs))
AudioControlSGTL5000     sgtl5000_1;     //xy=3827,1361



// TeensyAudioDesign: end automatically generated code



///SYNTHS.ino with full audio patch:
#define POLY_VOICES 3
#define INSTRUMENT_CHANNELS 2

#define AUTO_OFF_TIME 1800  // milliseconds
unsigned long voiceStartTime[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0, 0, 0 };

int cents[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0, 0, 0 };
int semitones[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0, 0, 0 };
int pan[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0, 0, 0 };
int volume[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };

int attackAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };
int decayAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };
float sustainAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = { 1, 1, 1 };
int releaseAmp[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };

int attackFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };
int decayFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };
float sustainFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = { 1, 1, 1 };
int releaseFilter[INSTRUMENT_CHANNELS][POLY_VOICES] = { 100, 100, 100 };

int cutoff[INSTRUMENT_CHANNELS][POLY_VOICES] = { 1000, 1000, 1000 };
float resonance[INSTRUMENT_CHANNELS][POLY_VOICES] = { 2.0, 2.0, 2.0 };
float filterAmount[INSTRUMENT_CHANNELS][POLY_VOICES] = { 2.0, 2.0, 2.0 };

int waveforms[INSTRUMENT_CHANNELS][POLY_VOICES] = { WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH, WAVEFORM_SAWTOOTH };
int waveformsArray[INSTRUMENT_CHANNELS][5] = { WAVEFORM_SAWTOOTH, WAVEFORM_PULSE, WAVEFORM_SQUARE, WAVEFORM_TRIANGLE, WAVEFORM_SINE };
float pulseWidth[INSTRUMENT_CHANNELS][POLY_VOICES] = { 0.25, 0.25, 0.25 };

int noteArray[INSTRUMENT_CHANNELS][8];
int notePlaying[2] = { 0 };



// arp_synth preset (menuIndex==?) with additional parameters

void arp_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  // Default tuning and sound parameters
  octave[ch] = 5.0;
  cents[ch][0] = 10;  cents[ch][1] = -10; cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 0;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 0;   decayAmp[ch][1] = 0;   decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 2; releaseAmp[ch][1] = 2; releaseAmp[ch][2] = 2;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 0;   decayFilter[ch][1] = 0;   decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 2;  releaseFilter[ch][1] = 2;  releaseFilter[ch][2] = 2;

  // Extreme modulation mapping for filter parameters (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  // Default waveform settings (for voices 1–3 remain)
  waveforms[ch][0] = 0;
  waveforms[ch][1] = 0;
  waveforms[ch][2] = 0;

  // --- Additional modulation ---

  // p4 for pitch offset:
  // Map p4 (0-64) to an additional offset from -50 to +50 cents...
  // and -2 to +2 semitones.
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;  
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);  
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }

  // p5 for octave override: map from 1.0 to 8.0.
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;

  // p6 for waveform of first voice: map to an integer between 0 and 7.
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// lead_synth preset (menuIndex==?) with additional parameters
void lead_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 0;   cents[ch][1] = 0;   cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 0;
  pan[ch][0] = 0;     pan[ch][1] = 0;     pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 0;   volume[ch][2] = 0;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 250; decayAmp[ch][1] = 250; decayAmp[ch][2] = 250;
  sustainAmp[ch][0] = 0; sustainAmp[ch][1] = 0; sustainAmp[ch][2] = 0;
  releaseAmp[ch][0] = 250; releaseAmp[ch][1] = 250; releaseAmp[ch][2] = 250;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 250;  decayFilter[ch][1] = 250;  decayFilter[ch][2] = 250;
  sustainFilter[ch][0] = 0;  sustainFilter[ch][1] = 0;  sustainFilter[ch][2] = 0;
  releaseFilter[ch][0] = 250; releaseFilter[ch][1] = 250; releaseFilter[ch][2] = 250;

  // Extreme modulation mapping for filter parameters (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 2;
  waveforms[ch][1] = 2;
  waveforms[ch][2] = 2;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// keys_synth preset (menuIndex==1) with additional parameters
void keys_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 2;   cents[ch][1] = -2;   cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100;  pan[ch][1] = 100;  pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 2500; decayAmp[ch][1] = 2500; decayAmp[ch][2] = 2500;
  sustainAmp[ch][0] = 0;   sustainAmp[ch][1] = 0;   sustainAmp[ch][2] = 0;
  releaseAmp[ch][0] = 300; releaseAmp[ch][1] = 300; releaseAmp[ch][2] = 300;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 2500; decayFilter[ch][1] = 2500; decayFilter[ch][2] = 2500;
  sustainFilter[ch][0] = 0;  sustainFilter[ch][1] = 0;  sustainFilter[ch][2] = 0;
  releaseFilter[ch][0] = 300; releaseFilter[ch][1] = 300; releaseFilter[ch][2] = 300;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 0;
  waveforms[ch][1] = 0;
  waveforms[ch][2] = 0;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// pad_synth preset (menuIndex==2) with additional parameters
void pad_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 15;   cents[ch][1] = -15;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0;  semitones[ch][2] = 0;
  pan[ch][0] = -100;   pan[ch][1] = 100;   pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100;  volume[ch][2] = 0;
  
  attackAmp[ch][0] = 1500; attackAmp[ch][1] = 1500; attackAmp[ch][2] = 1500;
  sustainAmp[ch][0] = 0.6;  sustainAmp[ch][1] = 0.6;  sustainAmp[ch][2] = 0.6;
  releaseAmp[ch][0] = 1500; releaseAmp[ch][1] = 1500; releaseAmp[ch][2] = 1500;
  
  attackFilter[ch][0] = 1500; attackFilter[ch][1] = 1500; attackFilter[ch][2] = 1500;
  sustainFilter[ch][0] = 0.2;  sustainFilter[ch][1] = 0.2;  sustainFilter[ch][2] = 0.2;
  releaseFilter[ch][0] = 1500; releaseFilter[ch][1] = 1500; releaseFilter[ch][2] = 1500;
  
  // Map p1 to decay amplitude (p1 modulates decay from 60 to 3000)
  float minDecay = 60.0, maxDecay = 3000.0;
  float mappedDecay = minDecay + (p1 / 64.0) * (maxDecay - minDecay);
  decayAmp[ch][0] = decayAmp[ch][1] = decayAmp[ch][2] = mappedDecay;

  // p2 for filter cutoff; p3 for filter resonance:
  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p2 / 64.0) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p3 / 64.0) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 4;  waveforms[ch][1] = 4;  waveforms[ch][2] = 4;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// organ_synth preset (menuIndex==6) with additional parameters
void organ_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 5.0;
  cents[ch][0] = 0;  cents[ch][1] = 0;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = -12; semitones[ch][2] = 7;
  pan[ch][0] = 0;    pan[ch][1] = 0;    pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 100; decayAmp[ch][1] = 100; decayAmp[ch][2] = 100;
  sustainAmp[ch][0] = 0.1; sustainAmp[ch][1] = 0.1; sustainAmp[ch][2] = 0.1;
  releaseAmp[ch][0] = 100; releaseAmp[ch][1] = 100; releaseAmp[ch][2] = 100;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 100; decayFilter[ch][1] = 100; decayFilter[ch][2] = 100;
  sustainFilter[ch][0] = 0.6; sustainFilter[ch][1] = 0.6; sustainFilter[ch][2] = 0.6;
  releaseFilter[ch][0] = 100; releaseFilter[ch][1] = 100; releaseFilter[ch][2] = 100;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 4;  waveforms[ch][1] = 4;  waveforms[ch][2] = 4;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// flute_synth preset (menuIndex==7) with additional parameters
void flute_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 5.0;
  cents[ch][0] = -1;  cents[ch][1] = 1;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100;  pan[ch][1] = 100;  pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 10;

  decayAmp[ch][0] = 0; decayAmp[ch][1] = 0; decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 400; releaseAmp[ch][1] = 400; releaseAmp[ch][2] = 400;
  
  attackFilter[ch][0] = 500; attackFilter[ch][1] = 500; attackFilter[ch][2] = 500;
  decayFilter[ch][0] = 0;   decayFilter[ch][1] = 0;   decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 500; releaseFilter[ch][1] = 500; releaseFilter[ch][2] = 500;

  // p1 modulates attack amplitude (flute dynamics)
  float minAttack = 100.0, maxAttack = 2000.0;
  float mappedAttack = minAttack + (p1 / 64.0) * (maxAttack - minAttack);
  attackAmp[ch][0] = attackAmp[ch][1] = attackAmp[ch][2] = mappedAttack;

  // p2 for filter cutoff; p3 for filter resonance:
  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p2 / 64.0) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p3 / 64.0) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 3;  waveforms[ch][1] = 3;  waveforms[ch][2] = 4;
  
  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// wow_synth preset (menuIndex==9) with additional parameters
void wow_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 5;  cents[ch][1] = -5;  cents[ch][2] = 0;
  semitones[ch][0] = 7; semitones[ch][1] = 0; semitones[ch][2] = -12;
  pan[ch][0] = -50; pan[ch][1] = 50; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;
  attackAmp[ch][0] = 400; attackAmp[ch][1] = 400; attackAmp[ch][2] = 400;
  decayAmp[ch][0] = 4000; decayAmp[ch][1] = 4000; decayAmp[ch][2] = 4000;
  sustainAmp[ch][0] = 0.8; sustainAmp[ch][1] = 0.8; sustainAmp[ch][2] = 0.8;
  
  // p3 now is used to modulate release amp dramatically: from 10 to 1000.
  float minRelease = 10.0, maxRelease = 1000.0;
  float mappedRelease = minRelease + (p3 / 64.0) * (maxRelease - minRelease);
  releaseAmp[ch][0] = releaseAmp[ch][1] = releaseAmp[ch][2] = mappedRelease;

  attackFilter[ch][0] = 800; attackFilter[ch][1] = 800; attackFilter[ch][2] = 800;
  decayFilter[ch][0] = 6000; decayFilter[ch][1] = 6000; decayFilter[ch][2] = 6000;
  sustainFilter[ch][0] = 0.2; sustainFilter[ch][1] = 0.2; sustainFilter[ch][2] = 0.2;
  releaseFilter[ch][0] = 10;  releaseFilter[ch][1] = 10;  releaseFilter[ch][2] = 10;

  // p1 and p2 still control filter cutoff and resonance:
  float minCutoff = 400.0, maxCutoff = 6000.0;
  float mappedCutoff = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  float minResonance = 0.1, maxResonance = 5.0;
  float mappedResonance = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;

  waveforms[ch][0] = 0;  waveforms[ch][1] = 0;  waveforms[ch][2] = 0;
  
  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// bass_synth preset (with integrated parameters) with additional parameters
void bass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 2.0;
  cents[ch][0] = 5;  cents[ch][1] = -5;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = -12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 400; decayAmp[ch][1] = 400; decayAmp[ch][2] = 400;
  sustainAmp[ch][0] = 0.0; sustainAmp[ch][1] = 0.0; sustainAmp[ch][2] = 0.0;
  releaseAmp[ch][0] = 10;  releaseAmp[ch][1] = 10;  releaseAmp[ch][2] = 10;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 250;  decayFilter[ch][1] = 250;  decayFilter[ch][2] = 250;
  sustainFilter[ch][0] = 0.0; sustainFilter[ch][1] = 0.0; sustainFilter[ch][2] = 0.0;
  releaseFilter[ch][0] = 10;  releaseFilter[ch][1] = 10;  releaseFilter[ch][2] = 10;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  // Default waveform for voices
  waveforms[ch][0] = 0;  waveforms[ch][1] = 0;  waveforms[ch][2] = 1;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// brass_synth preset (menuIndex==5) with additional parameters
void brass_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = -4;  cents[ch][1] = 4;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 0;
  pan[ch][0] = -100;  pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 100;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 2500; decayAmp[ch][1] = 2500; decayAmp[ch][2] = 2500;
  sustainAmp[ch][0] = 0.8; sustainAmp[ch][1] = 0.8; sustainAmp[ch][2] = 0.8;
  releaseAmp[ch][0] = 250; releaseAmp[ch][1] = 250; releaseAmp[ch][2] = 250;

  attackFilter[ch][0] = 100;  attackFilter[ch][1] = 100;  attackFilter[ch][2] = 100;
  decayFilter[ch][0] = 2500;  decayFilter[ch][1] = 2500;  decayFilter[ch][2] = 2500;
  sustainFilter[ch][0] = 0.8; sustainFilter[ch][1] = 0.8; sustainFilter[ch][2] = 0.8;
  releaseFilter[ch][0] = 250; releaseFilter[ch][1] = 250; releaseFilter[ch][2] = 250;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 0; waveforms[ch][1] = 0; waveforms[ch][2] = 0;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}


// chiptune_synth preset (menuIndex==8) with additional parameters
void chiptune_synth(int ch, int p1, int p2, int p3, int p4, int p5, int p6) {
  octave[ch] = 4.0;
  cents[ch][0] = 0;  cents[ch][1] = 0;  cents[ch][2] = 0;
  semitones[ch][0] = 0; semitones[ch][1] = 0; semitones[ch][2] = 12;
  pan[ch][0] = -100; pan[ch][1] = 100; pan[ch][2] = 0;
  volume[ch][0] = 100; volume[ch][1] = 100; volume[ch][2] = 10;

  attackAmp[ch][0] = 0;  attackAmp[ch][1] = 0;  attackAmp[ch][2] = 0;
  decayAmp[ch][0] = 0;  decayAmp[ch][1] = 0;  decayAmp[ch][2] = 0;
  sustainAmp[ch][0] = 1.0; sustainAmp[ch][1] = 1.0; sustainAmp[ch][2] = 1.0;
  releaseAmp[ch][0] = 100; releaseAmp[ch][1] = 100; releaseAmp[ch][2] = 100;

  attackFilter[ch][0] = 0;  attackFilter[ch][1] = 0;  attackFilter[ch][2] = 0;
  decayFilter[ch][0] = 0;  decayFilter[ch][1] = 0;  decayFilter[ch][2] = 0;
  sustainFilter[ch][0] = 1.0; sustainFilter[ch][1] = 1.0; sustainFilter[ch][2] = 1.0;
  releaseFilter[ch][0] = 100; releaseFilter[ch][1] = 100; releaseFilter[ch][2] = 100;

  // Extreme filter modulation (p1, p2, p3)
  float minCutoff = 400.0,   maxCutoff = 6000.0;
  float minResonance = 0.1,  maxResonance = 5.0;
  float minFilterAmount = 1.0, maxFilterAmount = 10.0;
  float mappedCutoff       = minCutoff + (p1 / 64.0) * (maxCutoff - minCutoff);
  float mappedResonance    = minResonance + (p2 / 64.0) * (maxResonance - minResonance);
  float mappedFilterAmount = minFilterAmount + (p3 / 64.0) * (maxFilterAmount - minFilterAmount);
  cutoff[ch][0] = cutoff[ch][1] = cutoff[ch][2] = mappedCutoff;
  resonance[ch][0] = resonance[ch][1] = resonance[ch][2] = mappedResonance;
  filterAmount[ch][0] = filterAmount[ch][1] = filterAmount[ch][2] = mappedFilterAmount;

  waveforms[ch][0] = 1; waveforms[ch][1] = 1; waveforms[ch][2] = 1;

  // --- Additional modulation ---
  float offsetCents = -50.0 + (p4 / 64.0) * 100.0;
  int offsetSemitones = (int)round(-2.0 + (p4 / 64.0) * 4.0);
  for (int i = 0; i < 3; i++) {
      cents[ch][i] += offsetCents;
      semitones[ch][i] += offsetSemitones;
  }
  octave[ch] = 1.0 + (p5 / 64.0) * 7.0;
  int newWaveform = (int)floor((p6 / 64.0) * 8);
  waveforms[ch][0] = newWaveform;

  updateVals(ch);
}



//–––––– updateVals() Function ––––––//
// This function updates all the audio objects based on the current global parameters.
void updateVals(int ch) {

  for (int i = 0; i < POLY_VOICES; i++) {
    // Update amplitude envelopes
    Senvelope1[i]->attack(attackAmp[ch][i]);
    Senvelope1[i]->decay(decayAmp[ch][i]);
    Senvelope1[i]->sustain(sustainAmp[ch][i]);
    Senvelope1[i]->release(releaseAmp[ch][i]);

    Senvelope2[i]->attack(attackAmp[ch][i]);
    Senvelope2[i]->decay(decayAmp[ch][i]);
    Senvelope2[i]->sustain(sustainAmp[ch][i]);
    Senvelope2[i]->release(releaseAmp[ch][i]);

    // Update filter envelope
    SenvelopeFilter1[i]->attack(attackFilter[ch][i]);
    SenvelopeFilter1[i]->decay(decayFilter[ch][i]);
    SenvelopeFilter1[i]->sustain(sustainFilter[ch][i]);
    SenvelopeFilter1[i]->release(releaseFilter[ch][i]);

    // Set waveform for oscillator 1 (waveform1)
    if (waveforms[ch][0] == 0) {
      Swaveform1[i]->begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][0] == 1) {
      Swaveform1[i]->begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][0] == 2) {
      Swaveform1[i]->begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][0] == 3) {
      Swaveform1[i]->begin(waveformsArray[ch][POLY_VOICES]);
    } else if (waveforms[ch][0] == 4) {
      Swaveform1[i]->begin(waveformsArray[ch][4]);
    }
    Swaveform1[i]->amplitude(1.0);
    Swaveform1[i]->pulseWidth(pulseWidth[ch][0]);

    // Set waveform for oscillator 2 (waveform2)
    if (waveforms[ch][1] == 0) {
      Swaveform2[i]->begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][1] == 1) {
      Swaveform2[i]->begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][1] == 2) {
      Swaveform2[i]->begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][1] == 3) {
      Swaveform2[i]->begin(waveformsArray[ch][POLY_VOICES]);
    } else if (waveforms[ch][1] == 4) {
      Swaveform2[i]->begin(waveformsArray[ch][4]);
    }
    Swaveform2[i]->amplitude(1.0);
    Swaveform2[i]->pulseWidth(pulseWidth[ch][1]);

    // Set waveform for oscillator 3 (waveform3)
    if (waveforms[ch][2] == 0) {
      Swaveform3[i]->begin(waveformsArray[ch][0]);
    } else if (waveforms[ch][2] == 1) {
      Swaveform3[i]->begin(waveformsArray[ch][1]);
    } else if (waveforms[ch][2] == 2) {
      Swaveform3[i]->begin(waveformsArray[ch][2]);
    } else if (waveforms[ch][2] == 3) {
      Swaveform3[i]->begin(waveformsArray[ch][POLY_VOICES]);
    } else if (waveforms[ch][2] == 4) {
      Swaveform3[i]->begin(waveformsArray[ch][4]);
    }

    Swaveform3[i]->amplitude(1.0);
    Swaveform3[i]->pulseWidth(pulseWidth[ch][2]);

    // Update filter settings for both ladders
    Sladder1[i]->frequency(cutoff[ch][i]);
    Sladder1[i]->octaveControl(filterAmount[ch][i]);
    Sladder1[i]->resonance(resonance[ch][i]);

    Sladder2[i]->frequency(cutoff[ch][i]);
    Sladder2[i]->octaveControl(filterAmount[ch][i]);
    Sladder2[i]->resonance(resonance[ch][i]);

    // Update mixer gains (example: using pan[ch] and volume[ch] to calculate gain)
    Smixer1[i]->gain(0, max(0, (-pan[ch][0] + 100.0) * 0.005) * (volume[ch][0] / 100.0) * 0.33);
    Smixer1[i]->gain(1, max(0, (-pan[ch][1] + 100.0) * 0.005) * (volume[ch][1] / 100.0) * 0.33);
    Smixer1[i]->gain(2, max(0, (-pan[ch][2] + 100.0) * 0.005) * (volume[ch][2] / 100.0) * 0.33);

    Smixer2[i]->gain(0, max(0, (pan[ch][0] + 100.0) * 0.005) * (volume[ch][0] / 100.0) * 0.33);
    Smixer2[i]->gain(1, max(0, (pan[ch][1] + 100.0) * 0.005) * (volume[ch][1] / 100.0) * 0.33);
    Smixer2[i]->gain(2, max(0, (pan[ch][2] + 100.0) * 0.005) * (volume[ch][2] / 100.0) * 0.33);
  }
}

// Modify playSound() so that when a note is played, you store its start time.
void playSound(int note, int ch) {
  stopSound(note, ch);
  notePlaying[ch] += 1;
  if (notePlaying[ch] >= POLY_VOICES) {
    notePlaying[ch] = 0;
  }
  noteArray[ch][notePlaying[ch]] = note;
  voiceStartTime[ch][notePlaying[ch]] = millis();  // Record the start time for this voice

  // Play note with frequency calculations, etc.
  Swaveform1[notePlaying[ch]]->frequency(
    notesArray[note + semitones[ch][0]] * pow(2, cents[ch][0] / 1200.0) * pow(2, random(-2, 3) / 1200.0));
  Swaveform2[notePlaying[ch]]->frequency(
    notesArray[note + semitones[ch][1]] * pow(2, cents[ch][1] / 1200.0) * pow(2, random(-2, 3) / 1200.0));
  Swaveform3[notePlaying[ch]]->frequency(
    notesArray[note + semitones[ch][2]] * pow(2, cents[ch][2] / 1200.0) * pow(2, random(-2, 3) / 1200.0));
  Senvelope1[notePlaying[ch]]->noteOn();
  Senvelope2[notePlaying[ch]]->noteOn();
  SenvelopeFilter1[notePlaying[ch]]->noteOn();
}

void autoOffActiveNotes2() {
  if (pressedKeyCount[11] >= 1) return;

  unsigned long currentTime = millis();
  for (int ch = 0; ch < 2; ch++) {
    for (int i = 0; i < POLY_VOICES; i++) {
      if (noteArray[ch][i] != 0) {  // There is an active note on this voice.
        // Calculate total note duration including delay
        int delay_ms = mapf(SMP.param_settings[ch][DELAY], 0, maxfilterResolution, 0, maxParamVal[DELAY]);
        int attack_ms = mapf(SMP.param_settings[ch][ATTACK], 0, maxfilterResolution, 0, maxParamVal[ATTACK]);
        int hold_ms = mapf(SMP.param_settings[ch][HOLD], 0, maxfilterResolution, 0, maxParamVal[HOLD]);
        int decay_ms = mapf(SMP.param_settings[ch][DECAY], 0, maxfilterResolution, 0, maxParamVal[DECAY]);
        int release_ms = mapf(SMP.param_settings[ch][RELEASE], 0, maxfilterResolution, 0, maxParamVal[RELEASE]);

        int noteDuration = delay_ms + attack_ms + hold_ms + decay_ms + release_ms;

        if (currentTime - voiceStartTime[ch][i] >= noteDuration) {
          stopSound(noteArray[ch][i], ch);  // Properly ends envelope
        }
      }
    }
  }
}


// This function checks each voice and auto-offs active notes after AUTO_OFF_TIME.
void autoOffActiveNotes() {
  ////Serial.println(persistentNoteOn[11]);
if (pressedKeyCount[11]>=1) return;
  unsigned long currentTime = millis();
  for (int ch = 0; ch < 2; ch++) {
    for (int i = 0; i < POLY_VOICES; i++) {
      if (noteArray[ch][i] != 0) {  // There is an active note on this voice.
        if (currentTime - voiceStartTime[ch][i] >= AUTO_OFF_TIME) {
          stopSound(noteArray[ch][i], ch);  // This will call noteOff on all envelopes for that note.
        }
      }
    }
  }
}

// Example stopSound() function (assuming noteArray[] holds the note numbers for active voices)
void stopSound(int note, int ch) {
  for (int num = 0; num < POLY_VOICES; num++) {
    if (noteArray[ch][num] == note) {
      Senvelope1[num]->noteOff();
      Senvelope2[num]->noteOff();
      SenvelopeFilter1[num]->noteOff();
      noteArray[ch][num] = 0;
    }
  }
}





// Helper function to process filter adjustments
float processSynthAdjustment(SynthTypes synthType, int index, int encoder) {
  SMP.synth_settings[index][synthType] = currentMode->pos[encoder];
  //Serial.print(":::::");
  //Serial.println(SMP.synth_settings[index][synthType]);
  
  int mappedValue;
  
    const int maxIndex = 64;
    if (SMP.synth_settings[GLOB.currentChannel][synthType] > maxIndex) {
      SMP.synth_settings[GLOB.currentChannel][synthType] = maxIndex;
      currentMode->pos[3] = SMP.synth_settings[GLOB.currentChannel][synthType];
      Encoder[3].writeCounter((int32_t)currentMode->pos[3]);
    }
    mappedValue = mapf(SMP.synth_settings[GLOB.currentChannel][synthType], 0, maxIndex, 1, maxIndex + 1); 
  
  //Serial.print(synthType);
  //Serial.print(" #####---->");
  //Serial.println(mappedValue);
  return mappedValue;
}

// Update filter values
void updateSynthValue(SynthTypes synthType, int index, float value) {
  // Common function to update all filter types
  //Serial.print(synthType);
  //Serial.print(" S==>: ");
  //Serial.println(value);
  switch (synthType) {
    case INSTRUMENT:
      break;
    case PARAM1:
      break;

    case PARAM2:
      break;

    default: return;
  }  
}



void switchSynthVoice(int typeValue, int ch, int p1,int p2, int p3, int p4, int p5, int p6) {
    switch(typeValue) {
        case 1: bass_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 2: keys_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 3: chiptune_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 4: pad_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 5: wow_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 6: organ_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 7: flute_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 8: lead_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 9: arp_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        case 10: brass_synth(ch,p1,p2,p3,p4,p5,p6);
            break;
        default:
            // Optionally, handle any values of typeValue that are not supported.
            break;
    }
}