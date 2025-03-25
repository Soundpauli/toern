
AudioPlayArrayResmp sound0;
AudioPlayArrayResmp sound1;
AudioPlayArrayResmp sound2;
AudioPlayArrayResmp sound3;
AudioPlayArrayResmp sound4;
AudioPlayArrayResmp sound5;
AudioPlayArrayResmp sound6;
AudioPlayArrayResmp sound7;
AudioPlayArrayResmp sound8;


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformDc     BDpitchAmt;     //xy=114.80554962158203,667.6388735771179
AudioMixer4              SNchaosMix;     //xy=133.00000762939453,893.3333435058594
AudioMixer4              HHchaosMix;     //xy=139.6666717529297,782.7221832275391
AudioEffectEnvelope      BDpitchEnv;     //xy=280.05555725097656,666.7778587341309
AudioSynthWaveformModulated SNtone;         //xy=291.3333282470703,875.0000610351562
AudioSynthWaveformModulated SNtone2;        //xy=291.47222900390625,925.0555419921875
AudioSynthWaveformModulated HHtone2;        //xy=296.77781677246094,813.6111450195312
AudioSynthWaveformModulated HHtone;         //xy=301.0000305175781,764.5
AudioSynthNoisePink      pink1;          //xy=411.0477066040039,1592.5715374946594
AudioSynthWaveform       waveform13_1;   //xy=451.3334426879883,1699.8573150634766
AudioSynthWaveform       waveform13_2;   //xy=452.3334426879883,1737.8573150634766
AudioSynthWaveform       waveform11_1;   //xy=460.761962890625,1325.1429443359375
AudioSynthWaveform       waveform11_2;   //xy=460.761962890625,1371.1429443359375
AudioMixer4              HHtoneMix;      //xy=463.88892364501953,800.6944274902344
AudioSynthWaveform       waveform12_1;   //xy=461.761962890625,1420.1429443359375
AudioSynthWaveform       waveform12_2;   //xy=461.761962890625,1458.1429443359375
AudioMixer4              BDchaosMix;     //xy=467.5277862548828,684.3333625793457
AudioMixer4              SNtoneMix;      //xy=469.8611526489258,899.4166393280029
AudioSynthWaveform       waveform14_1;   //xy=471.76201248168945,1886.571632385254
AudioSynthWaveform       waveform14_2;   //xy=471.76201248168945,1921.571632385254
AudioEffectEnvelope      SNtoneEnv;      //xy=626.0000076293945,927.7500133514404
AudioSynthWaveformModulated BDsine;         //xy=635.1666526794434,668.9166522026062
AudioEffectEnvelope      HHtoneEnv;      //xy=635.0000076293945,817.5000123977661
AudioSynthNoiseWhite     SNnoise;        //xy=635.0000076293945,883.5000123977661
AudioSynthWaveformModulated BDsaw;          //xy=638.8055191040039,712.2499809265137
AudioSynthNoiseWhite     HHnoise;        //xy=638.7500076293945,779.7500114440918
AudioMixer4              mixer_waveform12; //xy=678.0476951599121,1575.571548461914
AudioMixer4              mixer_waveform11; //xy=685.761962890625,1408.1429443359375
AudioMixer4              mixer_waveform13; //xy=685.3334426879883,1722.5716514587402
AudioSynthKarplusStrong  string13;       //xy=699.1905632019043,1775.4287405014038
AudioSynthSimpleDrum     drum13;         //xy=701.4762687683105,1663.5716171264648
AudioSynthKarplusStrong  string12;       //xy=703.4762687683105,1630.0001363754272
AudioMixer4              mixer_waveform14; //xy=702.7620124816895,1892.571632385254
AudioSynthKarplusStrong  string11;       //xy=712.6190662384033,1471.0001220703125
AudioSynthSimpleDrum     drum12;         //xy=713.6190757751465,1521.571584701538
AudioSynthSimpleDrum     drum11;         //xy=714.4762573242188,1347.8572998046875
AudioSynthKarplusStrong  string14;       //xy=716.4762725830078,1949.000129699707
AudioSynthSimpleDrum     drum14;         //xy=723.1905670166016,1838.857292175293
AudioMixer4              BDMix;          //xy=801.1666374206543,697.1666946411133
AudioMixer4              HHMix;          //xy=802.5000114440918,806.6428337097168
AudioMixer4              SNMix;          //xy=804.8214874267578,903.3571319580078
AudioMixer4              waveformmixer12; //xy=884.0476951599121,1578.571548461914
AudioMixer4              waveformmixer13; //xy=899.3334274291992,1724.1430625915527
AudioMixer4              waveformmixer14; //xy=908.7620124816895,1896.571632385254
AudioMixer4              waveformmixer11; //xy=911.761962890625,1406.1429443359375
AudioMixer4              BDMixer1;       //xy=962.0833511352539,694.8333396911621
AudioMixer4              SNMixer3;       //xy=962.7500152587891,895.5000133514404
AudioMixer4              HHMixer2;       //xy=965.0000152587891,803.2500114440918
AudioEffectEnvelope      envelope6;      //xy=1117.250015258789,1119.7500162124634
AudioEffectEnvelope      envelope8;      //xy=1116.9166717529297,1280.5833435058594
AudioEffectEnvelope      envelope11;     //xy=1117.047519683838,1404.8930644989014
AudioEffectEnvelope      envelope7;      //xy=1119.000015258789,1203.250018119812
AudioEffectEnvelope      envelope3;      //xy=1120.500015258789,895.5000133514404
AudioEffectEnvelope      envelope1;      //xy=1122,694
AudioEffectEnvelope      envelope4;      //xy=1121.250015258789,972.2500152587891
AudioEffectEnvelope      envelope5;      //xy=1121.000015258789,1042.250015258789
AudioEffectEnvelope      envelope2;      //xy=1123.000015258789,803.750011920929
AudioEffectEnvelope      envelope13;     //xy=1119.904899597168,1723.7143659591675
AudioEffectEnvelope      envelope12;     //xy=1125.119052886963,1578.261806488037
AudioEffectEnvelope      envelope14;     //xy=1126.9999961853027,1895.095314025879
AudioEffectBitcrusher    bitcrusher6;    //xy=1287.0278511047363,1120.0833902359009
AudioEffectBitcrusher    bitcrusher7;    //xy=1287.5278549194336,1203.5833911895752
AudioEffectBitcrusher    bitcrusher8;    //xy=1288.5278434753418,1279.7500095367432
AudioEffectBitcrusher    bitcrusher4;    //xy=1292.2778549194336,971.3333892822266
AudioEffectBitcrusher    bitcrusher5;    //xy=1292.2778511047363,1042.3333892822266
AudioEffectBitcrusher    bitcrusher1;    //xy=1294.2778511047363,694.0833849906921
AudioEffectBitcrusher    bitcrusher2;    //xy=1294.2778511047363,804.0833854675293
AudioEffectBitcrusher    bitcrusher11;   //xy=1293.2856407165527,1405.7141914367676
AudioEffectBitcrusher    bitcrusher3;    //xy=1297.2778511047363,896.5833873748779
AudioEffectBitcrusher    bitcrusher13;   //xy=1295.90482711792,1723.3809914588928
AudioEffectBitcrusher    bitcrusher12;   //xy=1305.6192207336426,1578.9285535812378
AudioEffectBitcrusher    bitcrusher14;   //xy=1315.333209991455,1893.4286847114563
AudioAmplifier           amp13;          //xy=1453.2620124816895,1723.6310739517212
AudioAmplifier           amp8;           //xy=1462.8847885131836,1279.5001411437988
AudioAmplifier           amp7;           //xy=1463.5514125823975,1201.6945972442627
AudioAmplifier           amp6;           //xy=1466.5237045288086,1121.1668872833252
AudioAmplifier           amp11;          //xy=1465.7502098083496,1406.2141513824463
AudioAmplifier           amp4;           //xy=1467.6347885131836,972.3889923095703
AudioAmplifier           amp3;           //xy=1468.162498474121,895.6945934295654
AudioAmplifier           amp5;           //xy=1467.968162536621,1042.833511352539
AudioAmplifier           amp2;           //xy=1469.1665267944336,801.6668825149536
AudioAmplifier           amp12;          //xy=1469.476203918457,1576.7618465423584
AudioAmplifier           amp1;           //xy=1473.3293685913086,693.277904510498
AudioAmplifier           amp14;          //xy=1483.1904373168945,1894.4286451339722
AudioFilterStateVariable filter13;       //xy=1606.5715065002441,1727.0476989746094
AudioFilterStateVariable filter12;       //xy=1620.702449798584,1584.6786155700684
AudioFilterStateVariable filter1;        //xy=1627.0000228881836,699.5000114440918
AudioFilterStateVariable filter8;        //xy=1625.7500228881836,1285.2500534057617
AudioFilterStateVariable filter11;       //xy=1625.9166831970215,1413.7500228881836
AudioFilterStateVariable filter2;        //xy=1628.7500228881836,808.7500114440918
AudioFilterStateVariable filter7;        //xy=1628.1666107177734,1207.0000495910645
AudioFilterStateVariable filter6;        //xy=1628.4999465942383,1128.4166297912598
AudioFilterStateVariable filter4;        //xy=1630.2500228881836,979.0000152587891
AudioFilterStateVariable filter3;        //xy=1631.7500247955322,901.7500133514404
AudioFilterStateVariable filter5;        //xy=1631.6666107177734,1053.5
AudioFilterStateVariable filter14;       //xy=1628.3334274291992,1901.4287252426147
AudioMixer4              filtermixer13;  //xy=1780.5715103149414,1733.0477809906006
AudioMixer4              filtermixer14;  //xy=1790.3333549499512,1907.4286851882935
AudioMixer4              filtermixer11;  //xy=1794.8809204101562,1420.4284896850586
AudioMixer4              filtermixer7;   //xy=1795.7500305175781,1212.0000171661377
AudioMixer4              filtermixer1;   //xy=1798.7500267028809,706.2500114440918
AudioMixer4              filtermixer8;   //xy=1797.1665725708008,1289.4167175292969
AudioMixer4              filtermixer12;  //xy=1796.3691596984863,1591.095220565796
AudioMixer4              filtermixer4;   //xy=1802.7500267028809,985.2500143051147
AudioMixer4              filtermixer6;   //xy=1802.4166145324707,1136.3333778381348
AudioMixer4              filtermixer2;   //xy=1804.2500267028809,814.7500114440918
AudioMixer4              filtermixer5;   //xy=1803.6665344238281,1061.1665878295898
AudioMixer4              filtermixer3;   //xy=1805.7500267028809,907.7500152587891
AudioEffectFreeverb      freeverb1;      //xy=1944.7500305175781,666.0000104904175
AudioEffectFlange        flange13;       //xy=1942.1905975341797,1703.8096947669983
AudioEffectFreeverb      freeverb13;     //xy=1942.4763107299805,1770.0953216552734
AudioEffectFreeverb      freeverb7;      //xy=1953.7500305175781,1175.7500171661377
AudioEffectFlange        flange12;       //xy=1953.5238914489746,1554.1428842544556
AudioEffectFreeverb      freeverb2;      //xy=1957.0000305175781,805.2500114440918
AudioEffectFlange        flange14;       //xy=1954.047519683838,1877.142855644226
AudioEffectFreeverb      freeverb12;     //xy=1956.3810539245605,1624.4286155700684
AudioEffectFreeverb      freeverb8;      //xy=1959.5000305175781,1261.0000190734863
AudioEffectFreeverb      freeverb11;     //xy=1959.3810005187988,1461.4285488128662
AudioEffectFreeverb      freeverb14;     //xy=1958.3334312438965,1959.0001192092896
AudioEffectFlange        flange11;       //xy=1962.6667137145996,1392.5714235305786
AudioMixer4              freeverbmixer1; //xy=2158.8334197998047,691.5000076293945
AudioMixer4              synthmixer4;   //xy=2158.809482574463,1931.1903266906738
AudioMixer4              freeverbmixer7; //xy=2164.083335876465,1194.2500076293945
AudioMixer4              freeverbmixer8; //xy=2164.75008392334,1286.333423614502
AudioMixer4              synthmixer1;    //xy=2164.618827819824,1437.9999752044678
AudioMixer4              synthmixer3;    //xy=2163.666660308838,1734.2382926940918
AudioMixer4              freeverbmixer2; //xy=2172.16658782959,835.0833263397217
AudioMixer4              synthmixer2;    //xy=2170.476177215576,1589.9999752044678
AudioMixer4              mixer1;         //xy=2394.777847290039,940.7499961853027
AudioMixer4              mixer2;         //xy=2395.000102996826,1125.9165496826172
AudioAmplifier           amp0;           //xy=2404.3017959594727,569.083384513855
AudioEffectEnvelope      envelope0;      //xy=2408.7500381469727,483.00000762939453
AudioPlaySdRaw           playRaw0;       //xy=2409.0002822875977,525.4166948795319
AudioMixer4              mixersynth_end; //xy=2409.190528869629,1621.1427688598633
AudioMixer4              mixer_end;      //xy=2604.571563720703,1347.2381286621094
AudioMixer4              mixer0;         //xy=2635.8057022094727,522.6666965484619
AudioMixer4              mixerPlay;      //xy=2819.555374145508,1348.8888034820557
AudioInputI2S            is23;           //xy=2975,1373
AudioRecordQueue         queue1;         //xy=2982.500045776367,1464.9999771118164
AudioOutputI2S           i2s1;           //xy=3138.555652618408,1261.1111469268799
AudioConnection          patchCord1(BDpitchAmt, BDpitchEnv);
AudioConnection          patchCord2(SNchaosMix, 0, SNtone, 0);
AudioConnection          patchCord3(SNchaosMix, 0, SNtone2, 0);
AudioConnection          patchCord4(HHchaosMix, 0, HHtone, 0);
AudioConnection          patchCord5(HHchaosMix, 0, HHtone2, 0);
AudioConnection          patchCord6(BDpitchEnv, 0, BDchaosMix, 0);
AudioConnection          patchCord7(SNtone, 0, SNtoneMix, 0);
AudioConnection          patchCord8(SNtone2, 0, SNchaosMix, 0);
AudioConnection          patchCord9(SNtone2, 0, SNtoneMix, 1);
AudioConnection          patchCord10(HHtone2, 0, HHchaosMix, 0);
AudioConnection          patchCord11(HHtone2, 0, HHtoneMix, 1);
AudioConnection          patchCord12(HHtone, 0, HHtoneMix, 0);
AudioConnection          patchCord13(pink1, 0, mixer_waveform13, 2);
AudioConnection          patchCord14(pink1, 0, mixer_waveform14, 3);
AudioConnection          patchCord15(pink1, 0, mixer_waveform12, 2);
AudioConnection          patchCord16(pink1, 0, mixer_waveform11, 2);
AudioConnection          patchCord17(waveform13_1, 0, mixer_waveform13, 0);
AudioConnection          patchCord18(waveform13_2, 0, mixer_waveform13, 1);
AudioConnection          patchCord19(waveform11_1, 0, mixer_waveform11, 0);
AudioConnection          patchCord20(waveform11_2, 0, mixer_waveform11, 1);
AudioConnection          patchCord21(HHtoneMix, HHtoneEnv);
AudioConnection          patchCord22(waveform12_1, 0, mixer_waveform12, 0);
AudioConnection          patchCord23(waveform12_2, 0, mixer_waveform12, 1);
AudioConnection          patchCord24(BDchaosMix, 0, BDsine, 0);
AudioConnection          patchCord25(BDchaosMix, 0, BDsaw, 0);
AudioConnection          patchCord26(BDchaosMix, 0, BDsaw, 1);
AudioConnection          patchCord27(SNtoneMix, SNtoneEnv);
AudioConnection          patchCord28(waveform14_1, 0, mixer_waveform14, 0);
AudioConnection          patchCord29(waveform14_2, 0, mixer_waveform14, 1);
AudioConnection          patchCord30(SNtoneEnv, 0, SNMix, 1);
AudioConnection          patchCord31(BDsine, 0, BDMix, 0);
AudioConnection          patchCord32(HHtoneEnv, 0, HHMix, 1);
AudioConnection          patchCord33(SNnoise, 0, SNMix, 0);
AudioConnection          patchCord34(BDsaw, 0, BDMix, 1);
AudioConnection          patchCord35(BDsaw, 0, BDchaosMix, 1);
AudioConnection          patchCord36(HHnoise, 0, HHMix, 0);
AudioConnection          patchCord37(mixer_waveform12, 0, waveformmixer12, 1);
AudioConnection          patchCord38(mixer_waveform11, 0, waveformmixer11, 1);
AudioConnection          patchCord39(mixer_waveform13, 0, waveformmixer13, 1);
AudioConnection          patchCord40(string13, 0, waveformmixer13, 2);
AudioConnection          patchCord41(drum13, 0, waveformmixer13, 0);
AudioConnection          patchCord42(string12, 0, waveformmixer12, 2);
AudioConnection          patchCord43(mixer_waveform14, 0, waveformmixer14, 1);
AudioConnection          patchCord44(string11, 0, waveformmixer11, 2);
AudioConnection          patchCord45(drum12, 0, waveformmixer12, 0);
AudioConnection          patchCord46(drum11, 0, waveformmixer11, 0);
AudioConnection          patchCord47(string14, 0, waveformmixer14, 2);
AudioConnection          patchCord48(drum14, 0, waveformmixer14, 0);
AudioConnection          patchCord49(BDMix, 0, BDMixer1, 0);
AudioConnection          patchCord50(HHMix, 0, HHMixer2, 0);
AudioConnection          patchCord51(SNMix, 0, SNMixer3, 0);
AudioConnection          patchCord52(waveformmixer12, envelope12);
AudioConnection          patchCord53(waveformmixer13, envelope13);
AudioConnection          patchCord54(waveformmixer14, envelope14);
AudioConnection          patchCord55(waveformmixer11, envelope11);
AudioConnection          patchCord56(BDMixer1, envelope1);
AudioConnection          patchCord57(SNMixer3, envelope3);
AudioConnection          patchCord58(HHMixer2, envelope2);
AudioConnection          patchCord59(envelope6, bitcrusher6);
AudioConnection          patchCord60(envelope8, bitcrusher8);
AudioConnection          patchCord61(envelope11, bitcrusher11);
AudioConnection          patchCord62(envelope7, bitcrusher7);
AudioConnection          patchCord63(envelope3, bitcrusher3);
AudioConnection          patchCord64(envelope1, bitcrusher1);
AudioConnection          patchCord65(envelope4, bitcrusher4);
AudioConnection          patchCord66(envelope5, bitcrusher5);
AudioConnection          patchCord67(envelope2, bitcrusher2);
AudioConnection          patchCord68(envelope13, bitcrusher13);
AudioConnection          patchCord69(envelope12, bitcrusher12);
AudioConnection          patchCord70(envelope14, bitcrusher14);
AudioConnection          patchCord71(bitcrusher6, amp6);
AudioConnection          patchCord72(bitcrusher7, amp7);
AudioConnection          patchCord73(bitcrusher8, amp8);
AudioConnection          patchCord74(bitcrusher4, amp4);
AudioConnection          patchCord75(bitcrusher5, amp5);
AudioConnection          patchCord76(bitcrusher1, amp1);
AudioConnection          patchCord77(bitcrusher2, amp2);
AudioConnection          patchCord78(bitcrusher11, amp11);
AudioConnection          patchCord79(bitcrusher3, amp3);
AudioConnection          patchCord80(bitcrusher13, amp13);
AudioConnection          patchCord81(bitcrusher12, amp12);
AudioConnection          patchCord82(bitcrusher14, amp14);
AudioConnection          patchCord83(amp13, 0, filter13, 0);
AudioConnection          patchCord84(amp8, 0, filter8, 0);
AudioConnection          patchCord85(amp7, 0, filter7, 0);
AudioConnection          patchCord86(amp6, 0, filter6, 0);
AudioConnection          patchCord87(amp11, 0, filter11, 0);
AudioConnection          patchCord88(amp4, 0, filter4, 0);
AudioConnection          patchCord89(amp3, 0, filter3, 0);
AudioConnection          patchCord90(amp5, 0, filter5, 0);
AudioConnection          patchCord91(amp2, 0, filter2, 0);
AudioConnection          patchCord92(amp12, 0, filter12, 0);
AudioConnection          patchCord93(amp1, 0, filter1, 0);
AudioConnection          patchCord94(amp14, 0, filter14, 0);
AudioConnection          patchCord95(filter13, 0, filtermixer13, 0);
AudioConnection          patchCord96(filter13, 1, filtermixer13, 1);
AudioConnection          patchCord97(filter13, 2, filtermixer13, 2);
AudioConnection          patchCord98(filter12, 0, filtermixer12, 0);
AudioConnection          patchCord99(filter12, 1, filtermixer12, 1);
AudioConnection          patchCord100(filter12, 2, filtermixer12, 2);
AudioConnection          patchCord101(filter1, 0, filtermixer1, 0);
AudioConnection          patchCord102(filter1, 1, filtermixer1, 1);
AudioConnection          patchCord103(filter1, 2, filtermixer1, 2);
AudioConnection          patchCord104(filter8, 0, filtermixer8, 0);
AudioConnection          patchCord105(filter8, 1, filtermixer8, 1);
AudioConnection          patchCord106(filter8, 2, filtermixer8, 2);
AudioConnection          patchCord107(filter11, 0, filtermixer11, 0);
AudioConnection          patchCord108(filter11, 1, filtermixer11, 1);
AudioConnection          patchCord109(filter11, 2, filtermixer11, 2);
AudioConnection          patchCord110(filter2, 0, filtermixer2, 0);
AudioConnection          patchCord111(filter2, 1, filtermixer2, 1);
AudioConnection          patchCord112(filter2, 2, filtermixer2, 2);
AudioConnection          patchCord113(filter7, 0, filtermixer7, 0);
AudioConnection          patchCord114(filter7, 1, filtermixer7, 1);
AudioConnection          patchCord115(filter7, 2, filtermixer7, 2);
AudioConnection          patchCord116(filter6, 0, filtermixer6, 0);
AudioConnection          patchCord117(filter6, 1, filtermixer6, 1);
AudioConnection          patchCord118(filter6, 2, filtermixer6, 2);
AudioConnection          patchCord119(filter4, 0, filtermixer4, 0);
AudioConnection          patchCord120(filter4, 1, filtermixer4, 1);
AudioConnection          patchCord121(filter4, 2, filtermixer4, 2);
AudioConnection          patchCord122(filter3, 0, filtermixer3, 0);
AudioConnection          patchCord123(filter3, 1, filtermixer3, 1);
AudioConnection          patchCord124(filter3, 2, filtermixer3, 2);
AudioConnection          patchCord125(filter5, 0, filtermixer5, 0);
AudioConnection          patchCord126(filter5, 1, filtermixer5, 1);
AudioConnection          patchCord127(filter5, 2, filtermixer5, 2);
AudioConnection          patchCord128(filter14, 0, filtermixer14, 0);
AudioConnection          patchCord129(filter14, 1, filtermixer14, 1);
AudioConnection          patchCord130(filter14, 2, filtermixer14, 2);
AudioConnection          patchCord131(filtermixer13, flange13);
AudioConnection          patchCord132(filtermixer13, freeverb13);
AudioConnection          patchCord133(filtermixer13, 0, synthmixer3, 3);
AudioConnection          patchCord134(filtermixer14, flange14);
AudioConnection          patchCord135(filtermixer14, freeverb14);
AudioConnection          patchCord136(filtermixer14, 0, synthmixer4, 3);
AudioConnection          patchCord137(filtermixer11, flange11);
AudioConnection          patchCord138(filtermixer11, freeverb11);
AudioConnection          patchCord139(filtermixer11, 0, synthmixer1, 3);
AudioConnection          patchCord140(filtermixer7, freeverb7);
AudioConnection          patchCord141(filtermixer7, 0, freeverbmixer7, 3);
AudioConnection          patchCord142(filtermixer1, freeverb1);
AudioConnection          patchCord143(filtermixer1, 0, freeverbmixer1, 3);
AudioConnection          patchCord144(filtermixer8, freeverb8);
AudioConnection          patchCord145(filtermixer8, 0, freeverbmixer8, 3);
AudioConnection          patchCord146(filtermixer12, flange12);
AudioConnection          patchCord147(filtermixer12, freeverb12);
AudioConnection          patchCord148(filtermixer12, 0, synthmixer2, 3);
AudioConnection          patchCord149(filtermixer4, 0, mixer1, 3);
AudioConnection          patchCord150(filtermixer6, 0, mixer2, 1);
AudioConnection          patchCord151(filtermixer2, freeverb2);
AudioConnection          patchCord152(filtermixer2, 0, freeverbmixer2, 3);
AudioConnection          patchCord153(filtermixer5, 0, mixer2, 0);
AudioConnection          patchCord154(filtermixer3, 0, mixer1, 2);
AudioConnection          patchCord155(freeverb1, 0, freeverbmixer1, 0);
AudioConnection          patchCord156(flange13, 0, synthmixer3, 0);
AudioConnection          patchCord157(freeverb13, 0, synthmixer3, 1);
AudioConnection          patchCord158(freeverb7, 0, freeverbmixer7, 0);
AudioConnection          patchCord159(flange12, 0, synthmixer2, 0);
AudioConnection          patchCord160(freeverb2, 0, freeverbmixer2, 0);
AudioConnection          patchCord161(flange14, 0, synthmixer4, 0);
AudioConnection          patchCord162(freeverb12, 0, synthmixer2, 1);
AudioConnection          patchCord163(freeverb8, 0, freeverbmixer8, 0);
AudioConnection          patchCord164(freeverb11, 0, synthmixer1, 1);
AudioConnection          patchCord165(freeverb14, 0, synthmixer4, 1);
AudioConnection          patchCord166(flange11, 0, synthmixer1, 0);
AudioConnection          patchCord167(freeverbmixer1, 0, mixer1, 0);
AudioConnection          patchCord168(synthmixer4, 0, mixersynth_end, 3);
AudioConnection          patchCord169(freeverbmixer7, 0, mixer2, 2);
AudioConnection          patchCord170(freeverbmixer8, 0, mixer2, 3);
AudioConnection          patchCord171(synthmixer1, 0, mixersynth_end, 0);
AudioConnection          patchCord172(synthmixer3, 0, mixersynth_end, 2);
AudioConnection          patchCord173(freeverbmixer2, 0, mixer1, 1);
AudioConnection          patchCord174(synthmixer2, 0, mixersynth_end, 1);
AudioConnection          patchCord175(mixer1, 0, mixer_end, 0);
AudioConnection          patchCord176(mixer2, 0, mixer_end, 1);
AudioConnection          patchCord177(amp0, 0, mixer0, 2);
AudioConnection          patchCord178(envelope0, 0, mixer0, 0);
AudioConnection          patchCord179(playRaw0, 0, mixer0, 1);
AudioConnection          patchCord180(mixersynth_end, 0, mixer_end, 2);
AudioConnection          patchCord181(mixer_end, 0, mixerPlay, 0);
AudioConnection          patchCord182(mixer_end, 0, mixerPlay, 1);
AudioConnection          patchCord183(mixer0, 0, mixerPlay, 2);
AudioConnection          patchCord184(mixerPlay, 0, i2s1, 0);
AudioConnection          patchCord185(mixerPlay, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=2987,1420
// GUItool: end automatically generated code


// --- ADDITIONAL sound declarations below here ... ---//
// --- AudioPlayArrayResmp doesnt exist in GUItool ---//
AudioAnalyzePeak         peak1; 
AudioConnection          patchCord2121(playRaw0, 0, peak1, 0);

AudioConnection snd0(sound0, envelope0);

AudioConnection snd1(sound1,0, BDMixer1,1);
AudioConnection snd2(sound2,0, HHMixer2,1);
AudioConnection snd3(sound3,0, SNMixer3,1);

AudioConnection snd4(sound4, envelope4);
AudioConnection snd5(sound5, envelope5);
AudioConnection snd6(sound6, envelope6);
AudioConnection snd7(sound7, envelope7);
AudioConnection snd8(sound8, envelope8);

