
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
AudioSynthWaveformDc     BDpitchAmt;     //xy=128.1388931274414,662.6388468742371
AudioMixer4              SNchaosMix;     //xy=133.00000762939453,893.3333435058594
AudioMixer4              HHchaosMix;     //xy=139.6666717529297,782.7221832275391
AudioSynthWaveformModulated SNtone;         //xy=291.3333282470703,875.0000610351562
AudioSynthWaveformModulated SNtone2;        //xy=291.47222900390625,925.0555419921875
AudioEffectEnvelope      BDpitchEnv;     //xy=293.38890075683594,661.77783203125
AudioSynthWaveformModulated HHtone2;        //xy=296.77781677246094,813.6111450195312
AudioSynthWaveformModulated HHtone;         //xy=301.0000305175781,764.5
AudioSynthNoisePink      pink1;          //xy=444.71432876586914,1601.5715065002441
AudioSynthWaveform       waveform13_1;   //xy=450.0000762939453,1703.8572731018066
AudioSynthWaveform       waveform13_2;   //xy=451.0000762939453,1741.8572731018066
AudioSynthWaveform       waveform11_1;   //xy=459.42859649658203,1329.1429023742676
AudioSynthWaveform       waveform11_2;   //xy=459.42859649658203,1375.1429023742676
AudioSynthWaveform       waveform12_1;   //xy=460.42859649658203,1424.1429023742676
AudioSynthWaveform       waveform12_2;   //xy=460.42859649658203,1462.1429023742676
AudioMixer4              HHtoneMix;      //xy=463.88892364501953,800.6944274902344
AudioMixer4              BDchaosMix;     //xy=467.5277862548828,684.3333625793457
AudioMixer4              SNtoneMix;      //xy=469.8611488342285,900.6666412353516
AudioSynthWaveform       waveform14_1;   //xy=470.4286460876465,1890.571590423584
AudioSynthWaveform       waveform14_2;   //xy=470.4286460876465,1925.571590423584
AudioEffectEnvelope      HHtoneEnv;      //xy=631.2500152587891,800.0000123977661
AudioEffectEnvelope      SNtoneEnv;      //xy=631.0000152587891,902.7500133514404
AudioSynthNoiseWhite     HHnoise;        //xy=635.0000152587891,762.2500114440918
AudioSynthWaveformModulated BDsaw;          //xy=638.8055191040039,712.2499809265137
AudioSynthNoiseWhite     SNnoise;        //xy=640.0000152587891,858.5000123977661
AudioSynthWaveformModulated BDsine;         //xy=643.5000152587891,672.2500085830688
AudioMixer4              mixer_waveform12; //xy=676.7143287658691,1579.5715065002441
AudioMixer4              mixer_waveform11; //xy=684.428596496582,1412.1429023742676
AudioMixer4              mixer_waveform13; //xy=684.0000762939453,1726.5716094970703
AudioSynthKarplusStrong  string13;       //xy=697.8571968078613,1779.428698539734
AudioSynthSimpleDrum     drum13;         //xy=700.1429023742676,1667.571575164795
AudioSynthSimpleDrum     drum11;         //xy=703.1428527832031,1341.857276916504
AudioSynthKarplusStrong  string12;       //xy=702.1429023742676,1634.0000944137573
AudioMixer4              mixer_waveform14; //xy=701.4286460876465,1896.571590423584
AudioSynthKarplusStrong  string11;       //xy=711.2856998443604,1475.0000801086426
AudioSynthSimpleDrum     drum12;         //xy=712.2857093811035,1525.5715427398682
AudioSynthKarplusStrong  string14;       //xy=715.1429061889648,1953.000087738037
AudioSynthSimpleDrum     drum14;         //xy=721.8572006225586,1842.857250213623
AudioMixer4              BDMix;          //xy=794.5000190734863,715.5000076293945
AudioMixer4              SNMix;          //xy=802.3214416503906,903.3571281433105
AudioMixer4              HHMix;          //xy=807.4999771118164,810.3928451538086
AudioMixer4              waveformmixer12; //xy=882.7143287658691,1582.5715065002441
AudioMixer4              waveformmixer13; //xy=898.0000610351562,1728.1430206298828
AudioMixer4              waveformmixer14; //xy=907.4286460876465,1900.571590423584
AudioMixer4              waveformmixer11; //xy=910.428596496582,1410.1429023742676
AudioMixer4              BDMixer1;       //xy=938.7500228881836,736.5000085830688
AudioMixer4              SNMixer3;       //xy=945.2500228881836,919.2500085830688
AudioMixer4              HHMixer2;       //xy=946.2500228881836,825.7500085830688
AudioEffectEnvelope      envelope8;      //xy=1115.2500228881836,1289.7500162124634
AudioEffectEnvelope      envelope2;      //xy=1118.0000076293945,814.999997138977
AudioEffectEnvelope      envelope3;      //xy=1118.0000076293945,882.999997138977
AudioEffectEnvelope      envelope1;      //xy=1119.0000076293945,757.999997138977
AudioEffectEnvelope      envelope7;      //xy=1117.7500228881836,1207.000014781952
AudioEffectEnvelope      envelope4;      //xy=1120.0000076293945,950.999997138977
AudioEffectEnvelope      envelope11;     //xy=1120.7142639160156,1435.1429023742676
AudioEffectEnvelope      envelope5;      //xy=1126.0000076293945,1020.999997138977
AudioEffectEnvelope      envelope12;     //xy=1126.2857398986816,1606.4286012649536
AudioEffectEnvelope      envelope6;      //xy=1131.0000228881836,1122.2500143051147
AudioEffectEnvelope      envelope13;     //xy=1128.5714874267578,1732.714367866516
AudioEffectEnvelope      envelope14;     //xy=1139.000057220459,1852.4286851882935
AudioAmplifier           amp11;          //xy=1266.9999961853027,1435.7142791748047
AudioAmplifier           amp12;          //xy=1271.1428871154785,1606.428593158722
AudioAmplifier           amp13;          //xy=1277.4286346435547,1730.7143597602844
AudioAmplifier           amp14;          //xy=1277.8572044372559,1851.4286770820618
AudioEffectBitcrusher    bitcrusher4;    //xy=1282.2777938842773,955.0833806991577
AudioEffectBitcrusher    bitcrusher7;    //xy=1286.2777976989746,1178.583384513855
AudioEffectBitcrusher    bitcrusher5;    //xy=1288.5277938842773,1032.3333821296692
AudioEffectBitcrusher    bitcrusher6;    //xy=1288.2777938842773,1107.5833835601807
AudioEffectBitcrusher    bitcrusher8;    //xy=1296.0277976989746,1284.3333864212036
AudioEffectBitcrusher    bitcrusher3;    //xy=1299.7777976989746,884.0833806991577
AudioEffectBitcrusher    bitcrusher2;    //xy=1303.0277976989746,826.5833806991577
AudioEffectBitcrusher    bitcrusher1;    //xy=1309.2777938842773,760.3333787918091
AudioEffectBitcrusher    bitcrusher11;   //xy=1415.2856521606445,1436.7142791748047
AudioEffectBitcrusher    bitcrusher12;   //xy=1417.285732269287,1604.4286041259766
AudioEffectBitcrusher    bitcrusher13;   //xy=1422.5714797973633,1731.714370727539
AudioEffectBitcrusher    bitcrusher14;   //xy=1444.0000495910645,1853.4286880493164
AudioAmplifier           amp8;           //xy=1450.6347427368164,1234.0000896453857
AudioAmplifier           amp7;           //xy=1457.3013801574707,1164.1946258544922
AudioAmplifier           amp6;           //xy=1461.5236587524414,1089.9169263839722
AudioAmplifier           amp3;           //xy=1474.4124641418457,904.4446058273315
AudioAmplifier           amp4;           //xy=1482.6347427368164,959.8890056610107
AudioAmplifier           amp1;           //xy=1487.0793228149414,785.7779188156128
AudioAmplifier           amp2;           //xy=1491.666561126709,846.6668438911438
AudioAmplifier           amp5;           //xy=1492.9681015014648,1025.3334798812866
AudioFilterStateVariable filter13;       //xy=1583.5714797973633,1737.714370727539
AudioFilterStateVariable filter12;       //xy=1587.285732269287,1612.4286041259766
AudioFilterStateVariable filter11;       //xy=1595,1444
AudioFilterStateVariable filter14;       //xy=1602.0000495910645,1860.4286880493164
AudioFilterStateVariable filter8;        //xy=1608.2500228881836,1292.7500190734863
AudioFilterStateVariable filter7;        //xy=1615.2500228881836,1210.7500171661377
AudioFilterStateVariable filter6;        //xy=1617.2500228881836,1115.5000162124634
AudioFilterStateVariable filter1;        //xy=1622,762
AudioFilterStateVariable filter5;        //xy=1621.2500228881836,1034.750015258789
AudioFilterStateVariable filter2;        //xy=1625,820
AudioFilterStateVariable filter3;        //xy=1628,883
AudioFilterStateVariable filter4;        //xy=1629,959
AudioMixer4              filtermixer13;  //xy=1752.5714797973633,1743.714370727539
AudioMixer4              filtermixer12;  //xy=1756.285732269287,1618.4286041259766
AudioMixer4              filtermixer11;  //xy=1762.714256286621,1449.4285125732422
AudioMixer4              filtermixer14;  //xy=1769.0000495910645,1866.4286880493164
AudioMixer4              filtermixer8;   //xy=1780.5000267028809,1292.7500190734863
AudioMixer4              filtermixer7;   //xy=1785.7500267028809,1212.0000190734863
AudioMixer4              filtermixer5;   //xy=1792,1037
AudioMixer4              filtermixer6;   //xy=1792.0000305175781,1123.000015258789
AudioMixer4              filtermixer2;   //xy=1798,821
AudioMixer4              filtermixer4;   //xy=1799,964
AudioMixer4              filtermixer1;   //xy=1800,755
AudioMixer4              filtermixer3;   //xy=1802,889
AudioEffectFlange        flange14;       //xy=1932.7142333984375,1836.142894744873
AudioEffectFlange        flange12;       //xy=1933.8572311401367,1583.142882347107
AudioEffectFlange        flange13;       //xy=1935.8572692871094,1711.142984867096
AudioEffectFreeverb      freeverb13;     //xy=1936.1429824829102,1777.428611755371
AudioEffectFreeverb      freeverb12;     //xy=1936.7143936157227,1653.4286136627197
AudioEffectFreeverb      freeverb14;     //xy=1937.000144958496,1918.0001583099365
AudioEffectFreeverb      freeverb11;     //xy=1939.714340209961,1490.4285469055176
AudioEffectFlange        flange11;       //xy=1943.0000534057617,1421.57142162323
AudioEffectFreeverb      freeverb1;      //xy=1949.7500305175781,727.2500095367432
AudioEffectFreeverb      freeverb7;      //xy=1953.7500305175781,1175.7500171661377
AudioEffectFreeverb      freeverb2;      //xy=1957.0000305175781,805.2500114440918
AudioEffectFreeverb      freeverb8;      //xy=1959.5000305175781,1261.0000190734863
AudioMixer4              freeverbmixer7; //xy=2115.750030517578,1194.2500190734863
AudioMixer4              freeverbmixer8; //xy=2119.750030517578,1288.0000190734863
AudioMixer4              freeverbmixer1; //xy=2130.500030517578,747.7500114440918
AudioMixer4              freeverbmixer2; //xy=2130.500030517578,836.7500114440918
AudioEffectEnvelope      envelope0;      //xy=2278.750030517578,630.5000095367432
AudioPlaySdRaw           playRaw0;       //xy=2284.0003356933594,671.6666736602783
AudioMixer4              synthmixer1;    //xy=2283.285572052002,1452.0000076293945
AudioMixer4              synthmixer3;    //xy=2284.000068664551,1726.5715370178223
AudioMixer4              synthmixer2;    //xy=2289.142921447754,1604.0000076293945
AudioMixer4              synthmixer4;   //xy=2289.1428146362305,1881.8571472167969
AudioAmplifier           amp0;           //xy=2394.3016395568848,715.3333683013916
AudioMixer4              mixersynth_end; //xy=2580.8572387695312,1691.1428298950195
AudioMixer4              mixer2;         //xy=2584.9999084472656,1157.5832977294922
AudioMixer4              mixer1;         //xy=2590.6109714508057,810.7499866485596
AudioMixer4              mixer0;         //xy=2634.555652618408,678.9166927337646
AudioMixer4              mixer_end;      //xy=2754.571517944336,1260.5714073181152
AudioMixer4              mixerPlay;      //xy=2949.555507659912,1262.222204208374
AudioInputI2S            is23;           //xy=2975,1373
AudioOutputI2S           i2s1;           //xy=3138.555652618408,1261.1111469268799
AudioRecordQueue         queue1;         //xy=3171,1372
AudioConnection          patchCord1(BDpitchAmt, BDpitchEnv);
AudioConnection          patchCord2(SNchaosMix, 0, SNtone, 0);
AudioConnection          patchCord3(SNchaosMix, 0, SNtone2, 0);
AudioConnection          patchCord4(HHchaosMix, 0, HHtone, 0);
AudioConnection          patchCord5(HHchaosMix, 0, HHtone2, 0);
AudioConnection          patchCord6(SNtone, 0, SNtoneMix, 0);
AudioConnection          patchCord7(SNtone2, 0, SNchaosMix, 0);
AudioConnection          patchCord8(SNtone2, 0, SNtoneMix, 1);
AudioConnection          patchCord9(BDpitchEnv, 0, BDchaosMix, 0);
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
AudioConnection          patchCord21(waveform12_1, 0, mixer_waveform12, 0);
AudioConnection          patchCord22(waveform12_2, 0, mixer_waveform12, 1);
AudioConnection          patchCord23(HHtoneMix, HHtoneEnv);
AudioConnection          patchCord24(BDchaosMix, 0, BDsine, 0);
AudioConnection          patchCord25(BDchaosMix, 0, BDsaw, 0);
AudioConnection          patchCord26(BDchaosMix, 0, BDsaw, 1);
AudioConnection          patchCord27(SNtoneMix, SNtoneEnv);
AudioConnection          patchCord28(waveform14_1, 0, mixer_waveform14, 0);
AudioConnection          patchCord29(waveform14_2, 0, mixer_waveform14, 1);
AudioConnection          patchCord30(HHtoneEnv, 0, HHMix, 1);
AudioConnection          patchCord31(SNtoneEnv, 0, SNMix, 1);
AudioConnection          patchCord32(HHnoise, 0, HHMix, 0);
AudioConnection          patchCord33(BDsaw, 0, BDMix, 1);
AudioConnection          patchCord34(BDsaw, 0, BDchaosMix, 1);
AudioConnection          patchCord35(SNnoise, 0, SNMix, 0);
AudioConnection          patchCord36(BDsine, 0, BDMix, 0);
AudioConnection          patchCord37(mixer_waveform12, 0, waveformmixer12, 1);
AudioConnection          patchCord38(mixer_waveform11, 0, waveformmixer11, 1);
AudioConnection          patchCord39(mixer_waveform13, 0, waveformmixer13, 1);
AudioConnection          patchCord40(string13, 0, waveformmixer13, 2);
AudioConnection          patchCord41(drum13, 0, waveformmixer13, 0);
AudioConnection          patchCord42(drum11, 0, waveformmixer11, 0);
AudioConnection          patchCord43(string12, 0, waveformmixer12, 2);
AudioConnection          patchCord44(mixer_waveform14, 0, waveformmixer14, 1);
AudioConnection          patchCord45(string11, 0, waveformmixer11, 2);
AudioConnection          patchCord46(drum12, 0, waveformmixer12, 0);
AudioConnection          patchCord47(string14, 0, waveformmixer14, 2);
AudioConnection          patchCord48(drum14, 0, waveformmixer14, 0);
AudioConnection          patchCord49(BDMix, 0, BDMixer1, 0);
AudioConnection          patchCord50(SNMix, 0, SNMixer3, 0);
AudioConnection          patchCord51(HHMix, 0, HHMixer2, 0);
AudioConnection          patchCord52(waveformmixer12, envelope12);
AudioConnection          patchCord53(waveformmixer13, envelope13);
AudioConnection          patchCord54(waveformmixer14, envelope14);
AudioConnection          patchCord55(waveformmixer11, envelope11);
AudioConnection          patchCord56(BDMixer1, envelope1);
AudioConnection          patchCord57(SNMixer3, envelope3);
AudioConnection          patchCord58(HHMixer2, envelope2);
AudioConnection          patchCord59(envelope8, bitcrusher8);
AudioConnection          patchCord60(envelope2, bitcrusher2);
AudioConnection          patchCord61(envelope3, bitcrusher3);
AudioConnection          patchCord62(envelope1, bitcrusher1);
AudioConnection          patchCord63(envelope7, bitcrusher7);
AudioConnection          patchCord64(envelope4, bitcrusher4);
AudioConnection          patchCord65(envelope11, amp11);
AudioConnection          patchCord66(envelope5, bitcrusher5);
AudioConnection          patchCord67(envelope12, amp12);
AudioConnection          patchCord68(envelope6, bitcrusher6);
AudioConnection          patchCord69(envelope13, amp13);
AudioConnection          patchCord70(envelope14, amp14);
AudioConnection          patchCord71(amp11, bitcrusher11);
AudioConnection          patchCord72(amp12, bitcrusher12);
AudioConnection          patchCord73(amp13, bitcrusher13);
AudioConnection          patchCord74(amp14, bitcrusher14);
AudioConnection          patchCord75(bitcrusher4, amp4);
AudioConnection          patchCord76(bitcrusher7, amp7);
AudioConnection          patchCord77(bitcrusher5, amp5);
AudioConnection          patchCord78(bitcrusher6, amp6);
AudioConnection          patchCord79(bitcrusher8, amp8);
AudioConnection          patchCord80(bitcrusher3, amp3);
AudioConnection          patchCord81(bitcrusher2, amp2);
AudioConnection          patchCord82(bitcrusher1, amp1);
AudioConnection          patchCord83(bitcrusher11, 0, filter11, 0);
AudioConnection          patchCord84(bitcrusher12, 0, filter12, 0);
AudioConnection          patchCord85(bitcrusher13, 0, filter13, 0);
AudioConnection          patchCord86(bitcrusher14, 0, filter14, 0);
AudioConnection          patchCord87(amp8, 0, filter8, 0);
AudioConnection          patchCord88(amp7, 0, filter7, 0);
AudioConnection          patchCord89(amp6, 0, filter6, 0);
AudioConnection          patchCord90(amp3, 0, filter3, 0);
AudioConnection          patchCord91(amp4, 0, filter4, 0);
AudioConnection          patchCord92(amp1, 0, filter1, 0);
AudioConnection          patchCord93(amp2, 0, filter2, 0);
AudioConnection          patchCord94(amp5, 0, filter5, 0);
AudioConnection          patchCord95(filter13, 0, filtermixer13, 0);
AudioConnection          patchCord96(filter13, 1, filtermixer13, 1);
AudioConnection          patchCord97(filter13, 2, filtermixer13, 2);
AudioConnection          patchCord98(filter12, 0, filtermixer12, 0);
AudioConnection          patchCord99(filter12, 1, filtermixer12, 1);
AudioConnection          patchCord100(filter12, 2, filtermixer12, 2);
AudioConnection          patchCord101(filter11, 0, filtermixer11, 0);
AudioConnection          patchCord102(filter11, 1, filtermixer11, 1);
AudioConnection          patchCord103(filter11, 2, filtermixer11, 2);
AudioConnection          patchCord104(filter14, 0, filtermixer14, 0);
AudioConnection          patchCord105(filter14, 1, filtermixer14, 1);
AudioConnection          patchCord106(filter14, 2, filtermixer14, 2);
AudioConnection          patchCord107(filter8, 0, filtermixer8, 0);
AudioConnection          patchCord108(filter8, 1, filtermixer8, 1);
AudioConnection          patchCord109(filter8, 2, filtermixer8, 2);
AudioConnection          patchCord110(filter7, 0, filtermixer7, 0);
AudioConnection          patchCord111(filter7, 1, filtermixer7, 1);
AudioConnection          patchCord112(filter7, 2, filtermixer7, 2);
AudioConnection          patchCord113(filter6, 0, filtermixer6, 0);
AudioConnection          patchCord114(filter6, 1, filtermixer6, 1);
AudioConnection          patchCord115(filter6, 2, filtermixer6, 2);
AudioConnection          patchCord116(filter1, 0, filtermixer1, 0);
AudioConnection          patchCord117(filter1, 1, filtermixer1, 1);
AudioConnection          patchCord118(filter1, 2, filtermixer1, 2);
AudioConnection          patchCord119(filter5, 0, filtermixer5, 0);
AudioConnection          patchCord120(filter5, 1, filtermixer5, 1);
AudioConnection          patchCord121(filter5, 2, filtermixer5, 2);
AudioConnection          patchCord122(filter2, 0, filtermixer2, 0);
AudioConnection          patchCord123(filter2, 1, filtermixer2, 1);
AudioConnection          patchCord124(filter2, 2, filtermixer2, 2);
AudioConnection          patchCord125(filter3, 0, filtermixer3, 0);
AudioConnection          patchCord126(filter3, 1, filtermixer3, 1);
AudioConnection          patchCord127(filter3, 2, filtermixer3, 2);
AudioConnection          patchCord128(filter4, 0, filtermixer4, 0);
AudioConnection          patchCord129(filter4, 1, filtermixer4, 1);
AudioConnection          patchCord130(filter4, 2, filtermixer4, 2);
AudioConnection          patchCord131(filtermixer13, flange13);
AudioConnection          patchCord132(filtermixer13, freeverb13);
AudioConnection          patchCord133(filtermixer13, 0, synthmixer3, 3);
AudioConnection          patchCord134(filtermixer12, flange12);
AudioConnection          patchCord135(filtermixer12, freeverb12);
AudioConnection          patchCord136(filtermixer12, 0, synthmixer2, 3);
AudioConnection          patchCord137(filtermixer11, flange11);
AudioConnection          patchCord138(filtermixer11, freeverb11);
AudioConnection          patchCord139(filtermixer11, 0, synthmixer1, 3);
AudioConnection          patchCord140(filtermixer14, flange14);
AudioConnection          patchCord141(filtermixer14, freeverb14);
AudioConnection          patchCord142(filtermixer14, 0, synthmixer4, 3);
AudioConnection          patchCord143(filtermixer8, freeverb8);
AudioConnection          patchCord144(filtermixer8, 0, freeverbmixer8, 3);
AudioConnection          patchCord145(filtermixer7, freeverb7);
AudioConnection          patchCord146(filtermixer7, 0, freeverbmixer7, 3);
AudioConnection          patchCord147(filtermixer5, 0, mixer2, 0);
AudioConnection          patchCord148(filtermixer6, 0, mixer2, 1);
AudioConnection          patchCord149(filtermixer2, freeverb2);
AudioConnection          patchCord150(filtermixer2, 0, freeverbmixer2, 3);
AudioConnection          patchCord151(filtermixer4, 0, mixer1, 3);
AudioConnection          patchCord152(filtermixer1, freeverb1);
AudioConnection          patchCord153(filtermixer1, 0, freeverbmixer1, 3);
AudioConnection          patchCord154(filtermixer3, 0, mixer1, 2);
AudioConnection          patchCord155(flange14, 0, synthmixer4, 0);
AudioConnection          patchCord156(flange12, 0, synthmixer2, 0);
AudioConnection          patchCord157(flange13, 0, synthmixer3, 0);
AudioConnection          patchCord158(freeverb13, 0, synthmixer3, 1);
AudioConnection          patchCord159(freeverb12, 0, synthmixer2, 1);
AudioConnection          patchCord160(freeverb14, 0, synthmixer4, 1);
AudioConnection          patchCord161(freeverb11, 0, synthmixer1, 1);
AudioConnection          patchCord162(flange11, 0, synthmixer1, 0);
AudioConnection          patchCord163(freeverb1, 0, freeverbmixer1, 0);
AudioConnection          patchCord164(freeverb7, 0, freeverbmixer7, 0);
AudioConnection          patchCord165(freeverb2, 0, freeverbmixer2, 0);
AudioConnection          patchCord166(freeverb8, 0, freeverbmixer8, 0);
AudioConnection          patchCord167(freeverbmixer7, 0, mixer2, 2);
AudioConnection          patchCord168(freeverbmixer8, 0, mixer2, 3);
AudioConnection          patchCord169(freeverbmixer1, 0, mixer1, 0);
AudioConnection          patchCord170(freeverbmixer2, 0, mixer1, 1);
AudioConnection          patchCord171(envelope0, 0, mixer0, 0);
AudioConnection          patchCord172(playRaw0, 0, mixer0, 1);
AudioConnection          patchCord173(synthmixer1, 0, mixersynth_end, 0);
AudioConnection          patchCord174(synthmixer3, 0, mixersynth_end, 2);
AudioConnection          patchCord175(synthmixer2, 0, mixersynth_end, 1);
AudioConnection          patchCord176(synthmixer4, 0, mixersynth_end, 3);
AudioConnection          patchCord177(amp0, 0, mixer0, 2);
AudioConnection          patchCord178(mixersynth_end, 0, mixer_end, 2);
AudioConnection          patchCord179(mixer2, 0, mixer_end, 1);
AudioConnection          patchCord180(mixer1, 0, mixer_end, 0);
AudioConnection          patchCord181(mixer0, 0, mixerPlay, 2);
AudioConnection          patchCord182(mixer_end, 0, mixerPlay, 0);
AudioConnection          patchCord183(mixer_end, 0, mixerPlay, 1);
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

