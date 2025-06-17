
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
  AudioAnalyzePeak        peakRec; 

  // GUItool: begin automatically generated code

  AudioMixer4              SNchaosMix;     //xy=208.00000762939453,728.3333892822266
  AudioMixer4              HHchaosMix;     //xy=259.66662216186523,917.7221794128418
  AudioSynthWaveformDc     BDpitchAmt;     //xy=271.4722213745117,509.3055229187012
  AudioSynthWaveformModulated SNtone;         //xy=366.3333282470703,710.0001068115234
  AudioSynthWaveformModulated SNtone2;        //xy=366.47222900390625,760.0555877685547
  
  AudioSynthWaveformModulated HHtone2;        //xy=416.7777671813965,948.611141204834
  AudioSynthWaveformModulated HHtone;         //xy=420.9999809265137,899.4999961853027
  AudioEffectEnvelope      BDpitchEnv;     //xy=436.72222900390625,508.4445080757141
  
  AudioSynthWaveform       waveform13_1;   //xy=451.3334426879883,1699.8573150634766
  AudioSynthWaveform       waveform13_2;   //xy=452.3334426879883,1737.8573150634766

  AudioSynthWaveform       waveform11_1;   //xy=451.3334426879883,1699.8573150634766
  AudioSynthWaveform       waveform11_2;   //xy=452.3334426879883,1737.8573150634766
  
  AudioSynthWaveform       waveform14_1;   //xy=471.76201248168945,1886.571632385254
  AudioSynthWaveform       waveform14_2;   //xy=471.76201248168945,1921.571632385254

  AudioMixer4              SNtoneMix;      //xy=544.8611526489258,734.4166851043701
  AudioMixer4              HHtoneMix;      //xy=583.8888740539551,935.6944236755371
  AudioMixer4              BDchaosMix;     //xy=624.1944580078125,526.000011920929
  AudioMixer4              mixer_waveform12; //xy=678.0476951599121,1575.571548461914
  AudioMixer4              mixer_waveform11; //xy=685.761962890625,1408.1429443359375
  AudioMixer4              mixer_waveform13; //xy=685.3334426879883,1722.5716514587402
  AudioEffectEnvelope      SNtoneEnv;      //xy=701.0000076293945,762.7500591278076
  AudioMixer4              mixer_waveform14; //xy=702.7620124816895,1892.571632385254
  AudioSynthNoiseWhite     SNnoise;        //xy=710.0000076293945,718.5000581741333
  AudioEffectEnvelope      HHtoneEnv;      //xy=754.9999580383301,952.5000085830688
  AudioSynthNoiseWhite     HHnoise;        //xy=758.7499580383301,914.7500076293945
  AudioSynthWaveformModulated BDsine;         //xy=791.833324432373,510.58330154418945
  AudioSynthWaveformModulated BDsaw;          //xy=793.8055229187012,563.9167056083679
  AudioMixer4              SNMix;          //xy=882.6786193847656,732.6428604125977
  
  AudioMixer4              HHMix;          //xy=925.3571243286133,940.2142524719238
  AudioMixer4              BDMix;          //xy=966.4047698974609,538.8333516120911
  AudioEffectEnvelope      SNenv;     //xy=1020.9998779296875,732.3333740234375
  AudioEffectEnvelope      HHenv;     //xy=1108.6666717529297,927.3333606719971
  AudioEffectEnvelope      envelope11;     //xy=1117.047519683838,1404.8930644989014
  AudioEffectEnvelope      envelope13;     //xy=1119.904899597168,1723.7143659591675
  AudioEffectEnvelope      BDenv;      //xy=1128.0000267028809,550.666718006134
  AudioEffectEnvelope      envelope12;     //xy=1125.119052886963,1578.261806488037
  AudioEffectEnvelope      envelope14;     //xy=1126.9999961853027,1895.095314025879
  AudioFilterStateVariable SNfilt;       //xy=1157,675
  AudioFilterStateVariable HhFilter;       //xy=1265.6666221618652,949.3333625793457
  AudioFilterStateVariable BDfilter;        //xy=1278.9999542236328,564.0000100135803
  AudioFilterStateVariable HHfilt;       //xy=1287,842
  AudioEffectBitcrusher    bitcrusher11;   //xy=1293.2856407165527,1405.7141914367676
  AudioEffectBitcrusher    bitcrusher13;   //xy=1295.90482711792,1723.3809914588928
  AudioEffectBitcrusher    bitcrusher12;   //xy=1305.6192207336426,1578.9285535812378
  AudioFilterStateVariable SnFilter;       //xy=1317,722.0000610351562
  AudioEffectBitcrusher    bitcrusher14;   //xy=1315.333209991455,1893.4286847114563
  AudioMixer4              HhFilterMix;         //xy=1452.3332748413086,946.0000228881836
  AudioAmplifier           amp13;          //xy=1453.2620124816895,1723.6310739517212
  AudioAmplifier           amp11;          //xy=1465.7502098083496,1406.2141513824463
  AudioMixer4              BdFilterMix;         //xy=1473.333293914795,567.6666970252991
  AudioAmplifier           amp12;          //xy=1469.476203918457,1576.7618465423584
  AudioMixer4              SnFilterMix;         //xy=1482.3333740234375,732.3333740234375
  AudioAmplifier           amp14;          //xy=1483.1904373168945,1894.4286451339722
  AudioFilterStateVariable filter13;       //xy=1606.5715065002441,1727.0476989746094
  AudioFilterStateVariable filter12;       //xy=1620.702449798584,1584.6786155700684
  AudioFilterStateVariable filter11;       //xy=1625.9166831970215,1413.7500228881836
  AudioFilterStateVariable filter14;       //xy=1628.3334274291992,1901.4287252426147
  AudioMixer4              BDMixer;       //xy=1724.4168128967285,584.1665878295898
  AudioMixer4              SNMixer;       //xy=1737.7499389648438,725.833309173584
  AudioMixer4              HHMixer;       //xy=1744.3333282470703,828.9166774749756
  AudioMixer4              filtermixer13;  //xy=1780.5715103149414,1733.0477809906006
  AudioMixer4              filtermixer14;  //xy=1790.3333549499512,1907.4286851882935
  AudioMixer4              filtermixer11;  //xy=1794.8809204101562,1420.4284896850586
  AudioMixer4              filtermixer12;  //xy=1796.3691596984863,1591.09522056579AudioEffectFlange        flange13;       //xy=1942.1905975341797,1703.8096947669983
  AudioEffectFreeverb      freeverb13;     //xy=1942.4763107299805,1770.0953216552734
  //AudioEffectFlange        flange12;       //xy=1953.5238914489746,1554.1428842544556
  //AudioEffectFlange        flange14;       //xy=1954.047519683838,1877.142855644226
  AudioEffectFreeverb      freeverb12;     //xy=1956.3810539245605,1624.4286155700684
  AudioEffectFreeverb      freeverb11;     //xy=1959.3810005187988,1461.4285488128662
  AudioEffectFreeverb      freeverb14;     //xy=1958.3334312438965,1959.0001192092896
  //AudioEffectFlange        flange11;       //xy=1962.6667137145996,1392.5714235305786
  AudioEffectEnvelope      envelope6;      //xy=1967.2497863769531,1053.083306312561
  AudioEffectEnvelope      envelope8;      //xy=1966.9164428710938,1213.916633605957
  AudioEffectEnvelope      envelope7;      //xy=1968.9997863769531,1136.5833082199097
  AudioEffectEnvelope      envelope3;      //xy=1970.4997863769531,828.8333034515381
  AudioEffectEnvelope      envelope1;      //xy=1971.999771118164,627.3332901000977
  AudioEffectEnvelope      envelope4;      //xy=1971.2497863769531,905.5833053588867
  AudioEffectEnvelope      envelope5;      //xy=1970.9997863769531,975.5833053588867
  AudioEffectEnvelope      envelope2;      //xy=1972.9997863769531,737.0833020210266
  AudioEffectBitcrusher    bitcrusher6;    //xy=2137.0276222229004,1053.4166803359985
  AudioEffectBitcrusher    bitcrusher7;    //xy=2137.5276260375977,1136.9166812896729
  AudioEffectBitcrusher    bitcrusher8;    //xy=2138.527614593506,1213.0832996368408
  AudioEffectBitcrusher    bitcrusher4;    //xy=2142.2776260375977,904.6666793823242
  AudioEffectBitcrusher    bitcrusher5;    //xy=2142.2776222229004,975.6666793823242
  AudioEffectBitcrusher    bitcrusher1;    //xy=2144.2776222229004,627.4166750907898
  AudioEffectBitcrusher    bitcrusher2;    //xy=2144.2776222229004,737.416675567627
  AudioEffectBitcrusher    bitcrusher3;    //xy=2147.2776222229004,829.9166774749756
  /* newDMAMEM*/
  AudioMixer4              synthmixer14;   //xy=2158.8095703125,1913.1903076171875
  AudioMixer4              synthmixer11;    //xy=2164.618827819824,1437.9999752044678
  AudioMixer4              synthmixer13;    //xy=2163.666660308838,1734.2382926940918
  AudioMixer4              synthmixer12;    //xy=2170.476177215576,1589.9999752044678
  AudioAmplifier           amp8;           //xy=2312.8845596313477,1212.8334312438965
  AudioAmplifier           amp7;           //xy=2313.5511837005615,1135.0278873443604
  AudioAmplifier           amp6;           //xy=2316.5234756469727,1054.5001773834229
  AudioAmplifier           amp4;           //xy=2317.6345596313477,905.722282409668
  AudioAmplifier           amp3;           //xy=2318.162269592285,829.0278835296631
  AudioAmplifier           amp5;           //xy=2317.967933654785,976.1668014526367
  AudioAmplifier           amp2;           //xy=2322.0233764648438,739.2858781814575
  AudioAmplifier           amp1;           //xy=2323.3291397094727,626.6111946105957
  AudioMixer4              mixersynth_end; //xy=2409.190528869629,1621.1427688598633
  AudioFilterStateVariable filter1;        //xy=2476.9997940063477,632.8333015441895
  AudioFilterStateVariable filter8;        //xy=2475.7497940063477,1218.5833435058594
  AudioFilterStateVariable filter7;        //xy=2478.1663818359375,1140.333339691162
  AudioFilterStateVariable filter6;        //xy=2478.4997177124023,1061.7499198913574
  AudioFilterStateVariable filter4;        //xy=2480.2497940063477,912.3333053588867
  AudioFilterStateVariable filter2;        //xy=2481.606887817383,744.9404106140137
  AudioFilterStateVariable filter3;        //xy=2481.7497959136963,835.0833034515381
  AudioFilterStateVariable filter5;        //xy=2481.6663818359375,986.8332901000977
  AudioMixer4              filtermixer7;   //xy=2645.749801635742,1145.3333072662354
  AudioMixer4              filtermixer1;   //xy=2648.749797821045,639.5833015441895
  AudioMixer4              filtermixer8;   //xy=2647.166343688965,1222.7500076293945
  AudioMixer4              filtermixer4;   //xy=2652.749797821045,918.5833044052124
  AudioMixer4              filtermixer6;   //xy=2652.4163856506348,1069.6666679382324
  AudioMixer4              filtermixer5;   //xy=2653.666305541992,994.4998779296875
  AudioMixer4              filtermixer2;   //xy=2655.678291320801,750.9404258728027
  AudioMixer4              filtermixer3;   //xy=2655.749797821045,841.0833053588867
  AudioEffectFreeverb      freeverb1;      //xy=2794.749801635742,599.3333005905151
  AudioEffectFreeverb      freeverb7;      //xy=2803.749801635742,1109.0833072662354
  AudioEffectFreeverb      freeverb2;      //xy=2805.5712356567383,738.5832767486572
  AudioEffectFreeverb      freeverb8;      //xy=2809.499801635742,1194.333309173584
  AudioMixer4              freeverbmixer1; //xy=3008.8331909179688,624.8332977294922
  AudioMixer4              freeverbmixer7; //xy=3014.083106994629,1127.5832977294922
  AudioMixer4              freeverbmixer8; //xy=3014.749855041504,1219.6667137145996
  AudioMixer4              freeverbmixer2; //xy=3022.166358947754,768.4166164398193
  AudioMixer4              mixer1;         //xy=3244.777618408203,874.0832862854004
  AudioMixer4              mixer2;         //xy=3244.9998741149902,1059.2498397827148
  AudioMixer4              mixer_end;      //xy=3372.9048080444336,1340.5714645385742
  AudioAmplifier           amp0;           //xy=3480.6350021362305,1086.4167108535767
  AudioEffectEnvelope      envelope0;      //xy=3485.0832443237305,1000.3333339691162

  AudioPlaySdWav           playSdWav1;     //xy=199,194

  AudioMixer4              mixerPlay;      //xy=3587.8886184692383,1342.2221393585205
  AudioMixer4              mixer0;         //xy=3712.1389083862305,1040.0000228881836
  AudioMixer4              mixer_stereoL;         //xy=3712.1389083862305,1040.0000228881836
  AudioMixer4              mixer_stereoR;         //xy=3712.1389083862305,1040.0000228881836
  AudioInputI2S            audioInput;           //xy=3743.3332443237305,1366.3333358764648
  AudioRecordQueue         queue1;         //xy=3750.8332901000977,1458.3333129882812
  AudioOutputI2S           i2s1;           //xy=3754.8889083862305,1254.4445419311523

/*. ----  */
  AudioConnection          patchCord1(SNchaosMix, 0, SNtone, 0);
  AudioConnection          patchCord2(SNchaosMix, 0, SNtone2, 0);
  AudioConnection          patchCord3(HHchaosMix, 0, HHtone, 0);
  AudioConnection          patchCord4(HHchaosMix, 0, HHtone2, 0);
  AudioConnection          patchCord5(BDpitchAmt, BDpitchEnv);
  AudioConnection          patchCord6(SNtone, 0, SNtoneMix, 0);
  AudioConnection          patchCord7(SNtone2, 0, SNchaosMix, 0);
  AudioConnection          patchCord8(SNtone2, 0, SNtoneMix, 1);
  
  
  AudioConnection          patchCord13(HHtone2, 0, HHchaosMix, 0);
  AudioConnection          patchCord14(HHtone2, 0, HHtoneMix, 1);
  AudioConnection          patchCord15(HHtone, 0, HHtoneMix, 0);
  AudioConnection          patchCord16(BDpitchEnv, 0, BDchaosMix, 0);
  AudioConnection          patchCord17(waveform13_1, 0, mixer_waveform13, 0);
  AudioConnection          patchCord18(waveform13_2, 0, mixer_waveform13, 1);


AudioConnection          patchCord17b(waveform11_1, 0, mixer_waveform11, 0);
  AudioConnection          patchCord18b(waveform11_2, 0, mixer_waveform11, 1);


  AudioConnection          patchCord23(waveform14_1, 0, mixer_waveform14, 0);
  AudioConnection          patchCord24(waveform14_2, 0, mixer_waveform14, 1);
  AudioConnection          patchCord25(SNtoneMix, SNtoneEnv);
  AudioConnection          patchCord26(HHtoneMix, HHtoneEnv);
  AudioConnection          patchCord27(BDchaosMix, 0, BDsine, 0);
  AudioConnection          patchCord28(BDchaosMix, 0, BDsaw, 0);
  AudioConnection          patchCord29(BDchaosMix, 0, BDsaw, 1);
  
  AudioConnection          patchCord33(SNtoneEnv, 0, SNMix, 1);


  
  AudioConnection          patchCord38(SNnoise, 0, SNMix, 0);

  AudioConnection          patchCord44(HHtoneEnv, 0, HHMix, 1);
  AudioConnection          patchCord45(HHnoise, 0, HHMix, 0);
  AudioConnection          patchCord46(BDsine, 0, BDMix, 0);
  AudioConnection          patchCord47(BDsaw, 0, BDMix, 1);
  AudioConnection          patchCord48(BDsaw, 0, BDchaosMix, 1);
  AudioConnection          patchCord49(SNMix, SNenv);
  
  AudioConnection          patchCord53(mixer_waveform11, bitcrusher11);
  AudioConnection          patchCord58(envelope11, 0, mixer_waveform11,2);
  AudioConnection          patchCord50(mixer_waveform12, envelope12);
  AudioConnection          patchCord51(mixer_waveform13, envelope13);
  AudioConnection          patchCord52(mixer_waveform14, envelope14);
  
  AudioConnection          patchCord54(HHMix, HHenv);
  AudioConnection          patchCord55(BDMix, BDenv);
  AudioConnection          patchCord56(SNenv, 0, SNfilt, 0);
  AudioConnection          patchCord57(HHenv, 0, HHfilt, 0);
  
  AudioConnection          patchCord59(envelope13, bitcrusher13);
  AudioConnection          patchCord60(BDenv, 0, BDfilter, 0);
  AudioConnection          patchCord61(envelope12, bitcrusher12);
  AudioConnection          patchCord62(envelope14, bitcrusher14);
  AudioConnection          patchCord63(SNfilt, 1, SnFilter, 0);
  AudioConnection          patchCord64(HhFilter, 0, HhFilterMix, 0);
  AudioConnection          patchCord65(HhFilter, 2, HhFilterMix, 1);
  AudioConnection          patchCord66(BDfilter, 0, BdFilterMix, 0);
  AudioConnection          patchCord67(BDfilter, 2, BdFilterMix, 1);
  AudioConnection          patchCord68(HHfilt, 1, HhFilter, 0);
  AudioConnection          patchCord69(bitcrusher11, amp11);

  AudioConnection          patchCord70(bitcrusher13, amp13);
  AudioConnection          patchCord71(bitcrusher12, amp12);
  AudioConnection          patchCord72(SnFilter, 0, SnFilterMix, 0);
  AudioConnection          patchCord73(SnFilter, 2, SnFilterMix, 1);
  AudioConnection          patchCord74(bitcrusher14, amp14);
  AudioConnection          patchCord75(HhFilterMix, 0, HHMixer, 1);
  AudioConnection          patchCord76(amp13, 0, filter13, 0);
  AudioConnection          patchCord77(amp11, 0, filter11, 0);
  

  AudioConnection          patchCord78(BdFilterMix, 0, BDMixer, 1);
  AudioConnection          patchCord79(amp12, 0, filter12, 0);
  AudioConnection          patchCord80(SnFilterMix, 0, SNMixer, 1);
  AudioConnection          patchCord81(amp14, 0, filter14, 0);
  AudioConnection          patchCord82(filter13, 0, filtermixer13, 0);
  AudioConnection          patchCord83(filter13, 1, filtermixer13, 1);
  AudioConnection          patchCord84(filter13, 2, filtermixer13, 2);
  AudioConnection          patchCord85(filter12, 0, filtermixer12, 0);
  AudioConnection          patchCord86(filter12, 1, filtermixer12, 1);
  AudioConnection          patchCord87(filter12, 2, filtermixer12, 2);
  AudioConnection          patchCord88(filter11, 0, filtermixer11, 0);
  AudioConnection          patchCord89(filter11, 1, filtermixer11, 1);
  AudioConnection          patchCord90(filter11, 2, filtermixer11, 2);
  AudioConnection          patchCord91(filter14, 0, filtermixer14, 0);
  AudioConnection          patchCord92(filter14, 1, filtermixer14, 1);
  AudioConnection          patchCord93(filter14, 2, filtermixer14, 2);
  AudioConnection          patchCord94(BDMixer, envelope1);
  AudioConnection          patchCord95(SNMixer, envelope2);
  AudioConnection          patchCord96(HHMixer, envelope3);
  //AudioConnection          patchCord97(filtermixer13, flange13);
  AudioConnection          patchCord98(filtermixer13, freeverb13);
  AudioConnection          patchCord99(filtermixer13, 0, synthmixer13, 3);
  //AudioConnection          patchCord100(filtermixer14, flange14);
  AudioConnection          patchCord101(filtermixer14, freeverb14);
  AudioConnection          patchCord102(filtermixer14, 0, synthmixer14, 3);
  //AudioConnection          patchCord103(filtermixer11, flange11);
  AudioConnection          patchCord104(filtermixer11, freeverb11);
  AudioConnection          patchCord105(filtermixer11, 0, synthmixer11, 3);
  //AudioConnection          patchCord106(filtermixer12, flange12);
  //AudioConnection          patchCord107(filtermixer12, freeverb12);
  AudioConnection          patchCord108(filtermixer12, 0, synthmixer12, 3);
  AudioConnection          patchCord109(filtermixer13, 0, synthmixer13, 0);
  AudioConnection          patchCord110(freeverb13, 0, synthmixer13, 1);
  AudioConnection          patchCord111(filtermixer12, 0, synthmixer12, 0);
  AudioConnection          patchCord112(filtermixer14, 0, synthmixer14, 0);
  AudioConnection          patchCord113(freeverb12, 0, synthmixer12, 1);
  AudioConnection          patchCord114(freeverb11, 0, synthmixer11, 1);
  AudioConnection          patchCord115(freeverb14, 0, synthmixer14, 1);
  AudioConnection          patchCord116(filtermixer11, 0, synthmixer11, 0);
  AudioConnection          patchCord117(envelope6, bitcrusher6);
  
  AudioConnection          patchCord118(envelope8, bitcrusher8);
  AudioConnection          patchCord119(envelope7, bitcrusher7);
  AudioConnection          patchCord120(envelope3, bitcrusher3);
  AudioConnection          patchCord121(envelope1, bitcrusher1);
  AudioConnection          patchCord122(envelope4, bitcrusher4);
  AudioConnection          patchCord123(envelope5, bitcrusher5);
  AudioConnection          patchCord124(envelope2, bitcrusher2);

  AudioConnection          patchCord125(bitcrusher6, amp6);
  AudioConnection          patchCord126(bitcrusher7, amp7);
  AudioConnection          patchCord127(bitcrusher8, amp8);
  AudioConnection          patchCord128(bitcrusher4, amp4);
  AudioConnection          patchCord129(bitcrusher5, amp5);
  AudioConnection          patchCord130(bitcrusher1, amp1);
  AudioConnection          patchCord131(bitcrusher2, amp2);
  AudioConnection          patchCord132(bitcrusher3, amp3);
  AudioConnection          patchCord133(synthmixer14, 0, mixersynth_end, 3);
  AudioConnection          patchCord134(synthmixer11, 0, mixersynth_end, 0);
  AudioConnection          patchCord135(synthmixer13, 0, mixersynth_end, 2);
  AudioConnection          patchCord136(synthmixer12, 0, mixersynth_end, 1);
  AudioConnection          patchCord137(amp8, 0, filter8, 0);
  AudioConnection          patchCord138(amp7, 0, filter7, 0);
  AudioConnection          patchCord139(amp6, 0, filter6, 0);
  AudioConnection          patchCord140(amp4, 0, filter4, 0);
  AudioConnection          patchCord141(amp3, 0, filter3, 0);
  AudioConnection          patchCord142(amp5, 0, filter5, 0);
  AudioConnection          patchCord143(amp2, 0, filter2, 0);
  AudioConnection          patchCord144(amp1, 0, filter1, 0);
  AudioConnection          patchCord145(mixersynth_end, 0, mixer_end, 2);
  AudioConnection          patchCord146(filter1, 0, filtermixer1, 0);
  AudioConnection          patchCord147(filter1, 1, filtermixer1, 1);
  AudioConnection          patchCord148(filter1, 2, filtermixer1, 2);
  AudioConnection          patchCord149(filter8, 0, filtermixer8, 0);
  AudioConnection          patchCord150(filter8, 1, filtermixer8, 1);
  AudioConnection          patchCord151(filter8, 2, filtermixer8, 2);
  AudioConnection          patchCord152(filter7, 0, filtermixer7, 0);
  AudioConnection          patchCord153(filter7, 1, filtermixer7, 1);
  AudioConnection          patchCord154(filter7, 2, filtermixer7, 2);
  AudioConnection          patchCord155(filter6, 0, filtermixer6, 0);
  AudioConnection          patchCord156(filter6, 1, filtermixer6, 1);
  AudioConnection          patchCord157(filter6, 2, filtermixer6, 2);
  AudioConnection          patchCord158(filter4, 0, filtermixer4, 0);
  AudioConnection          patchCord159(filter4, 1, filtermixer4, 1);
  AudioConnection          patchCord160(filter4, 2, filtermixer4, 2);
  AudioConnection          patchCord161(filter2, 0, filtermixer2, 0);
  AudioConnection          patchCord162(filter2, 1, filtermixer2, 1);
  AudioConnection          patchCord163(filter2, 2, filtermixer2, 2);
  AudioConnection          patchCord164(filter3, 0, filtermixer3, 0);
  AudioConnection          patchCord165(filter3, 1, filtermixer3, 1);
  AudioConnection          patchCord166(filter3, 2, filtermixer3, 2);
  AudioConnection          patchCord167(filter5, 0, filtermixer5, 0);
  AudioConnection          patchCord168(filter5, 1, filtermixer5, 1);
  AudioConnection          patchCord169(filter5, 2, filtermixer5, 2);
  AudioConnection          patchCord170(filtermixer7, freeverb7);
  AudioConnection          patchCord171(filtermixer7, 0, freeverbmixer7, 3);
  AudioConnection          patchCord172(filtermixer1, freeverb1);
  AudioConnection          patchCord173(filtermixer1, 0, freeverbmixer1, 3);
  AudioConnection          patchCord174(filtermixer8, freeverb8);
  AudioConnection          patchCord175(filtermixer8, 0, freeverbmixer8, 3);
  AudioConnection          patchCord176(filtermixer4, 0, mixer1, 3);
  AudioConnection          patchCord177(filtermixer6, 0, mixer2, 1);
  AudioConnection          patchCord178(filtermixer5, 0, mixer2, 0);
  AudioConnection          patchCord179(filtermixer2, freeverb2);
  AudioConnection          patchCord180(filtermixer2, 0, freeverbmixer2, 3);
  AudioConnection          patchCord181(filtermixer3, 0, mixer1, 2);
  AudioConnection          patchCord182(freeverb1, 0, freeverbmixer1, 0);
  AudioConnection          patchCord183(freeverb7, 0, freeverbmixer7, 0);
  AudioConnection          patchCord184(freeverb2, 0, freeverbmixer2, 0);
  AudioConnection          patchCord185(freeverb8, 0, freeverbmixer8, 0);
  AudioConnection          patchCord186(freeverbmixer1, 0, mixer1, 0);
  AudioConnection          patchCord187(freeverbmixer7, 0, mixer2, 2);
  AudioConnection          patchCord188(freeverbmixer8, 0, mixer2, 3);
  AudioConnection          patchCord189(freeverbmixer2, 0, mixer1, 1);
  AudioConnection          patchCord190(mixer1, 0, mixer_end, 0);
  AudioConnection          patchCord191(mixer2, 0, mixer_end, 1);
  AudioConnection          patchCord192(mixer_end, 0, mixerPlay, 0);
  AudioConnection          patchCord193(mixer_end, 0, mixerPlay, 1);
  //AudioConnection          patchCord194(mixer0, mixerPlay, 2);

  AudioConnection          patchCord195(envelope0, 0, mixer0, 0);
  AudioConnection          patchCord196(playSdWav1, 0, mixer0, 1);


    AudioConnection          patchCord198l1(mixerPlay, 0, mixer_stereoL, 0);
    AudioConnection        patchCord198l2(mixer0, 0, mixer_stereoL, 1);

    AudioConnection          patchCord198r1(mixerPlay, 0, mixer_stereoR, 0);
    AudioConnection        patchCord198r2(mixer0, 0, mixer_stereoR, 1);

  AudioConnection          patchCord198s(mixer_stereoR, 0, i2s1, 1);
  AudioConnection          patchCord199s(mixer_stereoL, 0, i2s1, 0);


  
  AudioControlSGTL5000     sgtl5000_1;     //xy=3755.3332443237305,1413.3333358764648
  // GUItool: end automatically generated code



  EXTMEM AudioSynthWaveform       Swaveform1[3];
  EXTMEM AudioSynthWaveform       Swaveform2[3];
  EXTMEM AudioSynthWaveform       Swaveform3[3];
  EXTMEM AudioSynthWaveformDc     Sdc1[3];
  EXTMEM  AudioEffectEnvelope     SenvelopeFilter1[3];
  EXTMEM AudioMixer4              Smixer2[3];
  EXTMEM AudioMixer4              Smixer1[3];
  EXTMEM AudioFilterLadder        Sladder2[3];
  EXTMEM AudioFilterLadder        Sladder1[3];
  EXTMEM AudioEffectEnvelope      Senvelope1[3];
  EXTMEM AudioEffectEnvelope      Senvelope2[3];
  EXTMEM AudioMixer4              SmixerL4;
  EXTMEM AudioMixer4              SmixerR4;
  EXTMEM AudioMixer4              CHMixer11;


  AudioConnection          SpatchCord1(Swaveform1[0], 0, Smixer1[0], 0);
  AudioConnection          SpatchCord2(Swaveform1[0], 0, Smixer2[0], 0);
  AudioConnection          SpatchCord3(Swaveform2[0], 0, Smixer1[0], 1);
  AudioConnection          SpatchCord4(Swaveform2[0], 0, Smixer2[0], 1);
  AudioConnection          SpatchCord5(Swaveform3[0], 0, Smixer1[0], 2);
  AudioConnection          SpatchCord6(Swaveform3[0], 0, Smixer2[0], 2);
  AudioConnection          SpatchCord7(Sdc1[0], SenvelopeFilter1[0]);
  AudioConnection          SpatchCord8(SenvelopeFilter1[0], 0, Sladder1[0], 1);
  AudioConnection          SpatchCord9(SenvelopeFilter1[0], 0, Sladder2[0], 1);
  AudioConnection          SpatchCord10(Smixer2[0], 0, Sladder2[0], 0);
  AudioConnection          SpatchCord11(Smixer1[0], 0, Sladder1[0], 0);
  AudioConnection          SpatchCord12(Sladder2[0], Senvelope2[0]);
  AudioConnection          SpatchCord13(Sladder1[0], Senvelope1[0]);
  AudioConnection          SpatchCord14(Senvelope1[0], 0, SmixerL4, 0);
  AudioConnection          SpatchCord15(Senvelope2[0], 0, SmixerR4, 0);

  AudioConnection          SpatchCord16(Swaveform1[1], 0, Smixer1[1], 0);
  AudioConnection          SpatchCord17(Swaveform1[1], 0, Smixer2[1], 0);
  AudioConnection          SpatchCord18(Swaveform2[1], 0, Smixer1[1], 1);
  AudioConnection          SpatchCord19(Swaveform2[1], 0, Smixer2[1], 1);
  AudioConnection          SpatchCord20(Swaveform3[1], 0, Smixer1[1], 2);
  AudioConnection          SpatchCord21(Swaveform3[1], 0, Smixer2[1], 2);
  AudioConnection          SpatchCord22(Sdc1[1], SenvelopeFilter1[1]);
  AudioConnection          SpatchCord23(SenvelopeFilter1[1], 0, Sladder1[1], 1);
  AudioConnection          SpatchCord24(SenvelopeFilter1[1], 0, Sladder2[1], 1);
  AudioConnection          SpatchCord25(Smixer2[1], 0, Sladder2[1], 0);
  AudioConnection          SpatchCord26(Smixer1[1], 0, Sladder1[1], 0);
  AudioConnection          SpatchCord27(Sladder2[1], Senvelope2[1]);
  AudioConnection          SpatchCord28(Sladder1[1], Senvelope1[1]);
  AudioConnection          SpatchCord29(Senvelope1[1], 0, SmixerL4, 1);
  AudioConnection          SpatchCord30(Senvelope2[1], 0, SmixerR4, 1);

  AudioConnection          SpatchCord31(Swaveform1[2], 0, Smixer1[2], 0);
  AudioConnection          SpatchCord32(Swaveform1[2], 0, Smixer2[2], 0);
  AudioConnection          SpatchCord33(Swaveform2[2], 0, Smixer1[2], 1);
  AudioConnection          SpatchCord34(Swaveform2[2], 0, Smixer2[2], 1);
  AudioConnection          SpatchCord35(Swaveform3[2], 0, Smixer1[2], 2);
  AudioConnection          SpatchCord36(Swaveform3[2], 0, Smixer2[2], 2);
  AudioConnection          SpatchCord37(Sdc1[2], SenvelopeFilter1[2]);
  AudioConnection          SpatchCord38(SenvelopeFilter1[2], 0, Sladder1[2], 1);
  AudioConnection          SpatchCord39(SenvelopeFilter1[2], 0, Sladder2[2], 1);
  AudioConnection          SpatchCord40(Smixer2[2], 0, Sladder2[2], 0);
  AudioConnection          SpatchCord41(Smixer1[2], 0, Sladder1[2], 0);
  AudioConnection          SpatchCord42(Sladder2[2], Senvelope2[2]);
  AudioConnection          SpatchCord43(Sladder1[2], Senvelope1[2]);
  AudioConnection          SpatchCord44(Senvelope1[2], 0, SmixerL4, 2);
  AudioConnection          SpatchCord45(Senvelope2[2], 0, SmixerR4, 2);

  AudioConnection          SpatchCord21214(SmixerL4, 0, CHMixer11, 0);
  AudioConnection          SpatchCord21224(SmixerR4, 0, CHMixer11, 1);
  AudioConnection          SpatchCord21234(CHMixer11, 0, mixer_waveform11, 3); //<<--- ???


// --- ADDITIONAL sound declarations below here ... ---//
// --- EXTMEM AudiowlayArrayResmp doesnt exist in GUItool ---//


AudioConnection          patchCord2121(playSdWav1, 0, peak1, 0);
AudioConnection          patchCord2122(audioInput, 0, queue1, 0);
AudioConnection          patchCord200(audioInput, 0, peakRec, 0);


 AudioConnection snd0(sound0, envelope0);
 AudioConnection snd1(sound1, 0, BDMixer,0);
 AudioConnection snd3(sound2, 0, SNMixer,0);
 AudioConnection snd2(sound3, 0, HHMixer,0);
 AudioConnection snd4(sound4, envelope4);
 AudioConnection snd5(sound5, envelope5);
 AudioConnection snd6(sound6, envelope6);
 AudioConnection snd7(sound7, envelope7);
 AudioConnection snd8(sound8, envelope8);

