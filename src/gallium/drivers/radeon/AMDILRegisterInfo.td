//===- AMDILRegisterInfo.td - AMDIL Register defs ----------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
//  Declarations that describe the AMDIL register file
//
//===----------------------------------------------------------------------===//

class AMDILReg<bits<16> num, string n> : Register<n> {
  field bits<16> Value;
  let Value = num;
  let Namespace = "AMDIL";
}

// We will start with 8 registers for each class before expanding to more
// Since the swizzle is added based on the register class, we can leave it
// off here and just specify different registers for different register classes
def R1 : AMDILReg<1, "r1">, DwarfRegNum<[1]>;
def R2 : AMDILReg<2, "r2">, DwarfRegNum<[2]>;
def R3 : AMDILReg<3, "r3">, DwarfRegNum<[3]>;
def R4 : AMDILReg<4, "r4">, DwarfRegNum<[4]>;
def R5 : AMDILReg<5, "r5">, DwarfRegNum<[5]>;
def R6 : AMDILReg<6, "r6">, DwarfRegNum<[6]>;
def R7 : AMDILReg<7, "r7">, DwarfRegNum<[7]>;
def R8 : AMDILReg<8, "r8">, DwarfRegNum<[8]>;
def R9 : AMDILReg<9, "r9">, DwarfRegNum<[9]>;
def R10 : AMDILReg<10, "r10">, DwarfRegNum<[10]>;
def R11 : AMDILReg<11, "r11">, DwarfRegNum<[11]>;
def R12 : AMDILReg<12, "r12">, DwarfRegNum<[12]>;
def R13 : AMDILReg<13, "r13">, DwarfRegNum<[13]>;
def R14 : AMDILReg<14, "r14">, DwarfRegNum<[14]>;
def R15 : AMDILReg<15, "r15">, DwarfRegNum<[15]>;
def R16 : AMDILReg<16, "r16">, DwarfRegNum<[16]>;
def R17 : AMDILReg<17, "r17">, DwarfRegNum<[17]>;
def R18 : AMDILReg<18, "r18">, DwarfRegNum<[18]>;
def R19 : AMDILReg<19, "r19">, DwarfRegNum<[19]>;
def R20 : AMDILReg<20, "r20">, DwarfRegNum<[20]>;
def R21 : AMDILReg<21, "r21">, DwarfRegNum<[21]>;
def R22 : AMDILReg<22, "r22">, DwarfRegNum<[22]>;
def R23 : AMDILReg<23, "r23">, DwarfRegNum<[23]>;
def R24 : AMDILReg<24, "r24">, DwarfRegNum<[24]>;
def R25 : AMDILReg<25, "r25">, DwarfRegNum<[25]>;
def R26 : AMDILReg<26, "r26">, DwarfRegNum<[26]>;
def R27 : AMDILReg<27, "r27">, DwarfRegNum<[27]>;
def R28 : AMDILReg<28, "r28">, DwarfRegNum<[28]>;
def R29 : AMDILReg<29, "r29">, DwarfRegNum<[29]>;
def R30 : AMDILReg<30, "r30">, DwarfRegNum<[30]>;
def R31 : AMDILReg<31, "r31">, DwarfRegNum<[31]>;
def R32 : AMDILReg<32, "r32">, DwarfRegNum<[32]>;
def R33 : AMDILReg<33, "r33">, DwarfRegNum<[33]>;
def R34 : AMDILReg<34, "r34">, DwarfRegNum<[34]>;
def R35 : AMDILReg<35, "r35">, DwarfRegNum<[35]>;
def R36 : AMDILReg<36, "r36">, DwarfRegNum<[36]>;
def R37 : AMDILReg<37, "r37">, DwarfRegNum<[37]>;
def R38 : AMDILReg<38, "r38">, DwarfRegNum<[38]>;
def R39 : AMDILReg<39, "r39">, DwarfRegNum<[39]>;
def R40 : AMDILReg<40, "r40">, DwarfRegNum<[40]>;
def R41 : AMDILReg<41, "r41">, DwarfRegNum<[41]>;
def R42 : AMDILReg<42, "r42">, DwarfRegNum<[42]>;
def R43 : AMDILReg<43, "r43">, DwarfRegNum<[43]>;
def R44 : AMDILReg<44, "r44">, DwarfRegNum<[44]>;
def R45 : AMDILReg<45, "r45">, DwarfRegNum<[45]>;
def R46 : AMDILReg<46, "r46">, DwarfRegNum<[46]>;
def R47 : AMDILReg<47, "r47">, DwarfRegNum<[47]>;
def R48 : AMDILReg<48, "r48">, DwarfRegNum<[48]>;
def R49 : AMDILReg<49, "r49">, DwarfRegNum<[49]>;
def R50 : AMDILReg<50, "r50">, DwarfRegNum<[50]>;
def R51 : AMDILReg<51, "r51">, DwarfRegNum<[51]>;
def R52 : AMDILReg<52, "r52">, DwarfRegNum<[52]>;
def R53 : AMDILReg<53, "r53">, DwarfRegNum<[53]>;
def R54 : AMDILReg<54, "r54">, DwarfRegNum<[54]>;
def R55 : AMDILReg<55, "r55">, DwarfRegNum<[55]>;
def R56 : AMDILReg<56, "r56">, DwarfRegNum<[56]>;
def R57 : AMDILReg<57, "r57">, DwarfRegNum<[57]>;
def R58 : AMDILReg<58, "r58">, DwarfRegNum<[58]>;
def R59 : AMDILReg<59, "r59">, DwarfRegNum<[59]>;
def R60 : AMDILReg<60, "r60">, DwarfRegNum<[60]>;
def R61 : AMDILReg<61, "r61">, DwarfRegNum<[61]>;
def R62 : AMDILReg<62, "r62">, DwarfRegNum<[62]>;
def R63 : AMDILReg<63, "r63">, DwarfRegNum<[63]>;
def R64 : AMDILReg<64, "r64">, DwarfRegNum<[64]>;
def R65 : AMDILReg<65, "r65">, DwarfRegNum<[65]>;
def R66 : AMDILReg<66, "r66">, DwarfRegNum<[66]>;
def R67 : AMDILReg<67, "r67">, DwarfRegNum<[67]>;
def R68 : AMDILReg<68, "r68">, DwarfRegNum<[68]>;
def R69 : AMDILReg<69, "r69">, DwarfRegNum<[69]>;
def R70 : AMDILReg<70, "r70">, DwarfRegNum<[70]>;
def R71 : AMDILReg<71, "r71">, DwarfRegNum<[71]>;
def R72 : AMDILReg<72, "r72">, DwarfRegNum<[72]>;
def R73 : AMDILReg<73, "r73">, DwarfRegNum<[73]>;
def R74 : AMDILReg<74, "r74">, DwarfRegNum<[74]>;
def R75 : AMDILReg<75, "r75">, DwarfRegNum<[75]>;
def R76 : AMDILReg<76, "r76">, DwarfRegNum<[76]>;
def R77 : AMDILReg<77, "r77">, DwarfRegNum<[77]>;
def R78 : AMDILReg<78, "r78">, DwarfRegNum<[78]>;
def R79 : AMDILReg<79, "r79">, DwarfRegNum<[79]>;
def R80 : AMDILReg<80, "r80">, DwarfRegNum<[80]>;
def R81 : AMDILReg<81, "r81">, DwarfRegNum<[81]>;
def R82 : AMDILReg<82, "r82">, DwarfRegNum<[82]>;
def R83 : AMDILReg<83, "r83">, DwarfRegNum<[83]>;
def R84 : AMDILReg<84, "r84">, DwarfRegNum<[84]>;
def R85 : AMDILReg<85, "r85">, DwarfRegNum<[85]>;
def R86 : AMDILReg<86, "r86">, DwarfRegNum<[86]>;
def R87 : AMDILReg<87, "r87">, DwarfRegNum<[87]>;
def R88 : AMDILReg<88, "r88">, DwarfRegNum<[88]>;
def R89 : AMDILReg<89, "r89">, DwarfRegNum<[89]>;
def R90 : AMDILReg<90, "r90">, DwarfRegNum<[90]>;
def R91 : AMDILReg<91, "r91">, DwarfRegNum<[91]>;
def R92 : AMDILReg<92, "r92">, DwarfRegNum<[92]>;
def R93 : AMDILReg<93, "r93">, DwarfRegNum<[93]>;
def R94 : AMDILReg<94, "r94">, DwarfRegNum<[94]>;
def R95 : AMDILReg<95, "r95">, DwarfRegNum<[95]>;
def R96 : AMDILReg<96, "r96">, DwarfRegNum<[96]>;
def R97 : AMDILReg<97, "r97">, DwarfRegNum<[97]>;
def R98 : AMDILReg<98, "r98">, DwarfRegNum<[98]>;
def R99 : AMDILReg<99, "r99">, DwarfRegNum<[99]>;
def R100 : AMDILReg<100, "r100">, DwarfRegNum<[100]>;
def R101 : AMDILReg<101, "r101">, DwarfRegNum<[101]>;
def R102 : AMDILReg<102, "r102">, DwarfRegNum<[102]>;
def R103 : AMDILReg<103, "r103">, DwarfRegNum<[103]>;
def R104 : AMDILReg<104, "r104">, DwarfRegNum<[104]>;
def R105 : AMDILReg<105, "r105">, DwarfRegNum<[105]>;
def R106 : AMDILReg<106, "r106">, DwarfRegNum<[106]>;
def R107 : AMDILReg<107, "r107">, DwarfRegNum<[107]>;
def R108 : AMDILReg<108, "r108">, DwarfRegNum<[108]>;
def R109 : AMDILReg<109, "r109">, DwarfRegNum<[109]>;
def R110 : AMDILReg<110, "r110">, DwarfRegNum<[110]>;
def R111 : AMDILReg<111, "r111">, DwarfRegNum<[111]>;
def R112 : AMDILReg<112, "r112">, DwarfRegNum<[112]>;
def R113 : AMDILReg<113, "r113">, DwarfRegNum<[113]>;
def R114 : AMDILReg<114, "r114">, DwarfRegNum<[114]>;
def R115 : AMDILReg<115, "r115">, DwarfRegNum<[115]>;
def R116 : AMDILReg<116, "r116">, DwarfRegNum<[116]>;
def R117 : AMDILReg<117, "r117">, DwarfRegNum<[117]>;
def R118 : AMDILReg<118, "r118">, DwarfRegNum<[118]>;
def R119 : AMDILReg<119, "r119">, DwarfRegNum<[119]>;
def R120 : AMDILReg<120, "r120">, DwarfRegNum<[120]>;
def R121 : AMDILReg<121, "r121">, DwarfRegNum<[121]>;
def R122 : AMDILReg<122, "r122">, DwarfRegNum<[122]>;
def R123 : AMDILReg<123, "r123">, DwarfRegNum<[123]>;
def R124 : AMDILReg<124, "r124">, DwarfRegNum<[124]>;
def R125 : AMDILReg<125, "r125">, DwarfRegNum<[125]>;
def R126 : AMDILReg<126, "r126">, DwarfRegNum<[126]>;
def R127 : AMDILReg<127, "r127">, DwarfRegNum<[127]>;
def R128 : AMDILReg<128, "r128">, DwarfRegNum<[128]>;
def R129 : AMDILReg<129, "r129">, DwarfRegNum<[129]>;
def R130 : AMDILReg<130, "r130">, DwarfRegNum<[130]>;
def R131 : AMDILReg<131, "r131">, DwarfRegNum<[131]>;
def R132 : AMDILReg<132, "r132">, DwarfRegNum<[132]>;
def R133 : AMDILReg<133, "r133">, DwarfRegNum<[133]>;
def R134 : AMDILReg<134, "r134">, DwarfRegNum<[134]>;
def R135 : AMDILReg<135, "r135">, DwarfRegNum<[135]>;
def R136 : AMDILReg<136, "r136">, DwarfRegNum<[136]>;
def R137 : AMDILReg<137, "r137">, DwarfRegNum<[137]>;
def R138 : AMDILReg<138, "r138">, DwarfRegNum<[138]>;
def R139 : AMDILReg<139, "r139">, DwarfRegNum<[139]>;
def R140 : AMDILReg<140, "r140">, DwarfRegNum<[140]>;
def R141 : AMDILReg<141, "r141">, DwarfRegNum<[141]>;
def R142 : AMDILReg<142, "r142">, DwarfRegNum<[142]>;
def R143 : AMDILReg<143, "r143">, DwarfRegNum<[143]>;
def R144 : AMDILReg<144, "r144">, DwarfRegNum<[144]>;
def R145 : AMDILReg<145, "r145">, DwarfRegNum<[145]>;
def R146 : AMDILReg<146, "r146">, DwarfRegNum<[146]>;
def R147 : AMDILReg<147, "r147">, DwarfRegNum<[147]>;
def R148 : AMDILReg<148, "r148">, DwarfRegNum<[148]>;
def R149 : AMDILReg<149, "r149">, DwarfRegNum<[149]>;
def R150 : AMDILReg<150, "r150">, DwarfRegNum<[150]>;
def R151 : AMDILReg<151, "r151">, DwarfRegNum<[151]>;
def R152 : AMDILReg<152, "r152">, DwarfRegNum<[152]>;
def R153 : AMDILReg<153, "r153">, DwarfRegNum<[153]>;
def R154 : AMDILReg<154, "r154">, DwarfRegNum<[154]>;
def R155 : AMDILReg<155, "r155">, DwarfRegNum<[155]>;
def R156 : AMDILReg<156, "r156">, DwarfRegNum<[156]>;
def R157 : AMDILReg<157, "r157">, DwarfRegNum<[157]>;
def R158 : AMDILReg<158, "r158">, DwarfRegNum<[158]>;
def R159 : AMDILReg<159, "r159">, DwarfRegNum<[159]>;
def R160 : AMDILReg<160, "r160">, DwarfRegNum<[160]>;
def R161 : AMDILReg<161, "r161">, DwarfRegNum<[161]>;
def R162 : AMDILReg<162, "r162">, DwarfRegNum<[162]>;
def R163 : AMDILReg<163, "r163">, DwarfRegNum<[163]>;
def R164 : AMDILReg<164, "r164">, DwarfRegNum<[164]>;
def R165 : AMDILReg<165, "r165">, DwarfRegNum<[165]>;
def R166 : AMDILReg<166, "r166">, DwarfRegNum<[166]>;
def R167 : AMDILReg<167, "r167">, DwarfRegNum<[167]>;
def R168 : AMDILReg<168, "r168">, DwarfRegNum<[168]>;
def R169 : AMDILReg<169, "r169">, DwarfRegNum<[169]>;
def R170 : AMDILReg<170, "r170">, DwarfRegNum<[170]>;
def R171 : AMDILReg<171, "r171">, DwarfRegNum<[171]>;
def R172 : AMDILReg<172, "r172">, DwarfRegNum<[172]>;
def R173 : AMDILReg<173, "r173">, DwarfRegNum<[173]>;
def R174 : AMDILReg<174, "r174">, DwarfRegNum<[174]>;
def R175 : AMDILReg<175, "r175">, DwarfRegNum<[175]>;
def R176 : AMDILReg<176, "r176">, DwarfRegNum<[176]>;
def R177 : AMDILReg<177, "r177">, DwarfRegNum<[177]>;
def R178 : AMDILReg<178, "r178">, DwarfRegNum<[178]>;
def R179 : AMDILReg<179, "r179">, DwarfRegNum<[179]>;
def R180 : AMDILReg<180, "r180">, DwarfRegNum<[180]>;
def R181 : AMDILReg<181, "r181">, DwarfRegNum<[181]>;
def R182 : AMDILReg<182, "r182">, DwarfRegNum<[182]>;
def R183 : AMDILReg<183, "r183">, DwarfRegNum<[183]>;
def R184 : AMDILReg<184, "r184">, DwarfRegNum<[184]>;
def R185 : AMDILReg<185, "r185">, DwarfRegNum<[185]>;
def R186 : AMDILReg<186, "r186">, DwarfRegNum<[186]>;
def R187 : AMDILReg<187, "r187">, DwarfRegNum<[187]>;
def R188 : AMDILReg<188, "r188">, DwarfRegNum<[188]>;
def R189 : AMDILReg<189, "r189">, DwarfRegNum<[189]>;
def R190 : AMDILReg<190, "r190">, DwarfRegNum<[190]>;
def R191 : AMDILReg<191, "r191">, DwarfRegNum<[191]>;
def R192 : AMDILReg<192, "r192">, DwarfRegNum<[192]>;
def R193 : AMDILReg<193, "r193">, DwarfRegNum<[193]>;
def R194 : AMDILReg<194, "r194">, DwarfRegNum<[194]>;
def R195 : AMDILReg<195, "r195">, DwarfRegNum<[195]>;
def R196 : AMDILReg<196, "r196">, DwarfRegNum<[196]>;
def R197 : AMDILReg<197, "r197">, DwarfRegNum<[197]>;
def R198 : AMDILReg<198, "r198">, DwarfRegNum<[198]>;
def R199 : AMDILReg<199, "r199">, DwarfRegNum<[199]>;
def R200 : AMDILReg<200, "r200">, DwarfRegNum<[200]>;
def R201 : AMDILReg<201, "r201">, DwarfRegNum<[201]>;
def R202 : AMDILReg<202, "r202">, DwarfRegNum<[202]>;
def R203 : AMDILReg<203, "r203">, DwarfRegNum<[203]>;
def R204 : AMDILReg<204, "r204">, DwarfRegNum<[204]>;
def R205 : AMDILReg<205, "r205">, DwarfRegNum<[205]>;
def R206 : AMDILReg<206, "r206">, DwarfRegNum<[206]>;
def R207 : AMDILReg<207, "r207">, DwarfRegNum<[207]>;
def R208 : AMDILReg<208, "r208">, DwarfRegNum<[208]>;
def R209 : AMDILReg<209, "r209">, DwarfRegNum<[209]>;
def R210 : AMDILReg<210, "r210">, DwarfRegNum<[210]>;
def R211 : AMDILReg<211, "r211">, DwarfRegNum<[211]>;
def R212 : AMDILReg<212, "r212">, DwarfRegNum<[212]>;
def R213 : AMDILReg<213, "r213">, DwarfRegNum<[213]>;
def R214 : AMDILReg<214, "r214">, DwarfRegNum<[214]>;
def R215 : AMDILReg<215, "r215">, DwarfRegNum<[215]>;
def R216 : AMDILReg<216, "r216">, DwarfRegNum<[216]>;
def R217 : AMDILReg<217, "r217">, DwarfRegNum<[217]>;
def R218 : AMDILReg<218, "r218">, DwarfRegNum<[218]>;
def R219 : AMDILReg<219, "r219">, DwarfRegNum<[219]>;
def R220 : AMDILReg<220, "r220">, DwarfRegNum<[220]>;
def R221 : AMDILReg<221, "r221">, DwarfRegNum<[221]>;
def R222 : AMDILReg<222, "r222">, DwarfRegNum<[222]>;
def R223 : AMDILReg<223, "r223">, DwarfRegNum<[223]>;
def R224 : AMDILReg<224, "r224">, DwarfRegNum<[224]>;
def R225 : AMDILReg<225, "r225">, DwarfRegNum<[225]>;
def R226 : AMDILReg<226, "r226">, DwarfRegNum<[226]>;
def R227 : AMDILReg<227, "r227">, DwarfRegNum<[227]>;
def R228 : AMDILReg<228, "r228">, DwarfRegNum<[228]>;
def R229 : AMDILReg<229, "r229">, DwarfRegNum<[229]>;
def R230 : AMDILReg<230, "r230">, DwarfRegNum<[230]>;
def R231 : AMDILReg<231, "r231">, DwarfRegNum<[231]>;
def R232 : AMDILReg<232, "r232">, DwarfRegNum<[232]>;
def R233 : AMDILReg<233, "r233">, DwarfRegNum<[233]>;
def R234 : AMDILReg<234, "r234">, DwarfRegNum<[234]>;
def R235 : AMDILReg<235, "r235">, DwarfRegNum<[235]>;
def R236 : AMDILReg<236, "r236">, DwarfRegNum<[236]>;
def R237 : AMDILReg<237, "r237">, DwarfRegNum<[237]>;
def R238 : AMDILReg<238, "r238">, DwarfRegNum<[238]>;
def R239 : AMDILReg<239, "r239">, DwarfRegNum<[239]>;
def R240 : AMDILReg<240, "r240">, DwarfRegNum<[240]>;
def R241 : AMDILReg<241, "r241">, DwarfRegNum<[241]>;
def R242 : AMDILReg<242, "r242">, DwarfRegNum<[242]>;
def R243 : AMDILReg<243, "r243">, DwarfRegNum<[243]>;
def R244 : AMDILReg<244, "r244">, DwarfRegNum<[244]>;
def R245 : AMDILReg<245, "r245">, DwarfRegNum<[245]>;
def R246 : AMDILReg<246, "r246">, DwarfRegNum<[246]>;
def R247 : AMDILReg<247, "r247">, DwarfRegNum<[247]>;
def R248 : AMDILReg<248, "r248">, DwarfRegNum<[248]>;
def R249 : AMDILReg<249, "r249">, DwarfRegNum<[249]>;
def R250 : AMDILReg<250, "r250">, DwarfRegNum<[250]>;
def R251 : AMDILReg<251, "r251">, DwarfRegNum<[251]>;
def R252 : AMDILReg<252, "r252">, DwarfRegNum<[252]>;
def R253 : AMDILReg<253, "r253">, DwarfRegNum<[253]>;
def R254 : AMDILReg<254, "r254">, DwarfRegNum<[254]>;
def R255 : AMDILReg<255, "r255">, DwarfRegNum<[255]>;
def R256 : AMDILReg<256, "r256">, DwarfRegNum<[256]>;
def R257 : AMDILReg<257, "r257">, DwarfRegNum<[257]>;
def R258 : AMDILReg<258, "r258">, DwarfRegNum<[258]>;
def R259 : AMDILReg<259, "r259">, DwarfRegNum<[259]>;
def R260 : AMDILReg<260, "r260">, DwarfRegNum<[260]>;
def R261 : AMDILReg<261, "r261">, DwarfRegNum<[261]>;
def R262 : AMDILReg<262, "r262">, DwarfRegNum<[262]>;
def R263 : AMDILReg<263, "r263">, DwarfRegNum<[263]>;
def R264 : AMDILReg<264, "r264">, DwarfRegNum<[264]>;
def R265 : AMDILReg<265, "r265">, DwarfRegNum<[265]>;
def R266 : AMDILReg<266, "r266">, DwarfRegNum<[266]>;
def R267 : AMDILReg<267, "r267">, DwarfRegNum<[267]>;
def R268 : AMDILReg<268, "r268">, DwarfRegNum<[268]>;
def R269 : AMDILReg<269, "r269">, DwarfRegNum<[269]>;
def R270 : AMDILReg<270, "r270">, DwarfRegNum<[270]>;
def R271 : AMDILReg<271, "r271">, DwarfRegNum<[271]>;
def R272 : AMDILReg<272, "r272">, DwarfRegNum<[272]>;
def R273 : AMDILReg<273, "r273">, DwarfRegNum<[273]>;
def R274 : AMDILReg<274, "r274">, DwarfRegNum<[274]>;
def R275 : AMDILReg<275, "r275">, DwarfRegNum<[275]>;
def R276 : AMDILReg<276, "r276">, DwarfRegNum<[276]>;
def R277 : AMDILReg<277, "r277">, DwarfRegNum<[277]>;
def R278 : AMDILReg<278, "r278">, DwarfRegNum<[278]>;
def R279 : AMDILReg<279, "r279">, DwarfRegNum<[279]>;
def R280 : AMDILReg<280, "r280">, DwarfRegNum<[280]>;
def R281 : AMDILReg<281, "r281">, DwarfRegNum<[281]>;
def R282 : AMDILReg<282, "r282">, DwarfRegNum<[282]>;
def R283 : AMDILReg<283, "r283">, DwarfRegNum<[283]>;
def R284 : AMDILReg<284, "r284">, DwarfRegNum<[284]>;
def R285 : AMDILReg<285, "r285">, DwarfRegNum<[285]>;
def R286 : AMDILReg<286, "r286">, DwarfRegNum<[286]>;
def R287 : AMDILReg<287, "r287">, DwarfRegNum<[287]>;
def R288 : AMDILReg<288, "r288">, DwarfRegNum<[288]>;
def R289 : AMDILReg<289, "r289">, DwarfRegNum<[289]>;
def R290 : AMDILReg<290, "r290">, DwarfRegNum<[290]>;
def R291 : AMDILReg<291, "r291">, DwarfRegNum<[291]>;
def R292 : AMDILReg<292, "r292">, DwarfRegNum<[292]>;
def R293 : AMDILReg<293, "r293">, DwarfRegNum<[293]>;
def R294 : AMDILReg<294, "r294">, DwarfRegNum<[294]>;
def R295 : AMDILReg<295, "r295">, DwarfRegNum<[295]>;
def R296 : AMDILReg<296, "r296">, DwarfRegNum<[296]>;
def R297 : AMDILReg<297, "r297">, DwarfRegNum<[297]>;
def R298 : AMDILReg<298, "r298">, DwarfRegNum<[298]>;
def R299 : AMDILReg<299, "r299">, DwarfRegNum<[299]>;
def R300 : AMDILReg<300, "r300">, DwarfRegNum<[300]>;
def R301 : AMDILReg<301, "r301">, DwarfRegNum<[301]>;
def R302 : AMDILReg<302, "r302">, DwarfRegNum<[302]>;
def R303 : AMDILReg<303, "r303">, DwarfRegNum<[303]>;
def R304 : AMDILReg<304, "r304">, DwarfRegNum<[304]>;
def R305 : AMDILReg<305, "r305">, DwarfRegNum<[305]>;
def R306 : AMDILReg<306, "r306">, DwarfRegNum<[306]>;
def R307 : AMDILReg<307, "r307">, DwarfRegNum<[307]>;
def R308 : AMDILReg<308, "r308">, DwarfRegNum<[308]>;
def R309 : AMDILReg<309, "r309">, DwarfRegNum<[309]>;
def R310 : AMDILReg<310, "r310">, DwarfRegNum<[310]>;
def R311 : AMDILReg<311, "r311">, DwarfRegNum<[311]>;
def R312 : AMDILReg<312, "r312">, DwarfRegNum<[312]>;
def R313 : AMDILReg<313, "r313">, DwarfRegNum<[313]>;
def R314 : AMDILReg<314, "r314">, DwarfRegNum<[314]>;
def R315 : AMDILReg<315, "r315">, DwarfRegNum<[315]>;
def R316 : AMDILReg<316, "r316">, DwarfRegNum<[316]>;
def R317 : AMDILReg<317, "r317">, DwarfRegNum<[317]>;
def R318 : AMDILReg<318, "r318">, DwarfRegNum<[318]>;
def R319 : AMDILReg<319, "r319">, DwarfRegNum<[319]>;
def R320 : AMDILReg<320, "r320">, DwarfRegNum<[320]>;
def R321 : AMDILReg<321, "r321">, DwarfRegNum<[321]>;
def R322 : AMDILReg<322, "r322">, DwarfRegNum<[322]>;
def R323 : AMDILReg<323, "r323">, DwarfRegNum<[323]>;
def R324 : AMDILReg<324, "r324">, DwarfRegNum<[324]>;
def R325 : AMDILReg<325, "r325">, DwarfRegNum<[325]>;
def R326 : AMDILReg<326, "r326">, DwarfRegNum<[326]>;
def R327 : AMDILReg<327, "r327">, DwarfRegNum<[327]>;
def R328 : AMDILReg<328, "r328">, DwarfRegNum<[328]>;
def R329 : AMDILReg<329, "r329">, DwarfRegNum<[329]>;
def R330 : AMDILReg<330, "r330">, DwarfRegNum<[330]>;
def R331 : AMDILReg<331, "r331">, DwarfRegNum<[331]>;
def R332 : AMDILReg<332, "r332">, DwarfRegNum<[332]>;
def R333 : AMDILReg<333, "r333">, DwarfRegNum<[333]>;
def R334 : AMDILReg<334, "r334">, DwarfRegNum<[334]>;
def R335 : AMDILReg<335, "r335">, DwarfRegNum<[335]>;
def R336 : AMDILReg<336, "r336">, DwarfRegNum<[336]>;
def R337 : AMDILReg<337, "r337">, DwarfRegNum<[337]>;
def R338 : AMDILReg<338, "r338">, DwarfRegNum<[338]>;
def R339 : AMDILReg<339, "r339">, DwarfRegNum<[339]>;
def R340 : AMDILReg<340, "r340">, DwarfRegNum<[340]>;
def R341 : AMDILReg<341, "r341">, DwarfRegNum<[341]>;
def R342 : AMDILReg<342, "r342">, DwarfRegNum<[342]>;
def R343 : AMDILReg<343, "r343">, DwarfRegNum<[343]>;
def R344 : AMDILReg<344, "r344">, DwarfRegNum<[344]>;
def R345 : AMDILReg<345, "r345">, DwarfRegNum<[345]>;
def R346 : AMDILReg<346, "r346">, DwarfRegNum<[346]>;
def R347 : AMDILReg<347, "r347">, DwarfRegNum<[347]>;
def R348 : AMDILReg<348, "r348">, DwarfRegNum<[348]>;
def R349 : AMDILReg<349, "r349">, DwarfRegNum<[349]>;
def R350 : AMDILReg<350, "r350">, DwarfRegNum<[350]>;
def R351 : AMDILReg<351, "r351">, DwarfRegNum<[351]>;
def R352 : AMDILReg<352, "r352">, DwarfRegNum<[352]>;
def R353 : AMDILReg<353, "r353">, DwarfRegNum<[353]>;
def R354 : AMDILReg<354, "r354">, DwarfRegNum<[354]>;
def R355 : AMDILReg<355, "r355">, DwarfRegNum<[355]>;
def R356 : AMDILReg<356, "r356">, DwarfRegNum<[356]>;
def R357 : AMDILReg<357, "r357">, DwarfRegNum<[357]>;
def R358 : AMDILReg<358, "r358">, DwarfRegNum<[358]>;
def R359 : AMDILReg<359, "r359">, DwarfRegNum<[359]>;
def R360 : AMDILReg<360, "r360">, DwarfRegNum<[360]>;
def R361 : AMDILReg<361, "r361">, DwarfRegNum<[361]>;
def R362 : AMDILReg<362, "r362">, DwarfRegNum<[362]>;
def R363 : AMDILReg<363, "r363">, DwarfRegNum<[363]>;
def R364 : AMDILReg<364, "r364">, DwarfRegNum<[364]>;
def R365 : AMDILReg<365, "r365">, DwarfRegNum<[365]>;
def R366 : AMDILReg<366, "r366">, DwarfRegNum<[366]>;
def R367 : AMDILReg<367, "r367">, DwarfRegNum<[367]>;
def R368 : AMDILReg<368, "r368">, DwarfRegNum<[368]>;
def R369 : AMDILReg<369, "r369">, DwarfRegNum<[369]>;
def R370 : AMDILReg<370, "r370">, DwarfRegNum<[370]>;
def R371 : AMDILReg<371, "r371">, DwarfRegNum<[371]>;
def R372 : AMDILReg<372, "r372">, DwarfRegNum<[372]>;
def R373 : AMDILReg<373, "r373">, DwarfRegNum<[373]>;
def R374 : AMDILReg<374, "r374">, DwarfRegNum<[374]>;
def R375 : AMDILReg<375, "r375">, DwarfRegNum<[375]>;
def R376 : AMDILReg<376, "r376">, DwarfRegNum<[376]>;
def R377 : AMDILReg<377, "r377">, DwarfRegNum<[377]>;
def R378 : AMDILReg<378, "r378">, DwarfRegNum<[378]>;
def R379 : AMDILReg<379, "r379">, DwarfRegNum<[379]>;
def R380 : AMDILReg<380, "r380">, DwarfRegNum<[380]>;
def R381 : AMDILReg<381, "r381">, DwarfRegNum<[381]>;
def R382 : AMDILReg<382, "r382">, DwarfRegNum<[382]>;
def R383 : AMDILReg<383, "r383">, DwarfRegNum<[383]>;
def R384 : AMDILReg<384, "r384">, DwarfRegNum<[384]>;
def R385 : AMDILReg<385, "r385">, DwarfRegNum<[385]>;
def R386 : AMDILReg<386, "r386">, DwarfRegNum<[386]>;
def R387 : AMDILReg<387, "r387">, DwarfRegNum<[387]>;
def R388 : AMDILReg<388, "r388">, DwarfRegNum<[388]>;
def R389 : AMDILReg<389, "r389">, DwarfRegNum<[389]>;
def R390 : AMDILReg<390, "r390">, DwarfRegNum<[390]>;
def R391 : AMDILReg<391, "r391">, DwarfRegNum<[391]>;
def R392 : AMDILReg<392, "r392">, DwarfRegNum<[392]>;
def R393 : AMDILReg<393, "r393">, DwarfRegNum<[393]>;
def R394 : AMDILReg<394, "r394">, DwarfRegNum<[394]>;
def R395 : AMDILReg<395, "r395">, DwarfRegNum<[395]>;
def R396 : AMDILReg<396, "r396">, DwarfRegNum<[396]>;
def R397 : AMDILReg<397, "r397">, DwarfRegNum<[397]>;
def R398 : AMDILReg<398, "r398">, DwarfRegNum<[398]>;
def R399 : AMDILReg<399, "r399">, DwarfRegNum<[399]>;
def R400 : AMDILReg<400, "r400">, DwarfRegNum<[400]>;
def R401 : AMDILReg<401, "r401">, DwarfRegNum<[401]>;
def R402 : AMDILReg<402, "r402">, DwarfRegNum<[402]>;
def R403 : AMDILReg<403, "r403">, DwarfRegNum<[403]>;
def R404 : AMDILReg<404, "r404">, DwarfRegNum<[404]>;
def R405 : AMDILReg<405, "r405">, DwarfRegNum<[405]>;
def R406 : AMDILReg<406, "r406">, DwarfRegNum<[406]>;
def R407 : AMDILReg<407, "r407">, DwarfRegNum<[407]>;
def R408 : AMDILReg<408, "r408">, DwarfRegNum<[408]>;
def R409 : AMDILReg<409, "r409">, DwarfRegNum<[409]>;
def R410 : AMDILReg<410, "r410">, DwarfRegNum<[410]>;
def R411 : AMDILReg<411, "r411">, DwarfRegNum<[411]>;
def R412 : AMDILReg<412, "r412">, DwarfRegNum<[412]>;
def R413 : AMDILReg<413, "r413">, DwarfRegNum<[413]>;
def R414 : AMDILReg<414, "r414">, DwarfRegNum<[414]>;
def R415 : AMDILReg<415, "r415">, DwarfRegNum<[415]>;
def R416 : AMDILReg<416, "r416">, DwarfRegNum<[416]>;
def R417 : AMDILReg<417, "r417">, DwarfRegNum<[417]>;
def R418 : AMDILReg<418, "r418">, DwarfRegNum<[418]>;
def R419 : AMDILReg<419, "r419">, DwarfRegNum<[419]>;
def R420 : AMDILReg<420, "r420">, DwarfRegNum<[420]>;
def R421 : AMDILReg<421, "r421">, DwarfRegNum<[421]>;
def R422 : AMDILReg<422, "r422">, DwarfRegNum<[422]>;
def R423 : AMDILReg<423, "r423">, DwarfRegNum<[423]>;
def R424 : AMDILReg<424, "r424">, DwarfRegNum<[424]>;
def R425 : AMDILReg<425, "r425">, DwarfRegNum<[425]>;
def R426 : AMDILReg<426, "r426">, DwarfRegNum<[426]>;
def R427 : AMDILReg<427, "r427">, DwarfRegNum<[427]>;
def R428 : AMDILReg<428, "r428">, DwarfRegNum<[428]>;
def R429 : AMDILReg<429, "r429">, DwarfRegNum<[429]>;
def R430 : AMDILReg<430, "r430">, DwarfRegNum<[430]>;
def R431 : AMDILReg<431, "r431">, DwarfRegNum<[431]>;
def R432 : AMDILReg<432, "r432">, DwarfRegNum<[432]>;
def R433 : AMDILReg<433, "r433">, DwarfRegNum<[433]>;
def R434 : AMDILReg<434, "r434">, DwarfRegNum<[434]>;
def R435 : AMDILReg<435, "r435">, DwarfRegNum<[435]>;
def R436 : AMDILReg<436, "r436">, DwarfRegNum<[436]>;
def R437 : AMDILReg<437, "r437">, DwarfRegNum<[437]>;
def R438 : AMDILReg<438, "r438">, DwarfRegNum<[438]>;
def R439 : AMDILReg<439, "r439">, DwarfRegNum<[439]>;
def R440 : AMDILReg<440, "r440">, DwarfRegNum<[440]>;
def R441 : AMDILReg<441, "r441">, DwarfRegNum<[441]>;
def R442 : AMDILReg<442, "r442">, DwarfRegNum<[442]>;
def R443 : AMDILReg<443, "r443">, DwarfRegNum<[443]>;
def R444 : AMDILReg<444, "r444">, DwarfRegNum<[444]>;
def R445 : AMDILReg<445, "r445">, DwarfRegNum<[445]>;
def R446 : AMDILReg<446, "r446">, DwarfRegNum<[446]>;
def R447 : AMDILReg<447, "r447">, DwarfRegNum<[447]>;
def R448 : AMDILReg<448, "r448">, DwarfRegNum<[448]>;
def R449 : AMDILReg<449, "r449">, DwarfRegNum<[449]>;
def R450 : AMDILReg<450, "r450">, DwarfRegNum<[450]>;
def R451 : AMDILReg<451, "r451">, DwarfRegNum<[451]>;
def R452 : AMDILReg<452, "r452">, DwarfRegNum<[452]>;
def R453 : AMDILReg<453, "r453">, DwarfRegNum<[453]>;
def R454 : AMDILReg<454, "r454">, DwarfRegNum<[454]>;
def R455 : AMDILReg<455, "r455">, DwarfRegNum<[455]>;
def R456 : AMDILReg<456, "r456">, DwarfRegNum<[456]>;
def R457 : AMDILReg<457, "r457">, DwarfRegNum<[457]>;
def R458 : AMDILReg<458, "r458">, DwarfRegNum<[458]>;
def R459 : AMDILReg<459, "r459">, DwarfRegNum<[459]>;
def R460 : AMDILReg<460, "r460">, DwarfRegNum<[460]>;
def R461 : AMDILReg<461, "r461">, DwarfRegNum<[461]>;
def R462 : AMDILReg<462, "r462">, DwarfRegNum<[462]>;
def R463 : AMDILReg<463, "r463">, DwarfRegNum<[463]>;
def R464 : AMDILReg<464, "r464">, DwarfRegNum<[464]>;
def R465 : AMDILReg<465, "r465">, DwarfRegNum<[465]>;
def R466 : AMDILReg<466, "r466">, DwarfRegNum<[466]>;
def R467 : AMDILReg<467, "r467">, DwarfRegNum<[467]>;
def R468 : AMDILReg<468, "r468">, DwarfRegNum<[468]>;
def R469 : AMDILReg<469, "r469">, DwarfRegNum<[469]>;
def R470 : AMDILReg<470, "r470">, DwarfRegNum<[470]>;
def R471 : AMDILReg<471, "r471">, DwarfRegNum<[471]>;
def R472 : AMDILReg<472, "r472">, DwarfRegNum<[472]>;
def R473 : AMDILReg<473, "r473">, DwarfRegNum<[473]>;
def R474 : AMDILReg<474, "r474">, DwarfRegNum<[474]>;
def R475 : AMDILReg<475, "r475">, DwarfRegNum<[475]>;
def R476 : AMDILReg<476, "r476">, DwarfRegNum<[476]>;
def R477 : AMDILReg<477, "r477">, DwarfRegNum<[477]>;
def R478 : AMDILReg<478, "r478">, DwarfRegNum<[478]>;
def R479 : AMDILReg<479, "r479">, DwarfRegNum<[479]>;
def R480 : AMDILReg<480, "r480">, DwarfRegNum<[480]>;
def R481 : AMDILReg<481, "r481">, DwarfRegNum<[481]>;
def R482 : AMDILReg<482, "r482">, DwarfRegNum<[482]>;
def R483 : AMDILReg<483, "r483">, DwarfRegNum<[483]>;
def R484 : AMDILReg<484, "r484">, DwarfRegNum<[484]>;
def R485 : AMDILReg<485, "r485">, DwarfRegNum<[485]>;
def R486 : AMDILReg<486, "r486">, DwarfRegNum<[486]>;
def R487 : AMDILReg<487, "r487">, DwarfRegNum<[487]>;
def R488 : AMDILReg<488, "r488">, DwarfRegNum<[488]>;
def R489 : AMDILReg<489, "r489">, DwarfRegNum<[489]>;
def R490 : AMDILReg<490, "r490">, DwarfRegNum<[490]>;
def R491 : AMDILReg<491, "r491">, DwarfRegNum<[491]>;
def R492 : AMDILReg<492, "r492">, DwarfRegNum<[492]>;
def R493 : AMDILReg<493, "r493">, DwarfRegNum<[493]>;
def R494 : AMDILReg<494, "r494">, DwarfRegNum<[494]>;
def R495 : AMDILReg<495, "r495">, DwarfRegNum<[495]>;
def R496 : AMDILReg<496, "r496">, DwarfRegNum<[496]>;
def R497 : AMDILReg<497, "r497">, DwarfRegNum<[497]>;
def R498 : AMDILReg<498, "r498">, DwarfRegNum<[498]>;
def R499 : AMDILReg<499, "r499">, DwarfRegNum<[499]>;
def R500 : AMDILReg<500, "r500">, DwarfRegNum<[500]>;
def R501 : AMDILReg<501, "r501">, DwarfRegNum<[501]>;
def R502 : AMDILReg<502, "r502">, DwarfRegNum<[502]>;
def R503 : AMDILReg<503, "r503">, DwarfRegNum<[503]>;
def R504 : AMDILReg<504, "r504">, DwarfRegNum<[504]>;
def R505 : AMDILReg<505, "r505">, DwarfRegNum<[505]>;
def R506 : AMDILReg<506, "r506">, DwarfRegNum<[506]>;
def R507 : AMDILReg<507, "r507">, DwarfRegNum<[507]>;
def R508 : AMDILReg<508, "r508">, DwarfRegNum<[508]>;
def R509 : AMDILReg<509, "r509">, DwarfRegNum<[509]>;
def R510 : AMDILReg<510, "r510">, DwarfRegNum<[510]>;
def R511 : AMDILReg<511, "r511">, DwarfRegNum<[511]>;
def R512 : AMDILReg<512, "r512">, DwarfRegNum<[512]>;
def R513 : AMDILReg<513, "r513">, DwarfRegNum<[513]>;
def R514 : AMDILReg<514, "r514">, DwarfRegNum<[514]>;
def R515 : AMDILReg<515, "r515">, DwarfRegNum<[515]>;
def R516 : AMDILReg<516, "r516">, DwarfRegNum<[516]>;
def R517 : AMDILReg<517, "r517">, DwarfRegNum<[517]>;
def R518 : AMDILReg<518, "r518">, DwarfRegNum<[518]>;
def R519 : AMDILReg<519, "r519">, DwarfRegNum<[519]>;
def R520 : AMDILReg<520, "r520">, DwarfRegNum<[520]>;
def R521 : AMDILReg<521, "r521">, DwarfRegNum<[521]>;
def R522 : AMDILReg<522, "r522">, DwarfRegNum<[522]>;
def R523 : AMDILReg<523, "r523">, DwarfRegNum<[523]>;
def R524 : AMDILReg<524, "r524">, DwarfRegNum<[524]>;
def R525 : AMDILReg<525, "r525">, DwarfRegNum<[525]>;
def R526 : AMDILReg<526, "r526">, DwarfRegNum<[526]>;
def R527 : AMDILReg<527, "r527">, DwarfRegNum<[527]>;
def R528 : AMDILReg<528, "r528">, DwarfRegNum<[528]>;
def R529 : AMDILReg<529, "r529">, DwarfRegNum<[529]>;
def R530 : AMDILReg<530, "r530">, DwarfRegNum<[530]>;
def R531 : AMDILReg<531, "r531">, DwarfRegNum<[531]>;
def R532 : AMDILReg<532, "r532">, DwarfRegNum<[532]>;
def R533 : AMDILReg<533, "r533">, DwarfRegNum<[533]>;
def R534 : AMDILReg<534, "r534">, DwarfRegNum<[534]>;
def R535 : AMDILReg<535, "r535">, DwarfRegNum<[535]>;
def R536 : AMDILReg<536, "r536">, DwarfRegNum<[536]>;
def R537 : AMDILReg<537, "r537">, DwarfRegNum<[537]>;
def R538 : AMDILReg<538, "r538">, DwarfRegNum<[538]>;
def R539 : AMDILReg<539, "r539">, DwarfRegNum<[539]>;
def R540 : AMDILReg<540, "r540">, DwarfRegNum<[540]>;
def R541 : AMDILReg<541, "r541">, DwarfRegNum<[541]>;
def R542 : AMDILReg<542, "r542">, DwarfRegNum<[542]>;
def R543 : AMDILReg<543, "r543">, DwarfRegNum<[543]>;
def R544 : AMDILReg<544, "r544">, DwarfRegNum<[544]>;
def R545 : AMDILReg<545, "r545">, DwarfRegNum<[545]>;
def R546 : AMDILReg<546, "r546">, DwarfRegNum<[546]>;
def R547 : AMDILReg<547, "r547">, DwarfRegNum<[547]>;
def R548 : AMDILReg<548, "r548">, DwarfRegNum<[548]>;
def R549 : AMDILReg<549, "r549">, DwarfRegNum<[549]>;
def R550 : AMDILReg<550, "r550">, DwarfRegNum<[550]>;
def R551 : AMDILReg<551, "r551">, DwarfRegNum<[551]>;
def R552 : AMDILReg<552, "r552">, DwarfRegNum<[552]>;
def R553 : AMDILReg<553, "r553">, DwarfRegNum<[553]>;
def R554 : AMDILReg<554, "r554">, DwarfRegNum<[554]>;
def R555 : AMDILReg<555, "r555">, DwarfRegNum<[555]>;
def R556 : AMDILReg<556, "r556">, DwarfRegNum<[556]>;
def R557 : AMDILReg<557, "r557">, DwarfRegNum<[557]>;
def R558 : AMDILReg<558, "r558">, DwarfRegNum<[558]>;
def R559 : AMDILReg<559, "r559">, DwarfRegNum<[559]>;
def R560 : AMDILReg<560, "r560">, DwarfRegNum<[560]>;
def R561 : AMDILReg<561, "r561">, DwarfRegNum<[561]>;
def R562 : AMDILReg<562, "r562">, DwarfRegNum<[562]>;
def R563 : AMDILReg<563, "r563">, DwarfRegNum<[563]>;
def R564 : AMDILReg<564, "r564">, DwarfRegNum<[564]>;
def R565 : AMDILReg<565, "r565">, DwarfRegNum<[565]>;
def R566 : AMDILReg<566, "r566">, DwarfRegNum<[566]>;
def R567 : AMDILReg<567, "r567">, DwarfRegNum<[567]>;
def R568 : AMDILReg<568, "r568">, DwarfRegNum<[568]>;
def R569 : AMDILReg<569, "r569">, DwarfRegNum<[569]>;
def R570 : AMDILReg<570, "r570">, DwarfRegNum<[570]>;
def R571 : AMDILReg<571, "r571">, DwarfRegNum<[571]>;
def R572 : AMDILReg<572, "r572">, DwarfRegNum<[572]>;
def R573 : AMDILReg<573, "r573">, DwarfRegNum<[573]>;
def R574 : AMDILReg<574, "r574">, DwarfRegNum<[574]>;
def R575 : AMDILReg<575, "r575">, DwarfRegNum<[575]>;
def R576 : AMDILReg<576, "r576">, DwarfRegNum<[576]>;
def R577 : AMDILReg<577, "r577">, DwarfRegNum<[577]>;
def R578 : AMDILReg<578, "r578">, DwarfRegNum<[578]>;
def R579 : AMDILReg<579, "r579">, DwarfRegNum<[579]>;
def R580 : AMDILReg<580, "r580">, DwarfRegNum<[580]>;
def R581 : AMDILReg<581, "r581">, DwarfRegNum<[581]>;
def R582 : AMDILReg<582, "r582">, DwarfRegNum<[582]>;
def R583 : AMDILReg<583, "r583">, DwarfRegNum<[583]>;
def R584 : AMDILReg<584, "r584">, DwarfRegNum<[584]>;
def R585 : AMDILReg<585, "r585">, DwarfRegNum<[585]>;
def R586 : AMDILReg<586, "r586">, DwarfRegNum<[586]>;
def R587 : AMDILReg<587, "r587">, DwarfRegNum<[587]>;
def R588 : AMDILReg<588, "r588">, DwarfRegNum<[588]>;
def R589 : AMDILReg<589, "r589">, DwarfRegNum<[589]>;
def R590 : AMDILReg<590, "r590">, DwarfRegNum<[590]>;
def R591 : AMDILReg<591, "r591">, DwarfRegNum<[591]>;
def R592 : AMDILReg<592, "r592">, DwarfRegNum<[592]>;
def R593 : AMDILReg<593, "r593">, DwarfRegNum<[593]>;
def R594 : AMDILReg<594, "r594">, DwarfRegNum<[594]>;
def R595 : AMDILReg<595, "r595">, DwarfRegNum<[595]>;
def R596 : AMDILReg<596, "r596">, DwarfRegNum<[596]>;
def R597 : AMDILReg<597, "r597">, DwarfRegNum<[597]>;
def R598 : AMDILReg<598, "r598">, DwarfRegNum<[598]>;
def R599 : AMDILReg<599, "r599">, DwarfRegNum<[599]>;
def R600 : AMDILReg<600, "r600">, DwarfRegNum<[600]>;
def R601 : AMDILReg<601, "r601">, DwarfRegNum<[601]>;
def R602 : AMDILReg<602, "r602">, DwarfRegNum<[602]>;
def R603 : AMDILReg<603, "r603">, DwarfRegNum<[603]>;
def R604 : AMDILReg<604, "r604">, DwarfRegNum<[604]>;
def R605 : AMDILReg<605, "r605">, DwarfRegNum<[605]>;
def R606 : AMDILReg<606, "r606">, DwarfRegNum<[606]>;
def R607 : AMDILReg<607, "r607">, DwarfRegNum<[607]>;
def R608 : AMDILReg<608, "r608">, DwarfRegNum<[608]>;
def R609 : AMDILReg<609, "r609">, DwarfRegNum<[609]>;
def R610 : AMDILReg<610, "r610">, DwarfRegNum<[610]>;
def R611 : AMDILReg<611, "r611">, DwarfRegNum<[611]>;
def R612 : AMDILReg<612, "r612">, DwarfRegNum<[612]>;
def R613 : AMDILReg<613, "r613">, DwarfRegNum<[613]>;
def R614 : AMDILReg<614, "r614">, DwarfRegNum<[614]>;
def R615 : AMDILReg<615, "r615">, DwarfRegNum<[615]>;
def R616 : AMDILReg<616, "r616">, DwarfRegNum<[616]>;
def R617 : AMDILReg<617, "r617">, DwarfRegNum<[617]>;
def R618 : AMDILReg<618, "r618">, DwarfRegNum<[618]>;
def R619 : AMDILReg<619, "r619">, DwarfRegNum<[619]>;
def R620 : AMDILReg<620, "r620">, DwarfRegNum<[620]>;
def R621 : AMDILReg<621, "r621">, DwarfRegNum<[621]>;
def R622 : AMDILReg<622, "r622">, DwarfRegNum<[622]>;
def R623 : AMDILReg<623, "r623">, DwarfRegNum<[623]>;
def R624 : AMDILReg<624, "r624">, DwarfRegNum<[624]>;
def R625 : AMDILReg<625, "r625">, DwarfRegNum<[625]>;
def R626 : AMDILReg<626, "r626">, DwarfRegNum<[626]>;
def R627 : AMDILReg<627, "r627">, DwarfRegNum<[627]>;
def R628 : AMDILReg<628, "r628">, DwarfRegNum<[628]>;
def R629 : AMDILReg<629, "r629">, DwarfRegNum<[629]>;
def R630 : AMDILReg<630, "r630">, DwarfRegNum<[630]>;
def R631 : AMDILReg<631, "r631">, DwarfRegNum<[631]>;
def R632 : AMDILReg<632, "r632">, DwarfRegNum<[632]>;
def R633 : AMDILReg<633, "r633">, DwarfRegNum<[633]>;
def R634 : AMDILReg<634, "r634">, DwarfRegNum<[634]>;
def R635 : AMDILReg<635, "r635">, DwarfRegNum<[635]>;
def R636 : AMDILReg<636, "r636">, DwarfRegNum<[636]>;
def R637 : AMDILReg<637, "r637">, DwarfRegNum<[637]>;
def R638 : AMDILReg<638, "r638">, DwarfRegNum<[638]>;
def R639 : AMDILReg<639, "r639">, DwarfRegNum<[639]>;
def R640 : AMDILReg<640, "r640">, DwarfRegNum<[640]>;
def R641 : AMDILReg<641, "r641">, DwarfRegNum<[641]>;
def R642 : AMDILReg<642, "r642">, DwarfRegNum<[642]>;
def R643 : AMDILReg<643, "r643">, DwarfRegNum<[643]>;
def R644 : AMDILReg<644, "r644">, DwarfRegNum<[644]>;
def R645 : AMDILReg<645, "r645">, DwarfRegNum<[645]>;
def R646 : AMDILReg<646, "r646">, DwarfRegNum<[646]>;
def R647 : AMDILReg<647, "r647">, DwarfRegNum<[647]>;
def R648 : AMDILReg<648, "r648">, DwarfRegNum<[648]>;
def R649 : AMDILReg<649, "r649">, DwarfRegNum<[649]>;
def R650 : AMDILReg<650, "r650">, DwarfRegNum<[650]>;
def R651 : AMDILReg<651, "r651">, DwarfRegNum<[651]>;
def R652 : AMDILReg<652, "r652">, DwarfRegNum<[652]>;
def R653 : AMDILReg<653, "r653">, DwarfRegNum<[653]>;
def R654 : AMDILReg<654, "r654">, DwarfRegNum<[654]>;
def R655 : AMDILReg<655, "r655">, DwarfRegNum<[655]>;
def R656 : AMDILReg<656, "r656">, DwarfRegNum<[656]>;
def R657 : AMDILReg<657, "r657">, DwarfRegNum<[657]>;
def R658 : AMDILReg<658, "r658">, DwarfRegNum<[658]>;
def R659 : AMDILReg<659, "r659">, DwarfRegNum<[659]>;
def R660 : AMDILReg<660, "r660">, DwarfRegNum<[660]>;
def R661 : AMDILReg<661, "r661">, DwarfRegNum<[661]>;
def R662 : AMDILReg<662, "r662">, DwarfRegNum<[662]>;
def R663 : AMDILReg<663, "r663">, DwarfRegNum<[663]>;
def R664 : AMDILReg<664, "r664">, DwarfRegNum<[664]>;
def R665 : AMDILReg<665, "r665">, DwarfRegNum<[665]>;
def R666 : AMDILReg<666, "r666">, DwarfRegNum<[666]>;
def R667 : AMDILReg<667, "r667">, DwarfRegNum<[667]>;
def R668 : AMDILReg<668, "r668">, DwarfRegNum<[668]>;
def R669 : AMDILReg<669, "r669">, DwarfRegNum<[669]>;
def R670 : AMDILReg<670, "r670">, DwarfRegNum<[670]>;
def R671 : AMDILReg<671, "r671">, DwarfRegNum<[671]>;
def R672 : AMDILReg<672, "r672">, DwarfRegNum<[672]>;
def R673 : AMDILReg<673, "r673">, DwarfRegNum<[673]>;
def R674 : AMDILReg<674, "r674">, DwarfRegNum<[674]>;
def R675 : AMDILReg<675, "r675">, DwarfRegNum<[675]>;
def R676 : AMDILReg<676, "r676">, DwarfRegNum<[676]>;
def R677 : AMDILReg<677, "r677">, DwarfRegNum<[677]>;
def R678 : AMDILReg<678, "r678">, DwarfRegNum<[678]>;
def R679 : AMDILReg<679, "r679">, DwarfRegNum<[679]>;
def R680 : AMDILReg<680, "r680">, DwarfRegNum<[680]>;
def R681 : AMDILReg<681, "r681">, DwarfRegNum<[681]>;
def R682 : AMDILReg<682, "r682">, DwarfRegNum<[682]>;
def R683 : AMDILReg<683, "r683">, DwarfRegNum<[683]>;
def R684 : AMDILReg<684, "r684">, DwarfRegNum<[684]>;
def R685 : AMDILReg<685, "r685">, DwarfRegNum<[685]>;
def R686 : AMDILReg<686, "r686">, DwarfRegNum<[686]>;
def R687 : AMDILReg<687, "r687">, DwarfRegNum<[687]>;
def R688 : AMDILReg<688, "r688">, DwarfRegNum<[688]>;
def R689 : AMDILReg<689, "r689">, DwarfRegNum<[689]>;
def R690 : AMDILReg<690, "r690">, DwarfRegNum<[690]>;
def R691 : AMDILReg<691, "r691">, DwarfRegNum<[691]>;
def R692 : AMDILReg<692, "r692">, DwarfRegNum<[692]>;
def R693 : AMDILReg<693, "r693">, DwarfRegNum<[693]>;
def R694 : AMDILReg<694, "r694">, DwarfRegNum<[694]>;
def R695 : AMDILReg<695, "r695">, DwarfRegNum<[695]>;
def R696 : AMDILReg<696, "r696">, DwarfRegNum<[696]>;
def R697 : AMDILReg<697, "r697">, DwarfRegNum<[697]>;
def R698 : AMDILReg<698, "r698">, DwarfRegNum<[698]>;
def R699 : AMDILReg<699, "r699">, DwarfRegNum<[699]>;
def R700 : AMDILReg<700, "r700">, DwarfRegNum<[700]>;
def R701 : AMDILReg<701, "r701">, DwarfRegNum<[701]>;
def R702 : AMDILReg<702, "r702">, DwarfRegNum<[702]>;
def R703 : AMDILReg<703, "r703">, DwarfRegNum<[703]>;
def R704 : AMDILReg<704, "r704">, DwarfRegNum<[704]>;
def R705 : AMDILReg<705, "r705">, DwarfRegNum<[705]>;
def R706 : AMDILReg<706, "r706">, DwarfRegNum<[706]>;
def R707 : AMDILReg<707, "r707">, DwarfRegNum<[707]>;
def R708 : AMDILReg<708, "r708">, DwarfRegNum<[708]>;
def R709 : AMDILReg<709, "r709">, DwarfRegNum<[709]>;
def R710 : AMDILReg<710, "r710">, DwarfRegNum<[710]>;
def R711 : AMDILReg<711, "r711">, DwarfRegNum<[711]>;
def R712 : AMDILReg<712, "r712">, DwarfRegNum<[712]>;
def R713 : AMDILReg<713, "r713">, DwarfRegNum<[713]>;
def R714 : AMDILReg<714, "r714">, DwarfRegNum<[714]>;
def R715 : AMDILReg<715, "r715">, DwarfRegNum<[715]>;
def R716 : AMDILReg<716, "r716">, DwarfRegNum<[716]>;
def R717 : AMDILReg<717, "r717">, DwarfRegNum<[717]>;
def R718 : AMDILReg<718, "r718">, DwarfRegNum<[718]>;
def R719 : AMDILReg<719, "r719">, DwarfRegNum<[719]>;
def R720 : AMDILReg<720, "r720">, DwarfRegNum<[720]>;
def R721 : AMDILReg<721, "r721">, DwarfRegNum<[721]>;
def R722 : AMDILReg<722, "r722">, DwarfRegNum<[722]>;
def R723 : AMDILReg<723, "r723">, DwarfRegNum<[723]>;
def R724 : AMDILReg<724, "r724">, DwarfRegNum<[724]>;
def R725 : AMDILReg<725, "r725">, DwarfRegNum<[725]>;
def R726 : AMDILReg<726, "r726">, DwarfRegNum<[726]>;
def R727 : AMDILReg<727, "r727">, DwarfRegNum<[727]>;
def R728 : AMDILReg<728, "r728">, DwarfRegNum<[728]>;
def R729 : AMDILReg<729, "r729">, DwarfRegNum<[729]>;
def R730 : AMDILReg<730, "r730">, DwarfRegNum<[730]>;
def R731 : AMDILReg<731, "r731">, DwarfRegNum<[731]>;
def R732 : AMDILReg<732, "r732">, DwarfRegNum<[732]>;
def R733 : AMDILReg<733, "r733">, DwarfRegNum<[733]>;
def R734 : AMDILReg<734, "r734">, DwarfRegNum<[734]>;
def R735 : AMDILReg<735, "r735">, DwarfRegNum<[735]>;
def R736 : AMDILReg<736, "r736">, DwarfRegNum<[736]>;
def R737 : AMDILReg<737, "r737">, DwarfRegNum<[737]>;
def R738 : AMDILReg<738, "r738">, DwarfRegNum<[738]>;
def R739 : AMDILReg<739, "r739">, DwarfRegNum<[739]>;
def R740 : AMDILReg<740, "r740">, DwarfRegNum<[740]>;
def R741 : AMDILReg<741, "r741">, DwarfRegNum<[741]>;
def R742 : AMDILReg<742, "r742">, DwarfRegNum<[742]>;
def R743 : AMDILReg<743, "r743">, DwarfRegNum<[743]>;
def R744 : AMDILReg<744, "r744">, DwarfRegNum<[744]>;
def R745 : AMDILReg<745, "r745">, DwarfRegNum<[745]>;
def R746 : AMDILReg<746, "r746">, DwarfRegNum<[746]>;
def R747 : AMDILReg<747, "r747">, DwarfRegNum<[747]>;
def R748 : AMDILReg<748, "r748">, DwarfRegNum<[748]>;
def R749 : AMDILReg<749, "r749">, DwarfRegNum<[749]>;
def R750 : AMDILReg<750, "r750">, DwarfRegNum<[750]>;
def R751 : AMDILReg<751, "r751">, DwarfRegNum<[751]>;
def R752 : AMDILReg<752, "r752">, DwarfRegNum<[752]>;
def R753 : AMDILReg<753, "r753">, DwarfRegNum<[753]>;
def R754 : AMDILReg<754, "r754">, DwarfRegNum<[754]>;
def R755 : AMDILReg<755, "r755">, DwarfRegNum<[755]>;
def R756 : AMDILReg<756, "r756">, DwarfRegNum<[756]>;
def R757 : AMDILReg<757, "r757">, DwarfRegNum<[757]>;
def R758 : AMDILReg<758, "r758">, DwarfRegNum<[758]>;
def R759 : AMDILReg<759, "r759">, DwarfRegNum<[759]>;
def R760 : AMDILReg<760, "r760">, DwarfRegNum<[760]>;
def R761 : AMDILReg<761, "r761">, DwarfRegNum<[761]>;
def R762 : AMDILReg<762, "r762">, DwarfRegNum<[762]>;
def R763 : AMDILReg<763, "r763">, DwarfRegNum<[763]>;
def R764 : AMDILReg<764, "r764">, DwarfRegNum<[764]>;
def R765 : AMDILReg<765, "r765">, DwarfRegNum<[765]>;
def R766 : AMDILReg<766, "r766">, DwarfRegNum<[766]>;
def R767 : AMDILReg<767, "r767">, DwarfRegNum<[767]>;

// All registers between 1000 and 1024 are reserved and cannot be used
// unless commented in this section
// r1021-r1025 are used to dynamically calculate the local/group/thread/region/region_local ID's
// r1020 is used to hold the frame index for local arrays
// r1019 is used to hold the dynamic stack allocation pointer
// r1018 is used as a temporary register for handwritten code
// r1017 is used as a temporary register for handwritten code
// r1016 is used as a temporary register for load/store code
// r1015 is used as a temporary register for data segment offset
// r1014 is used as a temporary register for store code
// r1013 is used as the section data pointer register
// r1012-r1010 and r1001-r1008 are used for temporary I/O registers
// r1009 is used as the frame pointer register
// r999 is used as the mem register.
// r998 is used as the return address register.
//def R1025 : AMDILReg<1025, "r1025">, DwarfRegNum<[1025]>;
//def R1024 : AMDILReg<1024, "r1024">, DwarfRegNum<[1024]>;
//def R1023 : AMDILReg<1023, "r1023">, DwarfRegNum<[1023]>;
//def R1022 : AMDILReg<1022, "r1022">, DwarfRegNum<[1022]>;
//def R1021 : AMDILReg<1021, "r1021">, DwarfRegNum<[1021]>;
//def R1020 : AMDILReg<1020, "r1020">, DwarfRegNum<[1020]>;
def SP : AMDILReg<1019, "r1019">, DwarfRegNum<[1019]>;
def T1 : AMDILReg<1018, "r1018">, DwarfRegNum<[1018]>;
def T2 : AMDILReg<1017, "r1017">, DwarfRegNum<[1017]>;
def T3 : AMDILReg<1016, "r1016">, DwarfRegNum<[1016]>;
def T4 : AMDILReg<1015, "r1015">, DwarfRegNum<[1015]>;
def T5 : AMDILReg<1014, "r1014">, DwarfRegNum<[1014]>;
def SDP : AMDILReg<1013, "r1013">, DwarfRegNum<[1013]>;
def R1012: AMDILReg<1012, "r1012">, DwarfRegNum<[1012]>;
def R1011: AMDILReg<1011, "r1011">, DwarfRegNum<[1011]>;
def R1010: AMDILReg<1010, "r1010">, DwarfRegNum<[1010]>;
def DFP : AMDILReg<1009, "r1009">, DwarfRegNum<[1009]>;
def R1008: AMDILReg<1008, "r1008">, DwarfRegNum<[1008]>;
def R1007: AMDILReg<1007, "r1007">, DwarfRegNum<[1007]>;
def R1006: AMDILReg<1006, "r1006">, DwarfRegNum<[1006]>;
def R1005: AMDILReg<1005, "r1005">, DwarfRegNum<[1005]>;
def R1004: AMDILReg<1004, "r1004">, DwarfRegNum<[1004]>;
def R1003: AMDILReg<1003, "r1003">, DwarfRegNum<[1003]>;
def R1002: AMDILReg<1002, "r1002">, DwarfRegNum<[1002]>;
def R1001: AMDILReg<1001, "r1001">, DwarfRegNum<[1001]>;
def MEM : AMDILReg<999, "mem">, DwarfRegNum<[999]>;
def RA : AMDILReg<998, "r998">, DwarfRegNum<[998]>;
def FP : AMDILReg<997, "r997">, DwarfRegNum<[997]>;
def GPRI8 : RegisterClass<"AMDIL", [i8], 8,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV2I8 : RegisterClass<"AMDIL", [v2i8], 16,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV4I8 : RegisterClass<"AMDIL", [v4i8], 32,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRI16 : RegisterClass<"AMDIL", [i16], 16,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV2I16 : RegisterClass<"AMDIL", [v2i16], 32,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV4I16 : RegisterClass<"AMDIL", [v4i16], 64,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRI32 : RegisterClass<"AMDIL", [i32], 32,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRF32 : RegisterClass<"AMDIL", [f32], 32,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
// For 64 bit integer emulation, the lower 32 bits are in x
// and the upper 32 bits are in y
def GPRI64 : RegisterClass<"AMDIL", [i64], 64,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRF64 : RegisterClass<"AMDIL", [f64], 64,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV4F32 : RegisterClass<"AMDIL", [v4f32], 128,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV4I32 : RegisterClass<"AMDIL", [v4i32], 128,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV2I32 : RegisterClass<"AMDIL", [v2i32], 64,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV2F32 : RegisterClass<"AMDIL", [v2f32], 64,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV2I64 : RegisterClass<"AMDIL", [v2i64], 128,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }
def GPRV2F64 : RegisterClass<"AMDIL", [v2f64], 128,
  (add (sequence "R%u", 1, 767), RA, SP, T1, T2, T3, T4, T5, SDP, R1010, R1011, R1001, R1002, R1003, R1004, R1005, R1006, R1007, R1008, MEM, R1012)>
{
        let AltOrders = [(add (sequence "R%u", 1, 767))];
        let AltOrderSelect = [{
          return 1;
        }];
    }

