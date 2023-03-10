#include "csprng.h"

#define INIT_BUFFER(offset) \
frt[offset]      = frt[offset + 1]  = frt[offset + 2]  = \
frt[offset + 3]  = frt[offset + 4]  = frt[offset + 5]  = \
frt[offset + 6]  = frt[offset + 7]  = frt[offset + 8]  = \
frt[offset + 9]  = frt[offset + 10] = frt[offset + 11] = \
frt[offset + 12] = frt[offset + 13] = frt[offset + 14] = \
frt[offset + 15] = frt[offset + 16] = frt[offset + 17] = \
frt[offset + 18] = frt[offset + 19] = frt[offset + 20] = \
frt[offset + 21] = frt[offset + 22] = frt[offset + 23] = \
frt[offset + 24] = frt[offset + 25] = frt[offset + 26] = \
frt[offset + 27] = frt[offset + 28] = frt[offset + 29] = \
frt[offset + 30] = frt[offset + 31] = frt[offset + 32] = \
frt[offset + 33] = frt[offset + 34] = frt[offset + 35] = \
frt[offset + 36] = frt[offset + 37] = frt[offset + 38] = \
frt[offset + 39] = frt[offset + 40] = frt[offset + 41] = \
frt[offset + 42] = frt[offset + 43] = frt[offset + 44] = \
frt[offset + 45] = frt[offset + 46] = frt[offset + 47] = \
frt[offset + 48] = frt[offset + 49] = frt[offset + 50] = \
frt[offset + 51] = frt[offset + 52] = frt[offset + 53] = \
frt[offset + 54] = frt[offset + 55] = frt[offset + 56] = \
frt[offset + 57] = frt[offset + 58] = frt[offset + 59] = \
frt[offset + 60] = frt[offset + 61] = frt[offset + 62] = \
frt[offset + 63]  

#define ISAAC_MIX(a,b,c,d,e,f,g,h) \
  a-=e; f^=h>>9;  h+=a; \
  b-=f; g^=a<<9;  a+=b; \
  c-=g; h^=b>>23; b+=c; \
  d-=h; a^=c<<15; c+=d; \
  e-=a; b^=d>>14; d+=e; \
  f-=b; c^=e<<20; e+=f; \
  g-=c; d^=f>>17; f+=g; \
  h-=d; e^=g<<14; g+=h; \

// Inverted Frankenstein sequel monstrosity based on ISAAC_MIX
#define ADAM_MIX(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
  frt[a]+=frt[m]; frt[f]^=frt[p]<<9;  frt[p]-=frt[a]; \
  frt[b]+=frt[n]; frt[g]^=frt[i]>>9;  frt[i]-=frt[b]; \
  frt[c]+=frt[o]; frt[h]^=frt[j]<<23; frt[j]-=frt[c]; \
  frt[d]+=frt[p]; frt[a]^=frt[k]>>15; frt[k]-=frt[d]; \
  frt[e]+=frt[i]; frt[b]^=frt[l]<<14; frt[l]-=frt[e]; \
  frt[f]+=frt[j]; frt[c]^=frt[m]>>20; frt[m]-=frt[f]; \
  frt[g]+=frt[k]; frt[d]^=frt[n]<<17; frt[n]-=frt[g]; \
  frt[h]+=frt[l]; frt[e]^=frt[o]>>14; frt[o]-=frt[h]; \

// ChaCha rounding functions (64-bit versions adapted from BLAKE2b)
#define ROTR(a,b) frt[a] = ((frt[a] >> b) | (frt[a] << ((-b) & 63)))
#define QR(a,b,c,d) (	\
  frt[a] += frt[b], frt[d] ^= frt[a], ROTR(d, 32), \
  frt[c] += frt[d], frt[b] ^= frt[c], ROTR(b, 24), \
  frt[a] += frt[b], frt[d] ^= frt[a], ROTR(d, 16), \
  frt[c] += frt[d], frt[b] ^= frt[c], ROTR(b, 63)  \
)

u64 CSPRNG::get(u8 ind) {
  u64 tmp = 0; 
  SWAP(tmp, frt[ind]); 
  --size; 
  return (tmp >> (64 - precision)); 
}

u8 CSPRNG::generate() {
  accumulate();
  diffuse();
  assimilate();
  mangle();
  // mangle();
  // mangle();
  // mangle();
  // mangle();
  // mangle();
  // mangle();

  return 0;
}

void CSPRNG::accumulate() {
  // INIT_BUFFER(0) = INIT_BUFFER(64) = 
  // INIT_BUFFER(128) = INIT_BUFFER(192) = seed;

  /*  
    start by supplying the initialization vectors
    8 u64's that correspond to the quote:
    "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
  */ 
 
  frt[4]   = frt[132] = 0x4265206672756974LLU ^ seed;
  frt[12]  = frt[140] = 0x66756C20616E6420LLU ^ seed;
  frt[48]  = frt[176] = 0x6D756C7469706C79LLU ^ seed;
  frt[56]  = frt[184] = 0x2C20616E64207265LLU ^ seed;
  frt[68]  = frt[196] = 0x706C656E69736820LLU ^ seed;
  frt[76]  = frt[204] = 0x7468652065617274LLU ^ seed;
  frt[112] = frt[240] = 0x68202847656E6573LLU ^ seed;
  frt[120] = frt[248] = 0x697320313A323829LLU ^ seed;

  // GOLDEN RATIO, per Bob's original implementation
  frt[7]  = frt[135] = frt[15] = frt[143] = frt[51]  = frt[179] = frt[59]  = frt[187] =
  frt[71] = frt[199] = frt[79] = frt[207] = frt[115] = frt[243] = frt[123] = frt[251] = 
  0x9e3779b97f4a7c13LLU;
}

void CSPRNG::diffuse() {
  // permute IV's with ISAAC mixing logic
  ISAAC_MIX(frt[4],frt[12],frt[48],frt[56],frt[196],frt[204],frt[240],frt[248])
  ISAAC_MIX(frt[132],frt[140],frt[176],frt[184],frt[68],frt[76],frt[112],frt[120])
  ISAAC_MIX(frt[4],frt[12],frt[48],frt[56],frt[196],frt[204],frt[240],frt[248])
  ISAAC_MIX(frt[132],frt[140],frt[176],frt[184],frt[68],frt[76],frt[112],frt[120])

  // permute GR with ISAAC mixing logic
  ISAAC_MIX(frt[7], frt[15], frt[51], frt[59], frt[199], frt[207], frt[243], frt[251]) 
  ISAAC_MIX(frt[135], frt[143], frt[179], frt[187], frt[71], frt[79], frt[115], frt[123]) 
  ISAAC_MIX(frt[7], frt[15], frt[51], frt[59], frt[199], frt[207], frt[243], frt[251])  
  ISAAC_MIX(frt[135], frt[143], frt[179], frt[187], frt[71], frt[79], frt[115], frt[123]) 

  reg m1, m2, m3, m4;
  int i{0};
  
  // SIMD time baby!
  do {
    // load 1024 bits of integer data (2048 if AVX-512 is available)
    m1 = REG_LOADBITS((reg*) &frt[i]); 
    m2 = REG_LOADBITS((reg*) &frt[i+(8  >> FRUIT_FACTOR)]);
    m3 = REG_LOADBITS((reg*) &frt[i+(16 >> FRUIT_FACTOR)]);
    m4 = REG_LOADBITS((reg*) &frt[i+(24 >> FRUIT_FACTOR)]);
    // add
    m1 = REG_ADD64(m1, m3);
    m2 = REG_ADD64(m2, m4);
    m3 = REG_ADD64(m1, m4);
    m4 = REG_ADD64(m2, m3);
    // store mixed results (note the index flip)
    REG_STOREBITS((reg*) &frt[i+(24 >> FRUIT_FACTOR)],  m1);
    REG_STOREBITS((reg*) &frt[i+(16 >> FRUIT_FACTOR)],  m2);
    REG_STOREBITS((reg*) &frt[i+(8  >> FRUIT_FACTOR)],  m3);
    REG_STOREBITS((reg*) &frt[i],                       m4);

    ISAAC_MIX(frt[4],frt[12],frt[48],frt[56],frt[196],frt[204],frt[240],frt[248])
    ISAAC_MIX(frt[132],frt[140],frt[176],frt[184],frt[68],frt[76],frt[112],frt[120])
    ISAAC_MIX(frt[7], frt[15], frt[51], frt[59], frt[199], frt[207], frt[243], frt[251]) 
    ISAAC_MIX(frt[135], frt[143], frt[179], frt[187], frt[71], frt[79], frt[115], frt[123])

    i += FRUIT_LOOP;
  } while (i < FRUIT_SIZE);

  // now swap alternating lower and upper columns from the "old" buffer and "new" buffer
  SWAP(frt[64],  frt[192])  SWAP(frt[1],   frt[129])  SWAP(frt[66],  frt[194])
  SWAP(frt[80],  frt[208])  SWAP(frt[17],  frt[145])  SWAP(frt[82],  frt[210])
  SWAP(frt[96],  frt[224])  SWAP(frt[33],  frt[161])  SWAP(frt[98],  frt[226])
  SWAP(frt[112], frt[240])  SWAP(frt[49],  frt[177])  SWAP(frt[114], frt[242])

  SWAP(frt[3],   frt[131])  SWAP(frt[68],  frt[196])  SWAP(frt[5],   frt[133])
  SWAP(frt[19],  frt[147])  SWAP(frt[84],  frt[212])  SWAP(frt[21],  frt[149])
  SWAP(frt[35],  frt[163])  SWAP(frt[100], frt[228])  SWAP(frt[37],  frt[165])
  SWAP(frt[51],  frt[179])  SWAP(frt[116], frt[244])  SWAP(frt[53],  frt[181])

  SWAP(frt[70],  frt[198])  SWAP(frt[7],   frt[135])  SWAP(frt[72],  frt[200])
  SWAP(frt[86],  frt[214])  SWAP(frt[23],  frt[151])  SWAP(frt[88],  frt[216])
  SWAP(frt[102], frt[230])  SWAP(frt[39],  frt[167])  SWAP(frt[104], frt[232])
  SWAP(frt[118], frt[246])  SWAP(frt[55],  frt[183])  SWAP(frt[120], frt[248])

  SWAP(frt[9],   frt[137])  SWAP(frt[74],  frt[202])  SWAP(frt[11],  frt[139])
  SWAP(frt[25],  frt[153])  SWAP(frt[90],  frt[218])  SWAP(frt[27],  frt[155])
  SWAP(frt[41],  frt[169])  SWAP(frt[106], frt[234])  SWAP(frt[43],  frt[171])
  SWAP(frt[57],  frt[185])  SWAP(frt[122], frt[250])  SWAP(frt[59],  frt[187])

  SWAP(frt[76],  frt[204])  SWAP(frt[13],  frt[141])  SWAP(frt[78],  frt[206])
  SWAP(frt[92],  frt[220])  SWAP(frt[29],  frt[157])  SWAP(frt[94],  frt[222])
  SWAP(frt[108], frt[236])  SWAP(frt[45],  frt[173])  SWAP(frt[110], frt[238])
  SWAP(frt[124], frt[252])  SWAP(frt[61],  frt[189])  SWAP(frt[126], frt[254])

  SWAP(frt[15],  frt[143])   
  SWAP(frt[31],  frt[159]) 
  SWAP(frt[47],  frt[175])  
  SWAP(frt[63],  frt[191])
}

void CSPRNG::assimilate() {
  // mask is -1 so inverted num stays the same
  const reg mask = REG_SET64(-1LLU);
  const reg srlv = REG_SET64(undulation);

  reg m1, m2, m3, m4;
  int i{0};

  do {
    m1 = REG_LOADBITS((reg*) &frt[i]); 
    m2 = REG_LOADBITS((reg*) &frt[i+(8  >> FRUIT_FACTOR)]);
    m3 = REG_LOADBITS((reg*) &frt[i+(16 >> FRUIT_FACTOR)]);
    m4 = REG_LOADBITS((reg*) &frt[i+(24 >> FRUIT_FACTOR)]);

    m1 = REG_ANDNOT(m1, mask);
    m2 = REG_ANDNOT(m2, mask);
    m3 = REG_ANDNOT(m3, mask);
    m4 = REG_ANDNOT(m4, mask);
    
    // now right shift by undulation parameter
    m1 = REG_SRLV64(m1, srlv);
    m2 = REG_SRLV64(m2, srlv);
    m3 = REG_SLLV64(m3, srlv);
    m4 = REG_SLLV64(m4, srlv);

    // note stored in reverse again - to change order
    REG_STOREBITS((reg*) &frt[i+(24 >> FRUIT_FACTOR)],  m1);
    REG_STOREBITS((reg*) &frt[i+(16 >> FRUIT_FACTOR)],  m2);
    REG_STOREBITS((reg*) &frt[i+(8  >> FRUIT_FACTOR)],  m3);
    REG_STOREBITS((reg*) &frt[i],                       m4);

    i += FRUIT_LOOP;
  } while (i < FRUIT_SIZE);

  // BLOCK 1                                // BLOCK 9
  ADAM_MIX(0,1,2,3, 16,17,18,19,            128,129,130,131,144,145,146,147);
  ADAM_MIX(32,33,34,35,48,49,50,51,         160,161,162,163,176,177,178,179);
  // BLOCK 2                                // BLOCK 10
  ADAM_MIX(4,5,6,7, 20,21,22,23,            132,133,134,135,148,149,150,151);
  ADAM_MIX(36,37,38,39,52,53,54,55,         164,165,166,167,180,181,182,183);
  // BLOCK 3                                // BLOCK 11
  ADAM_MIX(8, 9, 10,11,24,25,26,27,         136,137,138,139,152,153,154,155);
  ADAM_MIX(40,41,42,43,56,57,58,59,         168,169,170,171,184,185,186,187);
  // BLOCK 4                                // BLOCK 12
  ADAM_MIX(12,13,14,15,28,29,30,31,         140,141,142,143,156,157,158,159);
  ADAM_MIX(44,45,46,47,60,61,62,63,         172,173,174,175,188,189,190,191);
  // BLOCK 5                                // BLOCK 13
  ADAM_MIX(64,65,66,67,80, 81, 82, 83,      192,193,194,195,208,209,210,211);
  ADAM_MIX(96,97,98,99,112,113,114,115,     224,225,226,227,240,241,242,243);
  // BLOCK 6                                // BLOCK 14
  ADAM_MIX(68, 69, 70, 71, 84, 85, 86, 87,  196,197,198,199,212,213,214,215);
  ADAM_MIX(100,101,102,103,116,117,118,119, 228,229,230,231,244,245,246,247);
  // BLOCK 7                                // BLOCK 15
  ADAM_MIX(72, 73, 74, 75, 88, 89, 90, 91,  200,201,202,203,216,217,218,219);
  ADAM_MIX(104,105,106,107,120,121,122,123, 232,233,234,235,248,249,250,251);
  // BLOCK 8                                // BLOCK 16
  ADAM_MIX(76, 77, 78, 79, 92, 93, 94, 95,  204,205,206,207,220,221,222,223);
  ADAM_MIX(108,109,110,111,124,125,126,127, 236,237,238,239,252,253,254,255);
}

void CSPRNG::mangle() {
  /*  
    apply ChaCha20 quarter rounding function 3x per table. This also
    rotates right like BLAKE2b rather than left like the original ChaCha
  */
  
  // 1ST ROUND - VERTICAL

  // TABLE 1              TABLE 2                 TABLE 3                 TABLE 4
  QR(0,16,32,48);         QR(4,20,36,52);         QR(8,24,40,56);         QR(12,28,44,60);
  QR(17,33,49,1);         QR(21,37,53,5);         QR(25,41,57,9);         QR(29,45,61,13);
  QR(34,50,2,18);         QR(38,54,6,22);         QR(42,58,10,26);        QR(46,62,14,30);
  QR(51,3,19,35);         QR(55,7,23,39);         QR(59,11,27,43);        QR(63,15,31,47);
  
  // TABLE 5              TABLE 6                 TABLE 7                 TABLE 8
  QR(64,80,96,112);       QR(68,84,100,116);      QR(72,88,104,120);      QR(76,92,108,124);
  QR(81,97,113,65);       QR(85,101,117,69);      QR(73,89,105,121);      QR(93,109,125,77);
  QR(98,114,66,82);       QR(102,118,70,86);      QR(74,90,106,122);      QR(110,126,78,94);
  QR(115,67,83,99);       QR(119,71,87,103);      QR(75,91,107,123);      QR(127,79,95,111);
    
  // TABLE 9              TABLE 10                TABLE 11                TABLE 12
  QR(128,144,160,176);    QR(132,148,164,180);    QR(136,152,168,184);    QR(140,156,172,188);
  QR(145,161,177,129);    QR(149,165,181,133);    QR(153,169,185,137);    QR(157,173,189,141);
  QR(162,178,130,146);    QR(166,182,134,150);    QR(170,186,138,154);    QR(174,190,142,158);
  QR(179,131,147,163);    QR(183,135,151,167);    QR(187,139,155,171);    QR(191,143,159,175);

  // TABLE 13             TABLE 14                TABLE 15                TABLE 16
  QR(192,208,224,240);    QR(196,212,228,244);    QR(200,216,232,248);    QR(204,220,236,252);
  QR(209,225,241,193);    QR(213,229,245,197);    QR(217,233,249,201);    QR(221,237,253,205);
  QR(226,242,194,210);    QR(230,246,198,214);    QR(234,250,202,218);    QR(238,254,206,222);
  QR(243,195,211,227);    QR(247,199,215,231);    QR(251,203,219,235);    QR(255,207,223,239);

  // 2ND ROUND - DIAGONAL

  // TABLE 1              TABLE 2                 TABLE 3                 TABLE 4
  QR(0,17,34,51);         QR(4,21,38,55);         QR(8,25,42,59);         QR(12,29,46,63);
  QR(1,18,35,48);         QR(5,22,39,52);         QR(9,26,43,56);         QR(13,30,47,60);
  QR(2,19,32,49);         QR(6,23,36,53);         QR(10,27,40,57);        QR(14,31,44,61);
  QR(3,16,33,50);         QR(7,20,37,54);         QR(11,24,41,58);        QR(15,28,45,62);
  
  // TABLE 5              TABLE 6                 TABLE 7                 TABLE 8  
  QR(64,81,98,115);       QR(68,85,102,119);      QR(72,89,106,123);      QR(76,93,110,127);
  QR(65,82,99,112);       QR(69,86,103,116);      QR(73,90,107,120);      QR(77,94,111,124);
  QR(66,83,96,113);       QR(70,87,100,117);      QR(74,91,104,121);      QR(78,95,108,125);
  QR(67,80,97,114);       QR(71,84,101,118);      QR(75,88,105,122);      QR(79,92,109,126);
  
  // TABLE 9              TABLE 10                TABLE 11                TABLE 12
  QR(128,145,162,179);    QR(132,149,166,183);    QR(136,153,170,187);    QR(140,157,174,191);
  QR(129,146,163,176);    QR(133,150,167,180);    QR(137,154,171,184);    QR(141,158,175,188);
  QR(130,147,160,177);    QR(134,151,164,181);    QR(138,155,168,185);    QR(142,159,172,189);
  QR(131,144,161,178);    QR(135,148,165,182);    QR(139,152,169,186);    QR(143,156,173,190);

  // TABLE 13             TABLE 14                TABLE 15                TABLE 16  
  QR(192,209,226,243);    QR(196,213,230,247);    QR(200,217,234,251);    QR(204,221,238,255);
  QR(193,210,227,240);    QR(197,214,231,244);    QR(201,218,235,248);    QR(205,222,239,252);
  QR(194,211,224,241);    QR(198,215,228,245);    QR(202,219,232,249);    QR(206,223,236,253);
  QR(195,208,225,242);    QR(199,212,229,246);    QR(203,216,233,250);    QR(207,220,237,254);

  // 3RD ROUND - HORIZONTAL

  // TABLE 1              TABLE 2                 TABLE 3                 TABLE 4
  // QR(0,1,2,3);            QR(4,5,6,7);            QR(8,9,10,11);          QR(12,13,14,15);
  // QR(17,18,19,16);        QR(21,22,23,20);        QR(25,26,27,24);        QR(29,30,31,28);
  // QR(34,35,32,33);        QR(38,39,36,37);        QR(42,43,40,41);        QR(46,47,44,45);
  // QR(51,48,49,50);        QR(55,52,53,54);        QR(59,56,57,58);        QR(63,60,61,62);

  // // TABLE 5              TABLE 6                 TABLE 7                 TABLE 8
  // QR(64,65,66,67);        QR(68,69,70,71);        QR(72,73,74,75);        QR(76,77,78,79);
  // QR(81,82,83,80);        QR(85,86,87,84);        QR(89,90,91,88);        QR(93,94,95,92);
  // QR(98,99,96,97);        QR(102,103,100,101);    QR(106,107,104,105);    QR(110,111,108,109);
  // QR(115,112,113,114);    QR(119,116,117,118);    QR(123,120,121,122);    QR(127,124,125,126);

  // // TABLE 9              TABLE 10                TABLE 11                TABLE 12
  // QR(128,129,130,131);    QR(132,133,134,135);    QR(136,137,138,139);    QR(140,141,142,143);
  // QR(145,146,147,144);    QR(149,150,151,148);    QR(153,154,155,152);    QR(157,158,159,156);
  // QR(162,163,160,161);    QR(166,167,164,165);    QR(170,171,168,169);    QR(174,175,172,173);
  // QR(179,176,177,178);    QR(183,180,181,182);    QR(187,184,185,186);    QR(191,188,189,190);

  // // TABLE 13             TABLE 14                TABLE 15                TABLE 16
  // QR(192,193,194,195);    QR(196,197,198,199);    QR(200,201,202,203);    QR(204,205,206,207);
  // QR(209,210,211,208);    QR(213,214,215,212);    QR(217,218,219,216);    QR(221,222,223,220);
  // QR(226,227,224,225);    QR(230,231,228,229);    QR(234,235,232,233);    QR(238,239,236,237);
  // QR(243,240,241,242);    QR(247,244,245,246);    QR(251,248,249,250);    QR(255,252,253,254);
}
