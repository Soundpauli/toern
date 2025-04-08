
 AudioPlayArrayResmp sound0;

 AudioPlayArrayResmp sound1;
 AudioPlayArrayResmp sound2;
 AudioPlayArrayResmp sound3;
 AudioPlayArrayResmp sound4;
 AudioPlayArrayResmp sound5;
 AudioPlayArrayResmp sound6;
 AudioPlayArrayResmp sound7;
 AudioPlayArrayResmp sound8;

 AudioAnalyzePeak         peak1; 

// GUItool: begin automatically generated code

/*** SYNTH ***/
EXTMEM AudioSynthWaveformDc     dc11_1;          //xy=885.3331756591797,2335.999809741974
EXTMEM AudioSynthWaveformDc     dc11_2;          //xy=885.3331546783447,2792.6664338111877
EXTMEM AudioSynthWaveformDc     dc11_3;          //xy=888.6665687561035,2562.666401386261

EXTMEM AudioSynthWaveform       waveform1_1;    //xy=901.9998054504395,2379.333081483841
EXTMEM AudioSynthWaveform       waveform2_1;    //xy=901.9998016357422,2459.3330664634705
EXTMEM AudioSynthWaveform       waveform3_1;    //xy=901.9998664855957,2419.333144426346

EXTMEM AudioSynthWaveform       waveform1_2;    //xy=905.3332786560059,2602.6664032936096
EXTMEM AudioSynthWaveform       waveform2_2;    //xy=905.3332138061523,2642.6663851737976
EXTMEM AudioSynthWaveform       waveform3_2;    //xy=905.3331756591797,2682.666467189789

EXTMEM AudioSynthWaveform       waveform1_3;    //xy=898.6666679382324,2849.3331842422485
EXTMEM AudioSynthWaveform       waveform2_3;    //xy=898.6666488647461,2895.999852180481
EXTMEM AudioSynthWaveform       waveform3_3;    //xy=898.6666259765625,2945.999895095825

EXTMEM AudioEffectEnvelope      envelopeFilter1_1; //xy=1085.3332500457764,2335.999782562256
EXTMEM AudioEffectEnvelope      envelopeFilter1_2; //xy=1091.9998664855957,2562.66646528244
EXTMEM AudioEffectEnvelope      envelopeFilter1_3; //xy=1102.000072479248,2792.666477203369

EXTMEM AudioMixer4              mixer1_1;       //xy=1115.3332633972168,2392.6662945747375
EXTMEM AudioMixer4              mixer2_1;       //xy=1115.3330421447754,2462.666419506073
EXTMEM AudioMixer4              mixer1_2;       //xy=1118.666660308838,2615.9998202323914
EXTMEM AudioMixer4              mixer2_2;       //xy=1118.6666374206543,2685.9997210502625
EXTMEM AudioMixer4              mixer1_3;       //xy=1121.999912261963,2862.6663970947266
EXTMEM AudioMixer4              mixer2_3;       //xy=1121.9999504089355,2932.6665229797363


EXTMEM AudioFilterLadder        ladder1_1;      //xy=1335.3330345153809,2335.9999260902405
EXTMEM AudioFilterLadder        ladder1_2;      //xy=1345.333194732666,2562.6663641929626
EXTMEM AudioFilterLadder        ladder1_3;      //xy=1361.9999103546143,2792.6663603782654

EXTMEM AudioFilterLadder        ladder2_1;      //xy=1335.3331146240234,2402.6664538383484
EXTMEM AudioFilterLadder        ladder2_2;      //xy=1348.6665267944336,2625.99982213974
EXTMEM AudioFilterLadder        ladder2_3;      //xy=1355.3331489562988,2872.666519165039


EXTMEM AudioEffectEnvelope      envelope1_1;    //xy=1515.3331184387207,2335.999848842621
EXTMEM AudioEffectEnvelope      envelope1_2;    //xy=1528.6664924621582,2562.666401386261
EXTMEM AudioEffectEnvelope      envelope1_3;    //xy=1531.9998664855957,2792.666473865509

EXTMEM AudioEffectEnvelope      envelope2_1;    //xy=1515.3331184387207,2402.6664566993713
EXTMEM AudioEffectEnvelope      envelope2_2;    //xy=1528.6663703918457,2625.9997782707214
EXTMEM AudioEffectEnvelope      envelope2_3;    //xy=1535.3332824707031,2872.666476249695

EXTMEM AudioMixer4              mixerL;         //xy=1917.3331298828125,2568.66650390625
EXTMEM AudioMixer4              mixerR;         //xy=1920,2640


/*** DRUMS*****/

AudioMixer4              SNchaosMix;     //xy=1345,1748.333251953125
AudioMixer4              HHchaosMix;     //xy=1396,1937.333251953125
AudioSynthWaveformDc     BDpitchAmt;     //xy=1408,1529.333251953125
AudioSynthWaveformModulated SNtone;         //xy=1503,1730.333251953125
AudioSynthWaveformModulated SNtone2;        //xy=1503,1780.333251953125
AudioSynthWaveformModulated HHtone2;        //xy=1553,1968.333251953125
AudioSynthWaveformModulated HHtone;         //xy=1557,1919.333251953125
AudioEffectEnvelope      BDpitchEnv;     //xy=1573,1528.333251953125
AudioMixer4              SNtoneMix;      //xy=1681,1754.333251953125
AudioMixer4              HHtoneMix;      //xy=1720,1955.333251953125
AudioMixer4              BDchaosMix;     //xy=1761,1546.333251953125
AudioEffectEnvelope      SNtoneEnv;      //xy=1838,1782.333251953125
AudioSynthWaveform       waveform14_1;   //xy=1838,3234.333251953125
AudioSynthWaveform       waveform14_2;   //xy=1838,3269.333251953125
AudioSynthNoiseWhite     SNnoise;        //xy=1847,1738.333251953125
AudioEffectEnvelope      HHtoneEnv;      //xy=1891,1972.333251953125
AudioSynthNoiseWhite     HHnoise;        //xy=1895,1934.333251953125
AudioSynthWaveformModulated BDsine;         //xy=1928,1530.333251953125
AudioSynthWaveformModulated BDsaw;          //xy=1930,1583.333251953125
AudioMixer4              SNMix;          //xy=2019,1752.333251953125
AudioMixer4              HHMix;          //xy=2062,1960.333251953125
AudioMixer4              BDMix;          //xy=2103,1558.333251953125
AudioEffectEnvelope      SNenv;          //xy=2157,1752.333251953125
AudioEffectEnvelope      HHenv;          //xy=2245,1947.333251953125
AudioEffectEnvelope      BDenv;          //xy=2265,1570.333251953125
AudioFilterStateVariable SNfilt;         //xy=2294,1695.333251953125
AudioFilterStateVariable HhFilter;       //xy=2402,1969.333251953125
AudioFilterStateVariable BDfilter;       //xy=2415,1584.333251953125
AudioFilterStateVariable HHfilt;         //xy=2424,1862.333251953125
AudioMixer4              HhFilterMix;    //xy=2589,1966.333251953125
AudioMixer4              BdFilterMix;    //xy=2610,1587.333251953125
AudioMixer4              SnFilterMix;    //xy=2619,1752.333251953125
AudioFilterStateVariable SnFilter;       //xy=2454,1742.333251953125

AudioMixer4              BDMixer;        //xy=2861,1604.333251953125
AudioMixer4              SNMixer;        //xy=2874,1745.333251953125
AudioMixer4              HHMixer;        //xy=2881,1848.333251953125


/*** SAMPLER ***/

AudioEffectEnvelope      envelope1;      //xy=3108,1647.333251953125
AudioEffectEnvelope      envelope2;      //xy=3109,1757.333251953125
AudioEffectEnvelope      envelope3;      //xy=3107,1848.333251953125
AudioEffectEnvelope      envelope4;      //xy=3108,1925.333251953125
AudioEffectEnvelope      envelope5;      //xy=3107,1995.333251953125
AudioEffectEnvelope      envelope6;      //xy=3104,2073.333251953125
AudioEffectEnvelope      envelope7;      //xy=3105,2156.333251953125
AudioEffectEnvelope      envelope8;      //xy=3103,2233.333251953125
AudioEffectEnvelope      envelope11;     //xy=2352,2610.333251953125
AudioEffectEnvelope      envelope12;      //xy=2252,2847
AudioEffectEnvelope      envelope13;     //xy=2258,3075.333251953125
AudioEffectEnvelope      envelope14;     //xy=2275,3249.333251953125


AudioEffectBitcrusher    bitcrusher1;    //xy=3281,1647.333251953125
AudioEffectBitcrusher    bitcrusher2;    //xy=3281,1757.333251953125
AudioEffectBitcrusher    bitcrusher3;    //xy=3284,1849.333251953125
AudioEffectBitcrusher    bitcrusher4;    //xy=3279,1924.333251953125
AudioEffectBitcrusher    bitcrusher5;    //xy=3279,1995.333251953125
AudioEffectBitcrusher    bitcrusher6;    //xy=3274,2073.333251953125
AudioEffectBitcrusher    bitcrusher7;    //xy=3274,2156.333251953125
AudioEffectBitcrusher    bitcrusher8;    //xy=3275,2233.333251953125
AudioEffectBitcrusher    bitcrusher11;   //xy=2530,2611.333251953125
AudioEffectBitcrusher    bitcrusher12;    //xy=2440,2847
AudioEffectBitcrusher    bitcrusher13;   //xy=2444,3073.333251953125
AudioEffectBitcrusher    bitcrusher14;   //xy=2458,3247.333251953125


AudioMixer4              synthmixer12;         //xy=3384,2868
AudioMixer4              synthmixer14;    //xy=3395,3261.333251953125
AudioMixer4              synthmixer11;    //xy=3399,2627.333251953125
AudioMixer4              synthmixer13;    //xy=3402,3086.333251953125
AudioMixer4              mixersynth_end; //xy=3748,2653.333251953125

AudioAmplifier           amp1;           //xy=3460,1646.333251953125
AudioAmplifier           amp2;           //xy=3459,1759.333251953125
AudioAmplifier           amp3;           //xy=3455,1849.333251953125
AudioAmplifier           amp4;           //xy=3454,1925.333251953125
AudioAmplifier           amp5;           //xy=3454,1996.333251953125
AudioAmplifier           amp6;           //xy=3453,2074.333251953125
AudioAmplifier           amp7;           //xy=3450,2155.333251953125
AudioAmplifier           amp8;           //xy=3449,2232.333251953125
AudioAmplifier           amp11;          //xy=2694,2612.333251953125
AudioAmplifier           amp12;           //xy=2612,2846
AudioAmplifier           amp13;          //xy=2620,3077.333251953125
AudioAmplifier           amp14;          //xy=2650,3248.333251953125


AudioFilterStateVariable filter1;        //xy=3613,1652.333251953125
AudioFilterStateVariable filter2;        //xy=3618,1764.333251953125
AudioFilterStateVariable filter3;        //xy=3618,1855.333251953125
AudioFilterStateVariable filter4;        //xy=3617,1932.333251953125
AudioFilterStateVariable filter5;        //xy=3618,2006.333251953125
AudioFilterStateVariable filter6;        //xy=3615,2081.333251953125
AudioFilterStateVariable filter7;        //xy=3615,2160.333251953125
AudioFilterStateVariable filter8;        //xy=3612,2238.333251953125
AudioFilterStateVariable filter11;       //xy=2840,2619.333251953125
AudioFilterStateVariable filter12;       //xy=2748,2853.9999999999995
AudioFilterStateVariable filter13;       //xy=2773,3081.333251953125
AudioFilterStateVariable filter14;       //xy=2795,3255.333251953125

AudioMixer4              filtermixer1;   //xy=3785,1659.333251953125
AudioMixer4              filtermixer2;   //xy=3792,1770.333251953125
AudioMixer4              filtermixer3;   //xy=3792,1861.333251953125
AudioMixer4              filtermixer4;   //xy=3789,1938.333251953125
AudioMixer4              filtermixer5;   //xy=3790,2014.333251953125
AudioMixer4              filtermixer6;   //xy=3789,2089.333251953125
AudioMixer4              filtermixer7;   //xy=3782,2165.333251953125
AudioMixer4              filtermixer8;   //xy=3784,2242.333251953125
AudioMixer4              filtermixer11;  //xy=3009,2626.333251953125
AudioMixer4              filtermixer12;         //xy=2914,2874
AudioMixer4              filtermixer13;  //xy=2947,3087.333251953125
AudioMixer4              filtermixer14;  //xy=2957,3261.333251953125


AudioEffectFreeverb      freeverb1;      //xy=3931,1619.333251953125
AudioEffectFreeverb      freeverb2;      //xy=3942,1758.333251953125
AudioEffectFreeverb      freeverb7;      //xy=3940,2129.333251953125
AudioEffectFreeverb      freeverb8;      //xy=3946,2214.333251953125
AudioEffectFreeverb      freeverb11;     //xy=3174,2667.333251953125
AudioEffectFreeverb      freeverb12;      //xy=3080,2904
AudioEffectFreeverb      freeverb13;     //xy=3109,3124.333251953125
AudioEffectFreeverb      freeverb14;     //xy=3125,3313.333251953125

AudioEffectFlange        flange11;       //xy=3173,2580.333251953125
AudioEffectFlange        flange12;        //xy=3068,2842
AudioEffectFlange        flange13;       //xy=3109,3057.333251953125
AudioEffectFlange        flange14;       //xy=3121,3231.333251953125


AudioMixer4              freeverbmixer1; //xy=4145,1644.333251953125
AudioMixer4              freeverbmixer2; //xy=4159,1788.333251953125
AudioMixer4              freeverbmixer7; //xy=4151,2147.333251953125
AudioMixer4              freeverbmixer8; //xy=4151,2239.333251953125


AudioSynthWaveform       waveform12_1;      //xy=1826,2825
AudioSynthWaveform       waveform12_2;      //xy=1830,2863
AudioSynthWaveform       waveform13_1;   //xy=1818,3047.333251953125
AudioSynthWaveform       waveform13_2;   //xy=1819,3085.333251953125

AudioMixer4              waveformmixer11;         //xy=2134,2608
AudioMixer4              waveformmixer12;         //xy=2040,2847
AudioMixer4              waveformmixer13; //xy=2066,3078.333251953125
AudioMixer4              waveformmixer14; //xy=2075,3250.333251953125


AudioMixer4              mixer1;         //xy=4381,1894.333251953125
AudioMixer4              mixer2;         //xy=4381,2079.333251953125
AudioMixer4              mixer_end;      //xy=4509,2360.333251953125

AudioAmplifier           amp0;           //xy=4617,2106.333251953125
AudioEffectEnvelope      envelope0;      //xy=4622,2020.333251953125
AudioPlaySdRaw           playRaw0;       //xy=4622,2062.333251953125
AudioMixer4              mixerPlay;      //xy=4724,2362.333251953125
AudioMixer4              mixer0;         //xy=4849,2060.333251953125
AudioInputI2S            is23;           //xy=4880,2386.333251953125
AudioRecordQueue         queue1;         //xy=4887,2478.333251953125
AudioOutputI2S           i2s1;           //xy=4891,2274.333251953125


AudioConnection          patchCord1(dc11_1, envelopeFilter1_1);
AudioConnection          patchCord2(dc11_3, envelopeFilter1_3);
AudioConnection          patchCord3(dc11_2, envelopeFilter1_2);
AudioConnection          patchCord4(waveform1_3, 0, mixer1_3, 0);
AudioConnection          patchCord5(waveform1_3, 0, mixer2_3, 0);
AudioConnection          patchCord6(waveform2_3, 0, mixer1_3, 1);
AudioConnection          patchCord7(waveform2_3, 0, mixer2_3, 1);
AudioConnection          patchCord8(waveform3_3, 0, mixer1_3, 2);
AudioConnection          patchCord9(waveform3_3, 0, mixer2_3, 2);
AudioConnection          patchCord10(waveform1_1, 0, mixer1_1, 0);
AudioConnection          patchCord11(waveform1_1, 0, mixer2_1, 0);
AudioConnection          patchCord12(waveform3_1, 0, mixer1_1, 2);
AudioConnection          patchCord13(waveform3_1, 0, mixer2_1, 2);
AudioConnection          patchCord14(waveform2_1, 0, mixer1_1, 1);
AudioConnection          patchCord15(waveform2_1, 0, mixer2_1, 1);
AudioConnection          patchCord16(waveform1_2, 0, mixer1_2, 0);
AudioConnection          patchCord17(waveform1_2, 0, mixer2_2, 0);
AudioConnection          patchCord18(waveform2_2, 0, mixer1_2, 1);
AudioConnection          patchCord19(waveform2_2, 0, mixer2_2, 1);
AudioConnection          patchCord20(waveform3_2, 0, mixer1_2, 2);
AudioConnection          patchCord21(waveform3_2, 0, mixer2_2, 2);
AudioConnection          patchCord22(envelopeFilter1_1, 0, ladder1_1, 1);
AudioConnection          patchCord23(envelopeFilter1_1, 0, ladder2_1, 1);
AudioConnection          patchCord24(envelopeFilter1_2, 0, ladder1_2, 1);
AudioConnection          patchCord25(envelopeFilter1_2, 0, ladder2_2, 1);
AudioConnection          patchCord26(envelopeFilter1_3, 0, ladder1_3, 1);
AudioConnection          patchCord27(envelopeFilter1_3, 0, ladder2_3, 1);
AudioConnection          patchCord28(mixer1_1, 0, ladder1_1, 0);
AudioConnection          patchCord29(mixer2_1, 0, ladder2_1, 0);
AudioConnection          patchCord30(mixer1_2, 0, ladder1_2, 0);
AudioConnection          patchCord31(mixer2_2, 0, ladder2_2, 0);
AudioConnection          patchCord32(mixer1_3, 0, ladder1_3, 0);
AudioConnection          patchCord33(mixer2_3, 0, ladder2_3, 0);
AudioConnection          patchCord34(ladder1_1, envelope1_1);
AudioConnection          patchCord35(ladder2_1, envelope2_1);
AudioConnection          patchCord36(SNchaosMix, 0, SNtone, 0);
AudioConnection          patchCord37(SNchaosMix, 0, SNtone2, 0);
AudioConnection          patchCord38(ladder1_2, envelope1_2);
AudioConnection          patchCord39(ladder2_2, envelope2_2);
AudioConnection          patchCord40(ladder2_3, envelope2_3);
AudioConnection          patchCord41(ladder1_3, envelope1_3);
AudioConnection          patchCord42(HHchaosMix, 0, HHtone, 0);
AudioConnection          patchCord43(HHchaosMix, 0, HHtone2, 0);
AudioConnection          patchCord44(BDpitchAmt, BDpitchEnv);
AudioConnection          patchCord45(SNtone, 0, SNtoneMix, 0);
AudioConnection          patchCord46(SNtone2, 0, SNchaosMix, 0);
AudioConnection          patchCord47(SNtone2, 0, SNtoneMix, 1);
AudioConnection          patchCord48(envelope1_1, 0, mixerL, 0);
AudioConnection          patchCord49(envelope2_1, 0, mixerR, 0);
AudioConnection          patchCord50(envelope1_2, 0, mixerL, 1);
AudioConnection          patchCord51(envelope2_2, 0, mixerR, 1);
AudioConnection          patchCord52(envelope1_3, 0, mixerL, 2);
AudioConnection          patchCord53(envelope2_3, 0, mixerR, 2);
AudioConnection          patchCord54(HHtone2, 0, HHchaosMix, 0);
AudioConnection          patchCord55(HHtone2, 0, HHtoneMix, 1);
AudioConnection          patchCord56(HHtone, 0, HHtoneMix, 0);
AudioConnection          patchCord57(BDpitchEnv, 0, BDchaosMix, 0);
AudioConnection          patchCord58(SNtoneMix, SNtoneEnv);
AudioConnection          patchCord59(HHtoneMix, HHtoneEnv);
AudioConnection          patchCord60(BDchaosMix, 0, BDsine, 0);
AudioConnection          patchCord61(BDchaosMix, 0, BDsaw, 0);
AudioConnection          patchCord62(BDchaosMix, 0, BDsaw, 1);
AudioConnection          patchCord63(waveform13_1, 0, waveformmixer13, 0);
AudioConnection          patchCord64(waveform13_2, 0, waveformmixer13, 1);
AudioConnection          patchCord65(waveform12_1, 0, waveformmixer12, 0);
AudioConnection          patchCord66(waveform12_2, 0, waveformmixer12, 1);
AudioConnection          patchCord67(SNtoneEnv, 0, SNMix, 1);
AudioConnection          patchCord68(waveform14_1, 0, waveformmixer14, 0);
AudioConnection          patchCord69(waveform14_2, 0, waveformmixer14, 1);
AudioConnection          patchCord70(SNnoise, 0, SNMix, 0);
AudioConnection          patchCord71(HHtoneEnv, 0, HHMix, 1);
AudioConnection          patchCord72(HHnoise, 0, HHMix, 0);
AudioConnection          patchCord73(mixerL, 0, waveformmixer11, 0);
AudioConnection          patchCord74(mixerR, 0, waveformmixer11, 1);
AudioConnection          patchCord75(BDsine, 0, BDMix, 0);
AudioConnection          patchCord76(BDsaw, 0, BDMix, 1);
AudioConnection          patchCord77(BDsaw, 0, BDchaosMix, 1);
AudioConnection          patchCord78(SNMix, SNenv);
AudioConnection          patchCord79(waveformmixer12, envelope12);
AudioConnection          patchCord80(HHMix, HHenv);
AudioConnection          patchCord81(waveformmixer13, envelope13);
AudioConnection          patchCord82(waveformmixer14, envelope14);
AudioConnection          patchCord83(BDMix, BDenv);
AudioConnection          patchCord84(waveformmixer11, envelope11);
AudioConnection          patchCord85(SNenv, 0, SNfilt, 0);
AudioConnection          patchCord86(HHenv, 0, HHfilt, 0);
AudioConnection          patchCord87(envelope12, bitcrusher12);
AudioConnection          patchCord88(envelope13, bitcrusher13);
AudioConnection          patchCord89(BDenv, 0, BDfilter, 0);
AudioConnection          patchCord90(envelope14, bitcrusher14);
AudioConnection          patchCord91(SNfilt, 1, SnFilter, 0);
AudioConnection          patchCord92(envelope11, bitcrusher11);
AudioConnection          patchCord93(HhFilter, 0, HhFilterMix, 0);
AudioConnection          patchCord94(HhFilter, 2, HhFilterMix, 1);
AudioConnection          patchCord95(BDfilter, 0, BdFilterMix, 0);
AudioConnection          patchCord96(BDfilter, 2, BdFilterMix, 1);
AudioConnection          patchCord97(HHfilt, 1, HhFilter, 0);
AudioConnection          patchCord98(bitcrusher12, amp12);
AudioConnection          patchCord99(bitcrusher13, amp13);
AudioConnection          patchCord100(SnFilter, 0, SnFilterMix, 0);
AudioConnection          patchCord101(SnFilter, 2, SnFilterMix, 1);
AudioConnection          patchCord102(bitcrusher14, amp14);
AudioConnection          patchCord103(bitcrusher11, amp11);
AudioConnection          patchCord104(HhFilterMix, 0, HHMixer, 1);
AudioConnection          patchCord105(BdFilterMix, 0, BDMixer, 1);
AudioConnection          patchCord106(amp12, 0, filter12, 0);
AudioConnection          patchCord107(SnFilterMix, 0, SNMixer, 1);
AudioConnection          patchCord108(amp13, 0, filter13, 0);
AudioConnection          patchCord109(amp14, 0, filter14, 0);
AudioConnection          patchCord110(amp11, 0, filter11, 0);
AudioConnection          patchCord111(filter12, 0, filtermixer12, 0);
AudioConnection          patchCord112(filter12, 1, filtermixer12, 1);
AudioConnection          patchCord113(filter12, 2, filtermixer12, 2);
AudioConnection          patchCord114(filter13, 0, filtermixer13, 0);
AudioConnection          patchCord115(filter13, 1, filtermixer13, 1);
AudioConnection          patchCord116(filter13, 2, filtermixer13, 2);
AudioConnection          patchCord117(filter14, 0, filtermixer14, 0);
AudioConnection          patchCord118(filter14, 1, filtermixer14, 1);
AudioConnection          patchCord119(filter14, 2, filtermixer14, 2);
AudioConnection          patchCord120(filter11, 0, filtermixer11, 0);
AudioConnection          patchCord121(filter11, 1, filtermixer11, 1);
AudioConnection          patchCord122(filter11, 2, filtermixer11, 2);
AudioConnection          patchCord123(BDMixer, envelope1);
AudioConnection          patchCord124(SNMixer, envelope2);
AudioConnection          patchCord125(HHMixer, envelope3);
AudioConnection          patchCord126(filtermixer12, 0, synthmixer12, 3);
AudioConnection          patchCord127(filtermixer12, freeverb12);
AudioConnection          patchCord128(filtermixer12, flange12);
AudioConnection          patchCord129(filtermixer13, flange13);
AudioConnection          patchCord130(filtermixer13, freeverb13);
AudioConnection          patchCord131(filtermixer13, 0, synthmixer13, 3);
AudioConnection          patchCord132(filtermixer14, flange14);
AudioConnection          patchCord133(filtermixer14, freeverb14);
AudioConnection          patchCord134(filtermixer14, 0, synthmixer14, 3);
AudioConnection          patchCord135(filtermixer11, flange11);
AudioConnection          patchCord136(filtermixer11, freeverb11);
AudioConnection          patchCord137(filtermixer11, 0, synthmixer11, 3);
AudioConnection          patchCord138(flange12, 0, synthmixer12, 0);
AudioConnection          patchCord139(freeverb12, 0, synthmixer12, 1);
AudioConnection          patchCord140(envelope8, bitcrusher8);
AudioConnection          patchCord141(envelope6, bitcrusher6);
AudioConnection          patchCord142(envelope7, bitcrusher7);
AudioConnection          patchCord143(envelope3, bitcrusher3);
AudioConnection          patchCord144(envelope1, bitcrusher1);
AudioConnection          patchCord145(envelope5, bitcrusher5);
AudioConnection          patchCord146(envelope4, bitcrusher4);
AudioConnection          patchCord147(envelope2, bitcrusher2);
AudioConnection          patchCord148(flange13, 0, synthmixer13, 0);
AudioConnection          patchCord149(freeverb13, 0, synthmixer13, 1);
AudioConnection          patchCord150(flange14, 0, synthmixer14, 0);
AudioConnection          patchCord151(freeverb14, 0, synthmixer14, 1);
AudioConnection          patchCord152(flange11, 0, synthmixer11, 0);
AudioConnection          patchCord153(freeverb11, 0, synthmixer11, 1);
AudioConnection          patchCord154(bitcrusher6, amp6);
AudioConnection          patchCord155(bitcrusher7, amp7);
AudioConnection          patchCord156(bitcrusher8, amp8);
AudioConnection          patchCord157(bitcrusher4, amp4);
AudioConnection          patchCord158(bitcrusher5, amp5);
AudioConnection          patchCord159(bitcrusher1, amp1);
AudioConnection          patchCord160(bitcrusher2, amp2);
AudioConnection          patchCord161(bitcrusher3, amp3);
AudioConnection          patchCord162(synthmixer12, 0, mixersynth_end, 1);
AudioConnection          patchCord163(synthmixer14, 0, mixersynth_end, 3);
AudioConnection          patchCord164(synthmixer11, 0, mixersynth_end, 0);
AudioConnection          patchCord165(synthmixer13, 0, mixersynth_end, 2);
AudioConnection          patchCord166(amp8, 0, filter8, 0);
AudioConnection          patchCord167(amp7, 0, filter7, 0);
AudioConnection          patchCord168(amp6, 0, filter6, 0);
AudioConnection          patchCord169(amp4, 0, filter4, 0);
AudioConnection          patchCord170(amp5, 0, filter5, 0);
AudioConnection          patchCord171(amp3, 0, filter3, 0);
AudioConnection          patchCord172(amp2, 0, filter2, 0);
AudioConnection          patchCord173(amp1, 0, filter1, 0);
AudioConnection          patchCord174(filter1, 0, filtermixer1, 0);
AudioConnection          patchCord175(filter1, 1, filtermixer1, 1);
AudioConnection          patchCord176(filter1, 2, filtermixer1, 2);
AudioConnection          patchCord177(filter8, 0, filtermixer8, 0);
AudioConnection          patchCord178(filter8, 1, filtermixer8, 1);
AudioConnection          patchCord179(filter8, 2, filtermixer8, 2);
AudioConnection          patchCord180(filter6, 0, filtermixer6, 0);
AudioConnection          patchCord181(filter6, 1, filtermixer6, 1);
AudioConnection          patchCord182(filter6, 2, filtermixer6, 2);
AudioConnection          patchCord183(filter7, 0, filtermixer7, 0);
AudioConnection          patchCord184(filter7, 1, filtermixer7, 1);
AudioConnection          patchCord185(filter7, 2, filtermixer7, 2);
AudioConnection          patchCord186(filter4, 0, filtermixer4, 0);
AudioConnection          patchCord187(filter4, 1, filtermixer4, 1);
AudioConnection          patchCord188(filter4, 2, filtermixer4, 2);
AudioConnection          patchCord189(filter2, 0, filtermixer2, 0);
AudioConnection          patchCord190(filter2, 1, filtermixer2, 1);
AudioConnection          patchCord191(filter2, 2, filtermixer2, 2);
AudioConnection          patchCord192(filter3, 0, filtermixer3, 0);
AudioConnection          patchCord193(filter3, 1, filtermixer3, 1);
AudioConnection          patchCord194(filter3, 2, filtermixer3, 2);
AudioConnection          patchCord195(filter5, 0, filtermixer5, 0);
AudioConnection          patchCord196(filter5, 1, filtermixer5, 1);
AudioConnection          patchCord197(filter5, 2, filtermixer5, 2);
AudioConnection          patchCord198(mixersynth_end, 0, mixer_end, 2);
AudioConnection          patchCord199(filtermixer7, freeverb7);
AudioConnection          patchCord200(filtermixer7, 0, freeverbmixer7, 3);
AudioConnection          patchCord201(filtermixer1, freeverb1);
AudioConnection          patchCord202(filtermixer1, 0, freeverbmixer1, 3);
AudioConnection          patchCord203(filtermixer8, freeverb8);
AudioConnection          patchCord204(filtermixer8, 0, freeverbmixer8, 3);
AudioConnection          patchCord205(filtermixer4, 0, mixer1, 3);
AudioConnection          patchCord206(filtermixer6, 0, mixer2, 1);
AudioConnection          patchCord207(filtermixer5, 0, mixer2, 0);
AudioConnection          patchCord208(filtermixer2, freeverb2);
AudioConnection          patchCord209(filtermixer2, 0, freeverbmixer2, 3);
AudioConnection          patchCord210(filtermixer3, 0, mixer1, 2);
AudioConnection          patchCord211(freeverb1, 0, freeverbmixer1, 0);
AudioConnection          patchCord212(freeverb7, 0, freeverbmixer7, 0);
AudioConnection          patchCord213(freeverb2, 0, freeverbmixer2, 0);
AudioConnection          patchCord214(freeverb8, 0, freeverbmixer8, 0);
AudioConnection          patchCord215(freeverbmixer1, 0, mixer1, 0);
AudioConnection          patchCord216(freeverbmixer7, 0, mixer2, 2);
AudioConnection          patchCord217(freeverbmixer8, 0, mixer2, 3);
AudioConnection          patchCord218(freeverbmixer2, 0, mixer1, 1);
AudioConnection          patchCord219(mixer1, 0, mixer_end, 0);
AudioConnection          patchCord220(mixer2, 0, mixer_end, 1);
AudioConnection          patchCord221(mixer_end, 0, mixerPlay, 0);
AudioConnection          patchCord222(mixer_end, 0, mixerPlay, 1);
AudioConnection          patchCord223(amp0, 0, mixer0, 2);
AudioConnection          patchCord224(envelope0, 0, mixer0, 0);
AudioConnection          patchCord225(playRaw0, 0, mixer0, 1);
AudioConnection          patchCord226(mixerPlay, 0, i2s1, 0);
AudioConnection          patchCord227(mixerPlay, 0, i2s1, 1);
AudioConnection          patchCord228(mixer0, 0, mixerPlay, 2);
AudioControlSGTL5000     sgtl5000_1;     //xy=4892,2433.333251953125
// GUItool: end automatically generated code



// --- ADDITIONAL sound declarations below here ... ---//
// ---  AudioPlayArrayResmp doesnt exist in GUItool ---//


AudioConnection          patchCord1001(playRaw0, 0, peak1, 0);
AudioConnection          patchCord1002(is23, 0, queue1, 0);

 AudioConnection snd0(sound0, envelope0);

 AudioConnection snd1(sound1, 0, BDMixer,0);
 AudioConnection snd3(sound2, 0, SNMixer,0);
 AudioConnection snd2(sound3, 0, HHMixer,0);
 AudioConnection snd4(sound4, envelope4);
 AudioConnection snd5(sound5, envelope5);
 AudioConnection snd6(sound6, envelope6);
 AudioConnection snd7(sound7, envelope7);
 AudioConnection snd8(sound8, envelope8);

