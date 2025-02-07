
// GUItool: begin automatically generated code
AudioSynthWaveform       sound13;        //xy=102.5555648803711,1034.8889617919922
AudioSynthWaveform       sound14;        //xy=108.11111068725586,1108.6666951179504
AudioPlaySdRaw           playRaw0;       //xy=274.5558891296387,139.66687393188477
AudioEffectEnvelope      envelope2;      //xy=275.5557060241699,251.6666965484619
AudioEffectEnvelope      envelope3;      //xy=275.5557556152344,319.66668248176575
AudioEffectEnvelope      envelope1;      //xy=276.5557670593262,194.4444661140442
AudioEffectEnvelope      envelope4;      //xy=277.55572509765625,387.6667137145996
AudioEffectEnvelope      envelope0;      //xy=281.5557327270508,84.22226333618164
AudioEffectEnvelope      envelope8;      //xy=281.5557556152344,652.6667709350586
AudioEffectEnvelope      envelope5;      //xy=283.55572509765625,457.66674518585205
AudioEffectEnvelope      envelope6;      //xy=283.55573654174805,522.6667156219482
AudioEffectEnvelope      envelope11;     //xy=282.55567932128906,874.6667137145996
AudioEffectEnvelope      envelope9;      //xy=283.55572509765625,723.666711807251
AudioEffectEnvelope      envelope7;      //xy=286.55572509765625,583.6668090820312
AudioEffectEnvelope      envelope10;     //xy=287.5556755065918,797.6667747497559
AudioEffectEnvelope      envelope14;     //xy=286.55567932128906,1117.6667804718018
AudioEffectEnvelope      envelope13;     //xy=287.55564880371094,1043.666654586792
AudioEffectEnvelope      envelope12;     //xy=289.55564880371094,961.6666541099548
AudioAmplifier           amp0;           //xy=434.8888702392578,458.3333406448364
AudioAmplifier           amp5;           //xy=436.8888702392578,458.3333406448364
AudioAmplifier           amp1;           //xy=439.88893127441406,194.3333501815796
AudioAmplifier           amp4;           //xy=439.8888702392578,387.333309173584
AudioAmplifier           amp6;           //xy=439.88890075683594,523.3333415985107
AudioAmplifier           amp2;           //xy=443.88890075683594,250.33330535888672
AudioAmplifier           amp3;           //xy=443.8888740539551,316.33330726623535
AudioAmplifier           amp7;           //xy=442.8888702392578,582.3334350585938
AudioAmplifier           amp14;          //xy=442.8888702392578,1116.3335285186768
AudioAmplifier           amp9;           //xy=444.88890075683594,726.3333978652954
AudioAmplifier           amp11;          //xy=444.88890075683594,876.3334007263184
AudioAmplifier           amp8;           //xy=447.88890075683594,650.3333740234375
AudioAmplifier           amp10;          //xy=449.8888702392578,798.333399772644
AudioAmplifier           amp12;          //xy=451.88890075683594,961.3334646224976
AudioAmplifier           amp13;          //xy=453.88890075683594,1041.3334045410156
AudioFilterStateVariable filter5;        //xy=910.4444885253906,506.4444818496704
AudioFilterStateVariable filter1;        //xy=912.0000114440918,284.3333806991577
AudioFilterStateVariable filter0;        //xy=913.5556526184082,192.9444465637207
AudioFilterStateVariable filter2;        //xy=913.7777709960938,341.88897705078125
AudioFilterStateVariable filter6;        //xy=915.4444885253906,562.4444832801819
AudioFilterStateVariable filter7;        //xy=915.4444885253906,616.4444842338562
AudioFilterStateVariable filter9;        //xy=915.4444885253906,728.4444561004639
AudioFilterStateVariable filter3;        //xy=917.4444427490234,403.44449615478516
AudioFilterStateVariable filter8;        //xy=916.4444885253906,677.4444856643677
AudioFilterStateVariable filter4;        //xy=917.4444427490234,453.44449615478516
AudioFilterStateVariable filter11;       //xy=916.4444885253906,840.4444580078125
AudioFilterStateVariable filter10;       //xy=918.4444274902344,786.4444580078125
AudioFilterStateVariable filter12;       //xy=922.444486618042,894.4446067810059
AudioFilterStateVariable filter14;       //xy=927.0000686645508,1051.1112747192383
AudioFilterStateVariable filter13;       //xy=929.4444236755371,981.4445419311523
AudioMixer4              mixer3;         //xy=1227.1112747192383,778
AudioMixer4              mixer0;         //xy=1230.7778091430664,108.4444408416748
AudioMixer4              mixer1;         //xy=1231.111385345459,370.33335876464844
AudioMixer4              mixer2;         //xy=1235.5559120178223,579.2223196029663
AudioMixer4              mixer4;         //xy=1253.222469329834,1008.3333473205566
AudioMixer4              mixer_end;      //xy=1512.555519104004,679.5555572509766
AudioMixer4              mixerPlay;         //xy=1754.3334884643555,689.8888740539551
AudioInputI2S            i2s2;           //xy=1855.111442565918,817.2222881317139
AudioOutputI2S           i2s1;           //xy=2043.5557556152344,694.5558443069458
AudioRecordQueue         queue1;         //xy=2051.444715499878,816.0619468688965
AudioConnection          patchCord1(sound13, envelope13);
AudioConnection          patchCord2(sound14, envelope14);

AudioConnection          patchCord5(playRaw0, 0, mixer0, 1);
AudioConnection          patchCord6(playRaw0, 0, mixer0, 2);
AudioConnection          patchCord7(envelope2, amp2);
AudioConnection          patchCord8(envelope3, amp3);
AudioConnection          patchCord9(envelope1, amp1);
AudioConnection          patchCord10(envelope4, amp4);
AudioConnection          patchCord11(envelope0, 0, mixer0, 0);
AudioConnection          patchCord12(envelope8, amp8);
AudioConnection          patchCord13(envelope5, amp5);
AudioConnection          patchCord14(envelope6, amp6);
AudioConnection          patchCord15(envelope11, amp11);
AudioConnection          patchCord16(envelope9, amp9);
AudioConnection          patchCord17(envelope7, amp7);
AudioConnection          patchCord18(envelope10, amp10);
AudioConnection          patchCord19(envelope14, amp14);
AudioConnection          patchCord20(envelope13, amp13);
AudioConnection          patchCord21(envelope12, amp12);
AudioConnection          patchCord22(amp5, 0, filter5, 0);
AudioConnection          patchCord23(amp1, 0, filter1, 0);
AudioConnection          patchCord24(amp4, 0, filter4, 0);
AudioConnection          patchCord25(amp6, 0, filter6, 0);
AudioConnection          patchCord26(amp2, 0, filter2, 0);
AudioConnection          patchCord27(amp3, 0, filter3, 0);
AudioConnection          patchCord28(amp7, 0, filter7, 0);
AudioConnection          patchCord29(amp14, 0, filter14, 0);
AudioConnection          patchCord30(amp9, 0, filter9, 0);
AudioConnection          patchCord31(amp11, 0, filter11, 0);
AudioConnection          patchCord32(amp8, 0, filter8, 0);
AudioConnection          patchCord33(amp10, 0, filter10, 0);
AudioConnection          patchCord34(amp12, 0, filter12, 0);
AudioConnection          patchCord35(amp13, 0, filter13, 0);
AudioConnection          patchCord36(filter5, 0, mixer2, 0);
AudioConnection          patchCord37(filter1, 0, mixer1, 0);
AudioConnection          patchCord38(filter2, 0, mixer1, 1);
AudioConnection          patchCord39(filter6, 0, mixer2, 1);
AudioConnection          patchCord40(filter7, 0, mixer2, 2);
AudioConnection          patchCord41(filter9, 0, mixer3, 0);
AudioConnection          patchCord42(filter3, 0, mixer1, 2);
AudioConnection          patchCord43(filter8, 0, mixer2, 3);
AudioConnection          patchCord44(filter4, 0, mixer1, 3);
AudioConnection          patchCord45(filter11, 0, mixer3, 2);
AudioConnection          patchCord46(filter10, 0, mixer3, 1);
AudioConnection          patchCord47(filter12, 0, mixer3, 3);
AudioConnection          patchCord48(filter14, 0, mixer4, 1);
AudioConnection          patchCord49(filter13, 0, mixer4, 0);
AudioConnection          patchCord50(mixer3, 0, mixer_end, 2);
AudioConnection          patchCord51(mixer0, 0, mixerPlay, 2);
AudioConnection          patchCord52(mixer1, 0, mixer_end, 0);
AudioConnection          patchCord53(mixer2, 0, mixer_end, 1);
AudioConnection          patchCord54(mixer4, 0, mixer_end, 3);
AudioConnection          patchCord55(mixer_end, 0, mixerPlay, 0);
AudioConnection          patchCord56(mixer_end, 0, mixerPlay, 1);
AudioConnection          patchCord57(mixerPlay, 0, i2s1, 0);
AudioConnection          patchCord58(mixerPlay, 0, i2s1, 1);
AudioConnection          patchCord59(i2s2, 0, queue1, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=1867.2224884033203,864.6668453216553
// GUItool: end automatically generated code


// --- ADDITIONAL sound declarations below here ... ---//
// --- AudioPlayArrayResmp doesnt exist in GUItool ---//

AudioPlayArrayResmp sound0;
AudioPlayArrayResmp sound1;
AudioPlayArrayResmp sound2;
AudioPlayArrayResmp sound3;
AudioPlayArrayResmp sound4;
AudioPlayArrayResmp sound5;
AudioPlayArrayResmp sound6;
AudioPlayArrayResmp sound7;
AudioPlayArrayResmp sound8;
AudioPlayArrayResmp sound9;
AudioPlayArrayResmp sound10;
AudioPlayArrayResmp sound11;
AudioPlayArrayResmp sound12;


AudioConnection snd0(sound0, envelope0);
AudioConnection snd1(sound1, envelope1);
AudioConnection snd2(sound2, envelope2);
AudioConnection snd3(sound3, envelope3);
AudioConnection snd4(sound4, envelope4);
AudioConnection snd5(sound5, envelope5);
AudioConnection snd6(sound6, envelope6);
AudioConnection snd7(sound7, envelope7);
AudioConnection snd8(sound8, envelope8);
AudioConnection snd9(sound9, envelope9);
AudioConnection snd10(sound10, envelope10);
AudioConnection snd11(sound11, envelope11);
AudioConnection snd12(sound12, envelope12);
