#include "csprng.h"
  
#define isaac_mix(a,b,c,d,e,f,g,h)\
{ \
   a-=e; f^=h>>9;  h+=a; \
   b-=f; g^=a<<9;  a+=b; \
   c-=g; h^=b>>23; b+=c; \
   d-=h; a^=c<<15; c+=d; \
   e-=a; b^=d>>14; d+=e; \
   f-=b; c^=e<<20; e+=f; \
   g-=c; d^=f>>17; f+=g; \
   h-=d; e^=g<<14; g+=h; \
}

#define adam_mix(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)\
{ \
   frt[a]-=frt[m]; frt[f]^=frt[p]>>9;  frt[p]+=frt[a]; \
   frt[b]-=frt[n]; frt[g]^=frt[i]<<9;  frt[i]+=frt[b]; \
   frt[c]-=frt[o]; frt[h]^=frt[j]>>23; frt[j]+=frt[c]; \
   frt[d]-=frt[p]; frt[a]^=frt[k]<<15; frt[k]+=frt[d]; \
   frt[e]-=frt[i]; frt[b]^=frt[l]>>14; frt[l]+=frt[e]; \
   frt[f]-=frt[j]; frt[c]^=frt[m]<<20; frt[m]+=frt[f]; \
   frt[g]-=frt[k]; frt[d]^=frt[n]>>17; frt[n]+=frt[g]; \
   frt[h]-=frt[l]; frt[e]^=frt[o]<<14; frt[o]+=frt[h]; \
}

#define seed_block(a,b,c,d,e,f,g,h,i,j,k,l,m,n,ind,x) \
{ \
    frt[a] = GENESIS[ind]    ^ x; \
    frt[b] = GENESIS[ind+1]  ^ x; \
    frt[c] = GENESIS[ind+2]  ^ x; \
    frt[d] = GENESIS[ind+3]  ^ x; \
    frt[e] = GENESIS[ind+4]  ^ x; \
    frt[f] = GENESIS[ind+5]  ^ x; \
    frt[g] = GENESIS[ind+6]  ^ x; \
    frt[h] = GENESIS[ind+7]  ^ x; \
    frt[i] = GENESIS[ind+8]  ^ x; \
    frt[j] = GENESIS[ind+9]  ^ x; \
    frt[k] = GENESIS[ind+10] ^ x; \
    frt[l] = GENESIS[ind+11] ^ x; \
    frt[m] = GENESIS[ind+12] ^ x; \
    frt[n] = GENESIS[ind+13] ^ x; \
    ind += 14; \
}

// THE BOOK OF GENESIS (except not really at all)
static constexpr u64 GENESIS[GENESIS_SIZE] = {
0x91f0db5ad2b7d9ed, 0x85a4f380de623834, 0xbc317685e6d27b2e, 0x2b093ab7782e6873,
0x74f99f46d1153a39, 0xe9c6c1cf32d8e972, 0x3aff57c9d9ec21b2, 0xf89a6c0dd5b4320f,
0xac1f950c1b9746be, 0x9803009de2963ee9, 0xe63ed5740a95b83e, 0xd118ceae1c32cc75,
0xfcaa6f5ff7007523, 0xe47d1ea6d9f6fe17, 0x7b7a5bc30bc9da60, 0xbfe0fe1b46189448,
0x7b81bee500c9f5a5, 0x8e8ad4e33531f0c1, 0x3f8e9db6d42379e6, 0xa74acfd12651aace,
0xeef4a6df7fdf155b, 0x93dfc9cb0bda3ac2, 0x3f2dd4ee639eacf4, 0x9f4fe3690d670632,
0x1ba1a88c67ec19f4, 0x657e5bdaaefc5ed7, 0xdec52d9a0c4cc715, 0x78939de72750bfc7,
0xeea16f9370ae55b2, 0xba6b4639eab4657a, 0xf52038720d6a1dcc, 0x23ef82c024023a93,
0xf125f37776deeaf2, 0x2529da64fd9fff53, 0x64d2718e690a48d0, 0x17526f27713fe146,
0x7ad1202f8dadf305, 0x15d96d2c01ed3e9a, 0xa3853cad9e651bd, 0xd13d66deeb320b3e,
0xf8535381bbade8fc, 0xee989dff5bc56b8, 0xbe9432331e2342ce, 0x9c64564f91623efd,
0xc7ac32e6e31d4b2e, 0x64534a4cbd26c8da, 0x662d916d236b06cf, 0xe3af823259343c30,
0x59a3bcca043582f2, 0x77f2840d9d127643, 0x42c68a87c3de299d, 0x621a097535d4ca93,
0xafb793d97a7f221d, 0x35d9a23759fe1f2f, 0xfd12f56faa3f7a0, 0x89af30304fbb9fdd,
0x5d55d42aa0802f6f, 0xe4b491496086ce90, 0x3ea9e576e5de3d53, 0xdde76078543f0b60,
0x203ce9e5a65e91bb, 0x9242379c3eedfff6, 0x5b1f53a6e4126208, 0xa341ec1727355f38,
0x1a317438498ec613, 0x21bfabe471668040, 0x610119ce6085b841, 0x119dca71cf23f7db,
0xea0bdd75bdf8c9f3, 0x63a08145bb9277c0, 0x37d32f1e6be896d6, 0x9491e11b88624d7f,
0x25e863428bb548fb, 0xae874fbe6dea3d94, 0x59b7cc50711cafe7, 0xe02a662a9d7cd466,
0x587406e4d650ad84, 0x62214bb0a99b8013, 0x38b426613d23d749, 0x68ec49c9bbd34929,
0xf4d1bfa33c661118, 0x9d4b0bf151013d8, 0x78eb68398c6a3692, 0x21baf02724b97bcb,
0x462388d4a877a952, 0x98c5409480b38624, 0x38a6caae4fe9c860, 0x99eda2cc433b03bd,
0x2b96e83a95ea0e04, 0xd6fd130b9f64d12f, 0xda0ac21ff4cdeef1, 0x73d53fe1df6b2e36,
0x3ca4b9b48c24b7e3, 0x7d85c6ca1e6afb4f, 0x94c0f53d117b460e, 0xc387d9cedf90f73,
0x2a5aec743596c423, 0xd9c4a79c9222e80e, 0x3923362d33a9438, 0xf31c4e0297d1162e,
0xfb2be8e1ca025dc5, 0x8db65f4b97513ceb, 0xd09000e4909df97f, 0xb3ee2f7793f99d94,
0x670397b61de56f70, 0x5d99ef923b37d821, 0xf5da0fea21b026cb, 0xe77f4ed2143b4c2f,
0xa33f212904d19b11, 0x9e78eea5af1c8b1, 0xe701928d75a7711a, 0x2c56e5aad72ba3ab,
0x68381d3da1391780, 0x8f60b44a1be3dc3a, 0x77fefb73ecbda665, 0xa574ad73bff9245b,
0xc59c77a6f7a75ae7, 0x4c443207612cf6c0, 0xf3f7074deaabb7cc, 0x2ea9149c769c1bb5,
0xd202054da76f7e76, 0x4db979390edbc81c, 0xe2b2df89216eaa85, 0x3663885d6fdfcede,
0xc2abaa6201bb28b3, 0xbddaf7e571ef1aa, 0xd76f75473c79c42a, 0x916503053dc0bb7,
0x50bfdb4b32dde5d4, 0x9fd779b6d0fcd3fc, 0x172fd3d965f3c98e, 0xacc04b8914623f63,
0x1f3615cc83ca9, 0x55a47e47c99379ca, 0x4b2e5388eb0cd14c, 0x361b44ace5ca774e,
0x6f2cee2fe607534c, 0x2f815800a61c6cfd, 0x720b356cca706c3c, 0x96ac98787070fa10,
0xf12af25126d56fac, 0x6ab055fc0fe217d8, 0x55d5a481b3db9509, 0xbc0ea0a73a8dd8aa,
0xede4c6d24fd90f9c, 0x9a350d1e2dce307b, 0x313de4bde53bd0f5, 0xf6e08c5914d31a79,
0x2cdadfa3f96944c0, 0xa3b00aee24f61ba0, 0xbe6ef0c38d5e6ff9, 0x89907927380795aa,
0x232259c9d845a794, 0x98ddb6c8b23bb478, 0xc7f337932b6c10af, 0x62b119a0d97fc82c,
0xfaa538dd19d70c6, 0x28016adda0718b1a, 0x81b41c22f3002d06, 0x78ebbab85732b523,
0x21c2fbae1d585f9a, 0xd37726b55853cd9c, 0xbdf1ac8e08b3dbde, 0x673b03ead476664b,
0x41daa570dde81603, 0x7a49965b522a8ab, 0x14da29874d5947e4, 0xb6373bffb7502207,
0x8a33f6dd6450072e, 0x40234d479d220327, 0x6af9ba8e0200d449, 0xbe2f69a415cb4647,
0x75f8f73f36fba1f0, 0x3f7495ccc7f0d2e, 0x64a000d1dacc9b4b, 0x8060f5ac566ca2d3,
0x602e24450451a9d6, 0xe475260e191db2e6, 0x8ff627e6aef14a57, 0x29e4478f60c7c429,
0x8c492703ffc48bad, 0x4afbb4fc05be69a0, 0xa56a622dfe36b912, 0x23924d59cce0ac7c,
0xb515d3db65430561, 0xcaafbee30803ada9, 0x693f63e2f20e4a24, 0xa05abf0cb052ee7d,
0xe0aef7b700a7d507, 0x2f8a2ce98bf29b57, 0xf15f7726126766d5, 0xc24fef6691d782c8,
0x88b31ed70a23adc6, 0x8d6eadac818150a7, 0x7dc5dcb6dcf70217, 0x8842e895c8286611,
0x25b8b44c539fed81, 0xaf6ef12b68be7534, 0xf5449b77aa5ced0d, 0xa05063802d553163,
0xb99fe53534cb8459, 0x30b6356b284e299c, 0x5fa72fb8d2ac4d73, 0x8203ac4792056928,
0x5d431ccd7648df4a, 0xa414cb8a42d26065, 0xfff8c40ec447604d, 0xd51be986dc81ba2e,
0xfd4eb4aac7b84fa4, 0x164f3cd553727dd8, 0xcd7c56026fe158b8, 0xaef058f097b995af,
0x4f2cf882b98c5d49, 0x8f53000ba5522621, 0x3e1647f382f84e01, 0xd30f8594f4d805e7,
0x6f163f38ae8937d2, 0xfba68d571d8235e1, 0x11daa4b4afd0eca, 0x197e73f0e54c3c41,
0xfff04857167ffd94, 0xfd6ccb66b2b9bb6, 0x393a42960d758521, 0x55afb3651161f8a8,
0x992a5f31e0d1a67c, 0xb463a4e2a7e0ce33, 0x81a2187a3cb0d753, 0x296c35086a1ca5dc,
0x58f13a117e451898, 0x7dd3ff306b4f45f9, 0xfc554e5dbf1d212e, 0x6c4ddffa7fc697e8,
0xefb021e99711eb9a, 0x2ae1da2fd27ad436, 0x918d83509befc1ba, 0x36fce79f58591970,
0xa77bd434241d6289, 0x6b740a7ccb3c852e, 0x7d020bc8f05a729a, 0xcf24402a59efcb09,
0x9ba31522bcfcd033, 0x508918d6ed6caa9b, 0xe8c0d4a9421a7712, 0x81af310079783179,
0x3bfb45651aa19cd7, 0x594346d8fce4f3d7, 0xcee625065e1d7f6b, 0x3a11e08e6641833b,
0x5287a6fcabbba686, 0x9c779b285bd84a62, 0x55f392f8e8163395, 0x2e4ea9795052fc1c,
0xfa08350feeda92a1, 0x69b91fe4c3261f7f, 0x4e75ce0c083dda86, 0xd47a1b660edaf69a,
0x53a38221fcd53a3e, 0x9c59b5b037ce22c, 0x6f6124fb4ea34d53, 0xdf33ef769b9741fb,
0x35dda167b24ecd58, 0x182d7f081f083ce3, 0x64191c7e214b8d44, 0xa50eb750e57fcd73,
0x92b819c19765421e, 0xecc79ca5b737ead, 0x6061941114818e10, 0xcd252cbada00be87,
0x7bce80e159f7115f, 0xd34a1ffd7e70aa2e, 0xe83ccf1f3251b518, 0xdae8cc32d0b139fe,
0xbc786e747a13b9ef, 0x283987e7ba9c08e5, 0x3e73fd3152463377, 0xc7aadb0bf5413b82,
0x9e2331843d5c100a, 0x7cd31a591843c809, 0xd95d287f7617a0fc, 0x76867c1d782a9037,
0x8c14435d241f0e68, 0x3078b300facb0335, 0xe2123c455ae81039, 0xdbf00c4c383e867e,
0xdd027e05b76a28fb, 0x8506913e80de2cef, 0xc7bea6c56ff910d1, 0x853230449662440a,
0x94d58d9e9a329d3a, 0xd7c2155fc192490f, 0x2a31e3532ffba15a, 0xb0bcaa8a0d417f90,
0xcd3d60c3ca992a4d, 0xd88f205a64577de1, 0x36fab8a3a06695b4, 0x57242b7df1c14538,
0xd0bc372f9d120ae7, 0xa1f7f2f93c5b7aba, 0x7784ced76bd100ee, 0x959776b7e77084ae,
0x91cdb7917f56d971, 0x2926f888f8b814ee, 0xd425841e6eec3d19, 0x711a84be7dd4ded3,
0x599145c95e546084, 0xae7247219283b0a3, 0xdb04394e9d7a0e2e, 0x66bf5587ae2e1857,
0x74ba965356eea257, 0xd5d0b85b37e88756, 0xcec421a09ebd3cb7, 0x392824d14da84e98,
0x236ce4aace768490, 0xec06cd663acbb908, 0xbc857eddcc1fa2bf, 0xcad99d4c54e0552c,
0x1593832a01c33ae9, 0x80d8ac0471ca589f, 0x6277413e26913df7, 0x2c66adc8e3d206fc,
0x5c34b4b7d6ff757c, 0xb70380cc96d0e737, 0x307370eecd78fc31, 0xe607288bd34e845a,
0x1e8cd120c8c9ec38, 0x7b87b4f1ae623b4c, 0x9b9a91ac47f61b90, 0x579dda81c9b1007b,
0x8be6d353dd74ffbd, 0xe8229c5cc6d8ec4, 0x1eb5ca4bddcd47c0, 0x377c236b0fae8eb8,
0x7689b06a4bdafaa2, 0x3756f9aa5495cb0d, 0xef6e4877acce2cd3, 0x120e366b85002219,
0xc0ef9441d7ab7939, 0x95b3a4caf2a694c9, 0xae6e425683a2ef78, 0x7a3ada69163113f3,
0xe293c11b3565337a, 0x757bfc409315347, 0xeeaba7c43267a50f, 0xd87e6c48a0a91a90,
0xa002efb273d57a78, 0x14d4f1711f92808a, 0x2a900dfe6230b2fd, 0x8ecde2dcf45d822,
0xfc2f2a56173d6411, 0x207fe8200197d2ec, 0xc669f3ec3d22a777, 0xe5cf7aa11437f879,
0xf949730ab3dc929a, 0x60fd7aa1d7ac5fcf, 0x2d29a5518f789252, 0xb5709ae08826264a,
0x4b4f8d282a8495cd, 0xfc942ef0c69bc9c9, 0x12b5602c2f18a069, 0xe83a16b97b593f4a,
0x1a6ed5ca6482bf83, 0x6c1a98ffd3f569c3, 0x8128f0abec74d03a, 0xd7f0a999fbd1d17e,
0xe87bec00718c826c, 0x264511e074fe6be4, 0xd512d57401541578, 0xdb20281b17f9e4df,
0xffc941176b389fe7, 0x37905cb1d83e2489, 0xd75fcd0e6cdf585d, 0x8d0c136a8c36c24c,
0x948baae166f4d6f9, 0x57f22e29bcdfa9c1, 0xc0039fa06630d69f, 0xb1779c72b96a45e8,
0x1c8fb143ffb0b18c, 0xd45af53a20739ee, 0x9f328d6978231253, 0x20f7fac4e65178c9,
0x1cc0af68eabe3e48, 0x4cb29c467260a2bf, 0x67445ebf522953c, 0x82caec2827abb092,
0x179eea87b731c65, 0xe31d5ad564c776ca, 0x94d60f7667134952, 0xeaf46ab0f0e176de,
0x47b657888523c84d, 0x729d47d667bca998, 0x64b8639eedcadcf5, 0x72c83558730113c6,
0xc953cd256c75c78d, 0xafa966e9fbb3f7c9, 0xf7d625483bb362c5, 0xbdb0084277f0a1e1,
0xd07efe08f848d990, 0x320ab4540e1af10, 0x810d2f39cc83d0e, 0x14fbd3cb6b3a2fb1,
0xb7715137bafa39a6, 0x88f06ae97d3852a2, 0x1f8db2260a5fba54, 0x8cb93facab832135,
0x83645948d055d42, 0x671240eda5855969, 0x52f34087bff9e935, 0xeedb76e3b3bdb32a,
0x3f1607911cfdaa37, 0x20b78d863f9976ca, 0x9de2c05a5f6ebede, 0xa19850d096977485,
0x1e9998bdc3d27ef1, 0x95697bdb2b7a02dc, 0x38d66e349060cdde, 0x173f4568022efd8f,
0xdb2df96ba0dc1b1c, 0x7e5fb3092920e42a, 0x2bf922e7b28bcbd0, 0x6e9c0c448caf1134,
0xc1041d36f036ebed, 0x7d402daef6021147, 0x263cfedad5a8e2cf, 0x377f9dc432d14dab,
0x17892a646ee80eb5, 0x2862500e786df946, 0xc1b49716cb157ed5, 0xb08b2a737bb95f81,
0x56330e63776a486f, 0xf6d4d204809ca6da, 0x530d501d53850521, 0xf2c26933e4088a76,
0xd3a068962aae045e, 0x9928ab49726af0d9, 0xb24c1cc62ad649a, 0xe046482a98842007,
0xa4db4d5150b973d6, 0xf46ce29141395b93, 0x3521ca0e6cd821d8, 0x8b62876266adadfa,
0x60d3876c99048b5b, 0xcab95cb0683a6fa, 0x7654a5cf918de8bf, 0x2b8cfecd48da0585,
0x30d1b4e595306f86, 0xd324c892e388daf8, 0x730326ef616ee27e, 0x4e52c2773d9d5330,
0x46eb8dfc76c3afbd, 0xbb5498d8b1dcdc0d, 0x2fa59ce1a8163c0a, 0x6f17d8c794992e12,
0x9b23e7ca5dfb5371, 0x9dc3cee6734ce1b7, 0x7ca1b12d80da23ab, 0x2f1ae59350d314dc,
0x4334820afde6db6, 0x53703b0f60146d0c, 0xbaad7d6e97b9a051, 0xf7efd317827077ea,
0x2880b2a59e75c0cc, 0xba56dabbc7e648a3, 0x2cdd8784a451f912, 0x19dbf92eba999218,
0x1b5103d79e40de9, 0xbd44f71977332a1a, 0x2f404ea72804992c, 0x1b3b0959c4c1699e,
0x7ef8716654af88e7, 0xbfe6242297dae90c, 0x4ead42d6952706d2, 0x64c8feb795547c80,
0xeb6f25f8650dc96a, 0x527396411e1e7373, 0x9d8cb2f7f810d8c0, 0x10b31aae392ca54d,
0x58cfabc431c08e98, 0x7b8a67e5e42628f, 0xdc4dc93cef48cb58, 0x7d5d4ab5f8f793f9,
0x91f0f153c5ac8f35, 0xf000c950477adeb0, 0x4f843881f13d38f4, 0x4d9aeab897adc419,
0x173231723b0e0f6c, 0x5a1cffa5dd0b135b, 0x67c4910c46dce304, 0x492ee250d296b79f,
0xecbe2940fea815f7, 0xef6e168bf6c7f4ef, 0xeca7b3ba788f2dd7, 0x5af5345e35e609bf,
0xfd42ab89bbc6df9a, 0x12b1771ac0ca55e3, 0x6555fc256f6609f5, 0xc2be7a07d6725cf7,
0xbacab773041dd2d4, 0x9e91f93a31f5fddd, 0xc7d1f0d441656d4a, 0x644f642cc003b962,
0x6e797f3e1b546b20, 0x212a4e5249a17bea, 0x7d39c94d5497bede, 0xd2f326aeb0f46684,
0x691a50c5d280efa4, 0xadeb06c4c07c2415, 0xe96e84df4daa5e81, 0x83a0b7dcfc254b1e,
0xf636edf92098fab2, 0x5a7751d1ca88abd1, 0xe63615adf2c79bd3, 0xa594acf45aa81720,
0x1c0e2a69c2564773, 0xb1a9226a9155ca38, 0x5eeed40619627636, 0xed899ed19cf603ba,
0xee43b765d77c456b, 0xa6d64590d2bc583b, 0x4fa519cda07ff2f1, 0x8500d2add07e7482,
0x8392b3bd4160efc4, 0x3fddfbdede409c78, 0x6eaace36ff09e989, 0x39fcac7cd54b3178,
0x2944e4f7b5426eaf, 0x2d7ed3aad83f3933, 0x5e6459eab3998ebc, 0x4f2507570830afa4,
0xbe1c65ed7a3d38d8, 0x3fe294e74d8c5acc, 0x9f3dacc9a903c237, 0xee22ce38b3a30c7c,
0xd2f09e0f84f1687e, 0x7f90d225be753155, 0x2e150d2212af6637, 0x6200c1feed8a36a6,
0x2f035be66997817c, 0xe2a284efe9dabd85, 0x993ba8b54450846e, 0x57745a2232af6713,
0x4b893ae05ca73332, 0xa5161cb0ee810a44, 0xa0977b7607ae1a51, 0xc766975e5795626a,
0xe8e5ba63b25409cd, 0xf1feff511cc41862, 0xdf4303a8443069e3, 0x95e6d3a0d08afe4d,
0xe80216d216ec142c, 0xf09a5f14b1927580, 0x9dd608a19d5f71b7, 0x77a61851171dfbf2,
0x42ab00937c33dae3, 0xf842435054f82e6f, 0x9f6b02d7042d8fb, 0x6f670e6e3738b47b,
0x4f7e2043982a7455, 0x5cb092433f08b990, 0xeebdd988298ebd63, 0xf4e42b96fd7196ec,
0x6f998a9bd1f2380f, 0x415d7b51f6d03b38, 0x266ecc0e161d90d3, 0x874d807c9c04cf7a,
0x66ca25761a9a245d, 0x103fdf4215163111, 0x47d8e5cd2fae1474, 0xbb881a1b62d6efc7,
0xe3bc501db28e44ee, 0x5dd81570915cc913, 0x4074a0acb84d3366, 0x1462c9e6a176e679,
0x4abba660085adc84, 0xe1e810412efe6644, 0xfd4d8fd3709c76e0, 0x1660469103fd772f,
0x9491b78851c6b0ff, 0xa125a0cde8ab25c4, 0x58a6f7382fe39509, 0xe05586d68e62b3de,
0x1e7b0857e1c38da3, 0x464fa1b8202b0868, 0x4d2d28ab86b6b527, 0xef24703bc22b95fb,
0x1dc9a4fcfc4cbb2, 0xca3440f78a3f73d8, 0xdbe16daa59d4208f, 0x4fea1aae79e0f0b3,
0xd939d1ebd44d4bd5, 0xdb49d14900d2ab3b, 0xcf1ae42c28e61771, 0x7ee06a9231526a45,
0x431007d33e15da1f, 0xd74014d4953e9cc9, 0xc0295174d891076, 0x252013967dc51a58,
0x8b40694a18f6d1f1, 0xb777d3c61d0cf48f, 0xccafcff0a5df172, 0x53b7ca511839e236,
0x805ea2acbca23532, 0xf3f4f4dc3c5bbcfc, 0x978c05a7f93b7ca7, 0xdd52def7a5e02ed0,
0x7b0de5b94a1e42cd, 0x144a94b4c421ed85, 0x86dd2a0bfe192a1, 0xc6fca453b71ae787,
0x7dcd892f94368a83, 0xc1c4949a08e538b4, 0x9e6badd759c57684, 0xe2f423b12826510b,
0xc0c03081c9b39714, 0xb8e368c710113a09, 0xbd35d00db0ee9874, 0xc1bcb9f924717347,
0xc38d39e6d8619458, 0xe5f23be7d4566c57, 0x968fedf1fc7088e7, 0xb337f1d0958dba93,
0x9bafcdf35ce16fff, 0x91f9ddc2cf4d591c, 0x935c87d7d8a8a57a, 0xfdac31959d476bc4
};

//  NOTE: Digit sequences with leading zeroes had those zeroes removed and replaced with other integers.
static constexpr u64 TAU[TAU_BLOCKS] = {
    2831853071,7958647692,5286766559,5657683943,3879875021,
    1641949889,1846156328,1257241799,7256069650,6842341359,
    6429617302,6564613294,1876892191,5116446345,5718816256,
    9622349005,6820540387,7042211119,2892458979,5986076392,
    8857621951,3318668922,5695129646,7573566330,5424038182,
    9129713384,6920697220,9086532964,2678721452,5498282547,
    4491740132,1263117634,9763041841,9256585081,8343072873,
    5785180720,5226610610,9764093304,2768293903,8830232188,
    6611454073,1519183906,1843722347,6386522358,6210237096,
    1489247599,2549913470,3771505449,7824558763,6602389825,
    9667346724,8813132861,7204278989,2790449474,3814043597,
    2188740554,1078434352,5863535047,6934963693,5338810264,
    8911362542,9052712165,5571542685,5155792183,4727435744,
    2936881802,4499068602,9309917074,2101584559,3785178470,
    8403991222,4258043921,7280688363,1962725954,9542619921,
    5613607689,5469098405,2108293318,5040299488,5701465037,
    3320042648,6817638142,5972663469,2993029078,1159253712,
    2011016213,3175939963,2714947276,8105142918,2057941282,
    8022194241,2560878079,5190313543,1540084067,5739872014,
    4611175263,5271884374,6250294241,5658563836,5237225173,
    4643158396,8296976583,2894121915,5541391444,1835134233,
    4458219633,8183056034,7013425497,1664457436,7041870793,
    1450242167,1583027397,6418288842,5135020669,3422062825,
    3422273981,7317032796,6300394033,5302337034,2875315236,
    7031130176,9819979719,9647746910,5666327101,5295837071,
    9391356385,8819647785,2569887520,4977045580,5724043877,
    3392964431,2561872107,4635608172,7454536853,3928438598,
    9363842981,7403415066,7221895827,6360812657,4775187696,
    5390711661,5479152289,5994540006,9457603655,7056277900,
    6435972690,4322221332,1767862810,6453889810,9110557357,
    8883515840,4880042901,5603841996,5892276509,5611716096,
    8848328095,5006307210,9813182860,1563167448,6024627502,
    3124568031,6772885417,8143656963,3515054247,6935649190,
    6866889924,4020192142,1027412169,2360237508,6241450982,
    6699884952,3423126664,2817869218,3131231012,5063476843,
    7403140452,2062038332,5777412932,2877795472,6375618814,
    2305505634,9379152803,1620940339,3049511548,1783289137,
    3554343170,5116653988,6803354404,3135354481,3625673313,
    5528245964,8789302663,9471839941,8806551877,3053391149,
    4046362640,6487432841,1722820672,1304907387,8320101289,
    9061203225,3564529788,4874794333,4353224620,9795006377,
    1464331109,9766842436,3569382505,8172202971,3556305552,
    5124750091,2751538995,4686736920,3121545407,1019258098,
    7849741768,1256213588,7244837409,4940167376,8534204511,
    6604807199,6832919022,4497054526,7265290228,3347904961,
    7238927168,1567507113,7712446342,3104189444,6130874185,
    2135947020,3113109876,2449150967,4570914235,9478723151,
    2335283385,7916105145,9504467711,7222776644,3422147245,
    3163243768,4886357714,9775962180,5330758685,3328433981,
    6268083116,5253357902,1693552237,3319153203,3199635617,
    8829971509,9525687757,1220052759,3086356627,2680502716,
    2832230380,4192998267,3974662622,3004540136,3860271859,
    1919432803,9439210725,3067116959,9619269774,3607822322,
    5627191937,1309577366,5171287579,2346319524,3048392431,
    5579259580,9639644398,9245389742,7492488945,8186912940,
    5570753899,1771918321,3578564982,1088250319,9260156273,
    6734980418,7498314657,9254005731,3658688862,6846947024,
    7859651833,3479006851,9917379413,9453466516,5471806242,
    5774933209,3292297570,3692285655,5319832161,8079730515,
    1434526163,6669888836,4038706677,3142584691,5487511586,
    8812435742,2660126212,9066481079,8338736520,7492353277,
    1315177551,6040245873,2706540534,2013625236,5034582921,
    6405083785,7718704889,8214027641,2423107655,5871305938,
    5137934837,2867226995,8950491052,5828530456,8483848616,
    6776207160,1075740479,9908434422,7373100550,6827244233,
    8628138933,9026373856,2051495919,7121029001,9043431826,
    6355032199,1573110396,3772386422,5642214188,8457448088,
    4962306812,1117919167,1163046402,4369211641,1271853986,
    5695770226,4137253255,1775428920,7199331221,6861451393,
    8911261289,7837519893,3193545694,3430791472,2421636168,
    3094546285,3234978662,6834926532,4708444145,2002920254,
    5241386927,9041128891,9865833259,7332156617,8136237580,
    1816305901,2725356415,1228777631,5627022693,9073260775,
    6824184693,8857374616,7864086466,7745550993,6104206056,
    4308864944,6777690430,6874545002,5717949538,2921616628,
    8082517363,6308009837,5544575739,6037069090,7401305331,
    1298341830,8590455134,1844443494,8224125441,3132459796,
    1206578334,4137487309,8964922173,9473445109,4809625778,
    4849437086,4721150682,3345701515,1041142623,1339590916,
    9774797484,4562717759,7168156627,9121096581,1029655705,
    8978224381,9766391248,4574389695,1881571879,6095802188,
    3881413435,2887806546,1424271774,7700999872,7767641100,
    3366805554,9921405536,8976056382,4441273777,2736220871,
    3905860130,4391056523,1539825432,7455477683,7986574261,
    1269293764,5479657752,6397291419,6726178355,7297417335,
    2370971360,1953451053,5082948570,2056291614,8063059843,
    9562911551,3687362220,3706349963,3403285329,5768180525,
    7467159543,6163697569,1773300867,1648754008,2954208298,
    6985487691,5174214319,4631188788,5282514054,1930250216,
    2310964958,7880719536,2376234564,9443165002,1899219325,
    5786790761,8443911838,3637710535,6124299846,3455263264,
    3667979387,6151233711,8235059969,1026413425,8784808289,
    1877247976,1876248090,4382969663,2924202947,7836502021,
    8193547738,1328083179,4722095287,3000136154,2113134369,
    7256299274,2237664384,8913278916,2898297233,1000991353,
    9653806178,2237137597,3858941027,1496321834,8648603076,
    7369414585,7979656920,4447460290,5311359797,2555359361,
    8293959675,6537528623,1976642180,8743122259,9533043079,
    2709288417,3839513474,1011477529,9568753725,7536358499,
    4938876854,9305126326,4601110260,8348454683,2929102556,
    2556915554,4915040773,1875085656,5134282577,1669088870,
    2651241089,2848202207,5910928381,1623372461,1928953917,
    4108144283,9704242134
};

#ifdef __AVX512F__  
  #define COLSIZE 32
#else 
  #define COLSIZE 64
#endif

static reg cols[COLSIZE];

#define ROTR(a,b) ((frt[a] >> b) | (frt[a] << (( (-b) & 63))))
#define QR(a,b,c,d) (	\
	frt[a] += frt[b], frt[d] ^= frt[a], ROTR(d, 32),	\
	frt[c] += frt[d], frt[b] ^= frt[c], ROTR(b, 24),	\
	frt[a] += frt[b], frt[d] ^= frt[a], ROTR(d, 16),	\
	frt[c] += frt[d], frt[b] ^= frt[c], ROTR(b, 63)   \
)

u64 CSPRNG::get(u8 ind) { u64 tmp = 0; SWAP(tmp, frt[ind]); --size; return (tmp >> (64 - precision)); }

void CSPRNG::undulate() {
  #ifdef __AVX512F__
    alignas(256) static const u8 idx[32] = {
      // COLUMNS 1-4
      4,5,6,7,
      // COLUMNS 9-12
      12,13,14,15,
      // COLUMNS 3-4
      2,3,
      // COLUMNS 7-8
      6,7,
      // COLUMNS 11-12
      10,11,
      // COLUMNS 15-16
      14,15,
      // ALTERNATE
      1,3,5,7,9,11,13,15,
      // shift_count stored here so we can align
      1,2,3,4,5,6,7,8
    };
    const __m512i shift_count = REG_LOADBITS((reg *) &idx[24]);
    // avoid both branching and multiplication by 
    // conditionally setting/clearing polarity inversion 
    u8 invert = 4 >> cycle;
    cycle <<= 3;
    cols[idx[cycle]     - invert] = REG_SRLV64(cols[idx[cycle]     - invert], shift_count);
    cols[idx[cycle + 1] - invert] = REG_SRLV64(cols[idx[cycle + 1] - invert], shift_count);
    cols[idx[cycle + 2] - invert] = REG_SRLV64(cols[idx[cycle + 2] - invert], shift_count);
    cols[idx[cycle + 3] - invert] = REG_SRLV64(cols[idx[cycle + 3] - invert], shift_count);
    cols[idx[cycle + 4] - invert] = REG_SRLV64(cols[idx[cycle + 4] - invert], shift_count);
    cols[idx[cycle + 5] - invert] = REG_SRLV64(cols[idx[cycle + 5] - invert], shift_count);
    cols[idx[cycle + 6] - invert] = REG_SRLV64(cols[idx[cycle + 6] - invert], shift_count);
    cols[idx[cycle + 7] - invert] = REG_SRLV64(cols[idx[cycle + 7] - invert], shift_count);
  #else 
    alignas(512) static u8 idx[64] = {
      // COLUMNS 1-4
      8,9,10,11,12,13,14,15,
      // COLUMNS 9-12
      24,25,26,27,28,29,30,31,
      // COLUMNS 3-4
      4,5,6,7,
      // COLUMNS 7-8
      12,13,14,15,
      // COLUMNS 11-12
      20,21,22,23,
      // COLUMNS 15-16
      28,29,30,31,
      // ALTERNATE
      2,3,6,7,10,11,14,15,18,19,22,23,26,27,30,31
    };  
    const reg shift1 = REG_SETR64(1,2,3,4);
    const reg shift2 = REG_SETR64(5,6,7,8);
    // avoid both branching and multiplication by 
    // conditionally setting/clearing polarity inversion 
    __m128i invec = _mm_set1_epi8((flip << 3) >> (cycle + cycle >> 1));
    __m128i tmp = _mm_load_si128((__m128i *) &idx[cycle << 4]);
    tmp = _mm_sub_epi8(tmp, invec);
    _mm_store_si128((__m128i *) &idx[48], tmp); 

    cols[idx[48]] = REG_SRLV64(cols[idx[48]],  shift1);
    cols[idx[49]] = REG_SRLV64(cols[idx[49]],  shift2);
    cols[idx[50]] = REG_SRLV64(cols[idx[50]],  shift1);
    cols[idx[51]] = REG_SRLV64(cols[idx[51]],  shift2);
    cols[idx[52]] = REG_SRLV64(cols[idx[52]],  shift1);
    cols[idx[53]] = REG_SRLV64(cols[idx[53]],  shift2);
    cols[idx[54]] = REG_SRLV64(cols[idx[54]],  shift1);
    cols[idx[55]] = REG_SRLV64(cols[idx[55]],  shift2);
    cols[idx[56]] = REG_SRLV64(cols[idx[56]],  shift1);
    cols[idx[57]] = REG_SRLV64(cols[idx[57]],  shift2);
    cols[idx[58]] = REG_SRLV64(cols[idx[58]],  shift1);
    cols[idx[59]] = REG_SRLV64(cols[idx[59]],  shift2);
    cols[idx[60]] = REG_SRLV64(cols[idx[60]],  shift1);
    cols[idx[61]] = REG_SRLV64(cols[idx[61]],  shift2);
    cols[idx[62]] = REG_SRLV64(cols[idx[62]],  shift1);
    cols[idx[63]] = REG_SRLV64(cols[idx[63]],  shift2);
  #endif
}

void CSPRNG::augshift() {
  #ifdef __AVX512F__
    alignas(128) static const shift_idx[16] = {
      6,7,0,1,2,3,4,5,2,3,4,5,6,7,0,1
    };
    const reg down_idx = REG_LOADBITS(&shift_idx[0]);
    const reg up_idx = REG_LOADBITS(&shift_idx[8]);
    cols[0]  = REG_PERM64(down_idx, cols[0]);
    cols[1]  = REG_PERM64(up_idx,   cols[1]);
    cols[2]  = REG_PERM64(down_idx, cols[2]);
    cols[3]  = REG_PERM64(up_idx,   cols[3]);
    cols[4]  = REG_PERM64(down_idx, cols[4]);
    cols[5]  = REG_PERM64(up_idx,   cols[5]);
    cols[6]  = REG_PERM64(down_idx, cols[6]);
    cols[7]  = REG_PERM64(up_idx,   cols[7]);
    cols[8]  = REG_PERM64(down_idx, cols[8]);
    cols[9]  = REG_PERM64(up_idx,   cols[8]);
    cols[10] = REG_PERM64(down_idx, cols[10]);
    cols[11] = REG_PERM64(up_idx,   cols[11]);
    cols[12] = REG_PERM64(down_idx, cols[12]);
    cols[13] = REG_PERM64(up_idx,   cols[13]);
    cols[14] = REG_PERM64(down_idx, cols[14]);
    cols[15] = REG_PERM64(up_idx,   cols[15]);
  #else
    reg aa, bb, cc, dd, ee, ff, gg, hh;
    for (int i {0}; i<32; i+=8) {
      aa = REG_PERM2X128(cols[1+i], cols[i],    67);
      bb = REG_PERM2X128(cols[i],   cols[1+i],  67);
      cc = REG_PERM2X128(cols[3+i], cols[2+i],  67);
      dd = REG_PERM2X128(cols[2+i], cols[3+i],  67);
      ee = REG_PERM2X128(cols[5+i], cols[4+i],  67);
      ff = REG_PERM2X128(cols[4+i], cols[5+i],  67);
      gg = REG_PERM2X128(cols[7+i], cols[6+i],  67);
      hh = REG_PERM2X128(cols[6+i], cols[7+i],  67);
      cols[i] = aa; cols[1+i] = bb; cols[2+i] = cc; cols[3+i] = dd;
      cols[4+i] = ee; cols[5+i] = ff; cols[6+i] = gg; cols[7+i] = hh;
    }
  #endif
}

u8 CSPRNG::generate() {
  accumulate();
  diffuse();
  augment();
  mangle();
  mangle();
  mangle();
  mangle();
  mangle();
  mangle();
  mangle();
  
  int i{7};

  while (i++ < rounds);
    mangle();

  return 0;
}

void CSPRNG::accumulate() {
  /*  
    start by supplying the initialization vectors
    8 u64's that correspond to the quote:
    "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
  */ 
  frt[4]   = 0x4265206672756974;
  frt[12]  = 0x66756C20616E6420;
  frt[48]  = 0x6D756C7469706C79;
  frt[56]  = 0x2C20616E64207265;
  frt[68]  = 0x706C656E69736820;
  frt[76]  = 0x7468652065617274;
  frt[112] = 0x68202847656E6573;
  frt[120] = 0x697320313A323829;
  // Highest starting pos for reading seed
  const u16 MAX_START = TAU_BLOCKS - 1;
  // initialize nonce from a tau digit sequence XOR'd with a TRN
  frt[7]   = TAU[trng16() & MAX_START] ^ trng64();
  frt[15]  = frt[7] + 1;
  frt[51]  = frt[7] + 2;
  frt[59]  = frt[7] + 3;
  frt[71]  = frt[7] + 4;
  frt[79]  = frt[7] + 5;
  frt[115] = frt[7] + 6;
  frt[123] = frt[7] + 7;
  // get start pos for reading from genesis
  u16 gidx = trng16() & MAX_START;
  // fill the fruit buffer with the seed!
  // BLOCK 1
  seed_block(0,1,2,3,16,17,18,19,32,33,34,35,49,50,gidx,frt[7]);
  // BLOCK 2
  seed_block(5,6,20,21,22,23,36,37,38,39,52,53,54,55,gidx,frt[15]);
  // BLOCK 3
  seed_block(8,9,10,11,24,25,26,27,40,41,42,43,57,58,gidx,frt[51]);
  // BLOCK 4
  seed_block(13,14,28,29,30,31,44,45,46,47,60,61,62,63,gidx,frt[59]);
  // BLOCK 5
  seed_block(64,65,66,67,80,81,82,83,96,97,98,99,113,114,gidx,frt[71]);
  // BLOCK 6
  seed_block(69,70,84,85,86,87,100,101,102,103,116,117,118,119,gidx,frt[79]);
  // BLOCK 7
  seed_block(72,73,74,75,88,89,90,91,104,105,106,107,121,122,gidx,frt[115]);
  // BLOCK 8
  seed_block(77,78,92,93,94,95,108,109,110,111,124,125,126,127,gidx,frt[123]);
}

void CSPRNG::diffuse() {
  // initialize GOLDEN RATIO values AND THEN MIX!
  u64 a,b,c,d,e,f,g,h;
  a=b=c=d=e=f=g=h=0x9e3779b97f4a7c13LL;
  isaac_mix(a,b,c,d,e,f,g,h); 
  isaac_mix(a,b,c,d,e,f,g,h);
  isaac_mix(a,b,c,d,e,f,g,h); 
  isaac_mix(a,b,c,d,e,f,g,h);
  // SIMD time baby!
  #ifdef __AVX512F__
    reg grm, m1, m2;
    for (int i{0}; i < FRUIT_SIZE / 2; i+=16) {
      // mix GR
      grm = REG_SETR64(a,b,c,d,e,f,g,h);
      // load 1024 bits of integer data
      m1 = REG_LOADBITS((reg*) &frt[i]); 
      m2 = REG_LOADBITS((reg*) &frt[i+8]);
      // apply the mix
      m1 = REG_ADD64(m1, grm);
      m2 = REG_ADD64(m2, grm);
      // store mixed results (note the index flip)
      REG_STOREBITS((reg*) &frt[i+8], m1);
      REG_STOREBITS((reg*) &frt[i], m2);
      isaac_mix(a,b,c,d,e,f,g,h);
    }
    // initialize columns
    reg c1  = REG_SETR64(frt[0], frt[16],frt[32],frt[48],frt[64],frt[80],frt[96], frt[112]);
    reg c2  = REG_SETR64(frt[1], frt[17],frt[33],frt[49],frt[65],frt[81],frt[97], frt[113]);
    reg c3  = REG_SETR64(frt[2], frt[18],frt[34],frt[50],frt[66],frt[82],frt[98], frt[114]);
    reg c4  = REG_SETR64(frt[3], frt[19],frt[35],frt[51],frt[67],frt[83],frt[99], frt[115]);
    reg c5  = REG_SETR64(frt[4], frt[20],frt[36],frt[52],frt[68],frt[84],frt[100],frt[115]);
    reg c6  = REG_SETR64(frt[5], frt[21],frt[37],frt[53],frt[69],frt[85],frt[101],frt[116]);
    reg c7  = REG_SETR64(frt[6], frt[22],frt[38],frt[54],frt[70],frt[86],frt[102],frt[117]);
    reg c8  = REG_SETR64(frt[7], frt[23],frt[39],frt[55],frt[71],frt[87],frt[103],frt[118]);
    reg c9  = REG_SETR64(frt[8], frt[24],frt[40],frt[56],frt[72],frt[88],frt[104],frt[119]);
    reg c10 = REG_SETR64(frt[9], frt[25],frt[41],frt[57],frt[73],frt[89],frt[105],frt[120]);
    reg c11 = REG_SETR64(frt[10],frt[26],frt[42],frt[58],frt[74],frt[90],frt[106],frt[121]);
    reg c12 = REG_SETR64(frt[11],frt[27],frt[43],frt[59],frt[75],frt[91],frt[107],frt[122]);
    reg c13 = REG_SETR64(frt[12],frt[28],frt[44],frt[60],frt[76],frt[92],frt[108],frt[123]);
    reg c14 = REG_SETR64(frt[13],frt[29],frt[45],frt[61],frt[77],frt[93],frt[109],frt[124]);
    reg c15 = REG_SETR64(frt[14],frt[30],frt[46],frt[62],frt[78],frt[94],frt[110],frt[125]);
    reg c16 = REG_SETR64(frt[15],frt[31],frt[47],frt[63],frt[79],frt[95],frt[111],frt[126]);
    // now swap and work your way inward
    cols[0]  = REG_MBLEND64(0x80, c16, c1);
    cols[15] = REG_MBLEND64(0x80, c1, c16);
    cols[1]  = REG_MBLEND64(0x80, c15, c2);
    cols[14] = REG_MBLEND64(0x80, c2, c15);
    cols[2]  = REG_MBLEND64(0x80, c14, c3);
    cols[13] = REG_MBLEND64(0x80, c3, c14);
    cols[3]  = REG_MBLEND64(0x80, c13, c4);
    cols[12] = REG_MBLEND64(0x80, c4, c13);
    cols[4]  = REG_MBLEND64(0x80, c12, c5);
    cols[11] = REG_MBLEND64(0x80, c5, c12);
    cols[5]  = REG_MBLEND64(0x80, c11, c6);
    cols[10] = REG_MBLEND64(0x80, c6, c11);
    cols[6]  = REG_MBLEND64(0x80, c10, c7);
    cols[9]  = REG_MBLEND64(0x80, c7, c10);
    cols[7]  = REG_MBLEND64(0x80, c9, c8);
    cols[8]  = REG_MBLEND64(0x80, c8, c9);
  #else
    reg gr1, gr2, m1, m2, m3, m4;
    int i{0};
    for (; i < FRUIT_SIZE / 8; i+=16) {
      // mix GR
      gr1 = REG_SETR64(a,b,c,d);
      gr2 = REG_SETR64(e,f,g,h);
      // load 512 bits of integer data
      m1 = REG_LOADBITS((reg*) &frt[i]); 
      m2 = REG_LOADBITS((reg*) &frt[i+4]);
      m3 = REG_LOADBITS((reg*) &frt[i+8]); 
      m4 = REG_LOADBITS((reg*) &frt[i+12]);
      // apply the mix
      m1 = REG_ADD64(m1, gr1);
      m2 = REG_ADD64(m2, gr2);
      m3 = REG_ADD64(m3, gr1);
      m4 = REG_ADD64(m4, gr2);
      // store mixed results (note the index flip)
      REG_STOREBITS((reg*) &frt[i+12],m1);
      REG_STOREBITS((reg*) &frt[i+8], m2);
      REG_STOREBITS((reg*) &frt[i+4], m3);
      REG_STOREBITS((reg*) &frt[i],   m4);
      isaac_mix(a,b,c,d,e,f,g,h);
    }

    /*  initialize columns - loaded in halves unlike above with _mm512
        this means that c1-c8 holds columns 1-4 and c9-c16 holds columns 12-16
        columns are then swapped working towards the inside
    */

    reg c1  = REG_SETR64(frt[0], frt[16],frt[32], frt[48]);
    reg c2  = REG_SETR64(frt[64],frt[80],frt[96], frt[112]);
    reg c3  = REG_SETR64(frt[1], frt[17],frt[33], frt[49]);
    reg c4  = REG_SETR64(frt[65],frt[81],frt[97], frt[113]);
    reg c5  = REG_SETR64(frt[2], frt[18],frt[34], frt[50]);
    reg c6  = REG_SETR64(frt[66],frt[82],frt[98], frt[114]);
    reg c7  = REG_SETR64(frt[3], frt[19],frt[35], frt[51]);
    reg c8  = REG_SETR64(frt[67],frt[83],frt[99], frt[115]);
    reg c9  = REG_SETR64(frt[4], frt[20],frt[36], frt[52]);
    reg c10 = REG_SETR64(frt[68],frt[84],frt[100],frt[115]);
    reg c11 = REG_SETR64(frt[5], frt[21],frt[37], frt[53]);
    reg c12 = REG_SETR64(frt[69],frt[85],frt[101],frt[116]);
    reg c13 = REG_SETR64(frt[6], frt[22],frt[38], frt[54]);
    reg c14 = REG_SETR64(frt[70],frt[86],frt[102],frt[117]);
    reg c15 = REG_SETR64(frt[7], frt[23],frt[39], frt[55]);
    reg c16 = REG_SETR64(frt[71],frt[87],frt[103],frt[118]);
    reg c17 = REG_SETR64(frt[8], frt[24],frt[40], frt[56]);
    reg c18 = REG_SETR64(frt[72],frt[88],frt[104],frt[119]);
    reg c19 = REG_SETR64(frt[9], frt[25],frt[41], frt[57]);
    reg c20 = REG_SETR64(frt[73],frt[89],frt[105],frt[120]);
    reg c21 = REG_SETR64(frt[10],frt[26],frt[42], frt[58]);
    reg c22 = REG_SETR64(frt[74],frt[90],frt[106],frt[121]);
    reg c23 = REG_SETR64(frt[11],frt[27],frt[43], frt[59]);
    reg c24 = REG_SETR64(frt[75],frt[91],frt[107],frt[122]);
    reg c25 = REG_SETR64(frt[12],frt[28],frt[44], frt[60]);
    reg c26 = REG_SETR64(frt[76],frt[92],frt[108],frt[123]);
    reg c27 = REG_SETR64(frt[13],frt[29],frt[45], frt[61]);
    reg c28 = REG_SETR64(frt[77],frt[93],frt[109],frt[124]);
    reg c29 = REG_SETR64(frt[14],frt[30],frt[46], frt[62]);
    reg c30 = REG_SETR64(frt[78],frt[94],frt[110],frt[125]);
    reg c31 = REG_SETR64(frt[15],frt[31],frt[47], frt[63]);
    reg c32 = REG_SETR64(frt[79],frt[95],frt[111],frt[126]);

    // now swap opposing columns (top with top, bottom with bottom)
    // vectorize me cap'n!

    cols[0]  = REG_BLEND32(c31, c1,  0x80);
    cols[30] = REG_BLEND32(c1,  c31, 0x80);
    cols[1]  = REG_BLEND32(c32, c2,  0x80);
    cols[31] = REG_BLEND32(c2,  c32, 0x80);
    cols[2]  = REG_BLEND32(c29, c3,  0x80);
    cols[28] = REG_BLEND32(c3,  c29, 0x80);
    cols[3]  = REG_BLEND32(c30, c4,  0x80);
    cols[29] = REG_BLEND32(c4,  c30, 0x80);
    cols[4]  = REG_BLEND32(c27, c5,  0x80);
    cols[26] = REG_BLEND32(c5,  c27, 0x80);
    cols[5]  = REG_BLEND32(c28, c6,  0x80);
    cols[27] = REG_BLEND32(c6,  c28, 0x80);
    cols[6]  = REG_BLEND32(c25, c7,  0x80);
    cols[24] = REG_BLEND32(c7,  c25, 0x80);
    cols[7]  = REG_BLEND32(c26, c8,  0x80);

    cols[25] = REG_BLEND32(c8,  c26, 0x80);
    cols[8]  = REG_BLEND32(c23, c9,  0x80);
    cols[22] = REG_BLEND32(c9,  c23, 0x80);
    cols[9]  = REG_BLEND32(c24, c10, 0x80);
    cols[23] = REG_BLEND32(c10, c24, 0x80);
    cols[10] = REG_BLEND32(c21, c11, 0x80);
    cols[20] = REG_BLEND32(c11, c21, 0x80);
    cols[11] = REG_BLEND32(c22, c12, 0x80);
    cols[21] = REG_BLEND32(c12, c22, 0x80);
    cols[12] = REG_BLEND32(c19, c13, 0x80);
    cols[18] = REG_BLEND32(c13, c19, 0x80);
    cols[13] = REG_BLEND32(c20, c14, 0x80);
    cols[19] = REG_BLEND32(c14, c20, 0x80);
    cols[14] = REG_BLEND32(c17, c15, 0x80);
    cols[16] = REG_BLEND32(c15, c17, 0x80);
    cols[15] = REG_BLEND32(c18, c16, 0x80);
    cols[17] = REG_BLEND32(c16, c18, 0x80);
  #endif  
}

void CSPRNG::augment() {
  /*
    PHASE 1: UNDULATION
    Shift, compress, and then shift again.
  */ 

  // time to get shwifty
  augshift();
  // now shift bits in a way that resembles a wave
  undulate();
  // time to get shwifty (again)
  augshift(); 

  /*
    PHASE 2: SUMMATION
    For every 4 columns, columns 1 and 3 are summed, 
    then 2 and 4, then 2 and 3, then 1 and 4.
  */ 

  #ifdef __AVX512F__
    // COLUMNS 1-4 -> COLUMNS 17-20
    cols[16] = REG_ADD64(cols[0], cols[2]);
    cols[17] = REG_ADD64(cols[1], cols[3]);
    cols[18] = REG_ADD64(cols[1], cols[2]);
    cols[19] = REG_ADD64(cols[0], cols[3]);
    // COLUMNS 5-8 -> COLUMNS 21-24
    cols[20] = REG_ADD64(cols[4], cols[6]);
    cols[21] = REG_ADD64(cols[5], cols[7]);
    cols[22] = REG_ADD64(cols[5], cols[6]);
    cols[23] = REG_ADD64(cols[4], cols[7]);
    // COLUMNS 9-12 -> COLUMNS 25-28
    cols[24] = REG_ADD64(cols[8],  cols[10]);
    cols[25] = REG_ADD64(cols[9], cols[11]);
    cols[26] = REG_ADD64(cols[9], cols[10]);
    cols[27] = REG_ADD64(cols[8],  cols[11]);
    // COLUMNS 13-16 -> COLUMNS 29-32
    cols[28] = REG_ADD64(cols[12], cols[14]);
    cols[29] = REG_ADD64(cols[13], cols[15]);
    cols[30] = REG_ADD64(cols[13], cols[14]);
    cols[31] = REG_ADD64(cols[12], cols[15]);
  #else
    // COLUMNS 1-4 -> COLUMNS 17-20
    cols[32] = REG_ADD64(cols[0], cols[4]);
    cols[33] = REG_ADD64(cols[1], cols[5]);
    cols[34] = REG_ADD64(cols[2], cols[6]);
    cols[35] = REG_ADD64(cols[3], cols[7]);
    cols[36] = REG_ADD64(cols[2], cols[4]);
    cols[37] = REG_ADD64(cols[3], cols[5]);
    cols[38] = REG_ADD64(cols[0], cols[6]);
    cols[39] = REG_ADD64(cols[1], cols[7]);
    // COLUMNS 5-8 -> COLUMNS 21-24
    cols[40] = REG_ADD64(cols[8],  cols[12]);
    cols[41] = REG_ADD64(cols[9],  cols[13]);
    cols[42] = REG_ADD64(cols[10], cols[14]);
    cols[43] = REG_ADD64(cols[11], cols[15]);
    cols[44] = REG_ADD64(cols[10], cols[12]);
    cols[45] = REG_ADD64(cols[11], cols[13]);
    cols[46] = REG_ADD64(cols[8],  cols[14]);
    cols[47] = REG_ADD64(cols[9],  cols[15]);
    // COLUMNS 9-12 -> COLUMNS 25-28
    cols[48] = REG_ADD64(cols[16], cols[20]);
    cols[49] = REG_ADD64(cols[17], cols[21]);
    cols[50] = REG_ADD64(cols[18], cols[22]);
    cols[51] = REG_ADD64(cols[19], cols[23]);
    cols[52] = REG_ADD64(cols[18], cols[20]);
    cols[53] = REG_ADD64(cols[19], cols[21]);
    cols[54] = REG_ADD64(cols[16], cols[22]);
    cols[55] = REG_ADD64(cols[17], cols[23]);
    // COLUMNS 13-16 -> COLUMNS 29-32
    cols[56] = REG_ADD64(cols[24], cols[28]);
    cols[57] = REG_ADD64(cols[25], cols[29]);
    cols[58] = REG_ADD64(cols[26], cols[30]);
    cols[59] = REG_ADD64(cols[27], cols[31]);
    cols[60] = REG_ADD64(cols[26], cols[28]);
    cols[61] = REG_ADD64(cols[27], cols[29]);
    cols[62] = REG_ADD64(cols[24], cols[30]);
    cols[63] = REG_ADD64(cols[25], cols[31]);
  #endif

  /*
    PHASE 3: ASSIMILATION
    Rearrange and blend the parent and child sets
  */ 

  // I CALL THIS ONE THE HURACAN SHIFT
  #ifdef __AVX512F__
    static const reg p1 = REG_SETR64(0,5,4,1,1,6,2,2);
    static const reg p2 = REG_SETR64(4,4,1,6,2,3,3,7);
    // OLD SET
    cols[0]  = REG_PERM64(p1, cols[0]);
    cols[1]  = REG_PERM64(p2, cols[1]);
    cols[2]  = REG_PERM64(p1, cols[2]);
    cols[3]  = REG_PERM64(p2, cols[3]);
    cols[4]  = REG_PERM64(p1, cols[4]);
    cols[5]  = REG_PERM64(p2, cols[5]);
    cols[6]  = REG_PERM64(p1, cols[6]);
    cols[7]  = REG_PERM64(p2, cols[7]);
    cols[8]  = REG_PERM64(p1, cols[8]);
    cols[9]  = REG_PERM64(p2, cols[9]);
    cols[10] = REG_PERM64(p1, cols[10]);
    cols[11] = REG_PERM64(p2, cols[11]);
    cols[12] = REG_PERM64(p1, cols[12]);
    cols[13] = REG_PERM64(p2, cols[13]);
    cols[14] = REG_PERM64(p1, cols[14]);
    cols[15] = REG_PERM64(p2, cols[15]);
    // NEW SET
    cols[16] = REG_PERM64(p1, cols[16]);
    cols[17] = REG_PERM64(p2, cols[17]);
    cols[18] = REG_PERM64(p1, cols[18]);
    cols[19] = REG_PERM64(p2, cols[19]);
    cols[20] = REG_PERM64(p1, cols[20]);
    cols[21] = REG_PERM64(p2, cols[21]);
    cols[22] = REG_PERM64(p1, cols[22]);
    cols[23] = REG_PERM64(p2, cols[23]);
    cols[24] = REG_PERM64(p1, cols[24]);
    cols[25] = REG_PERM64(p2, cols[25]);
    cols[26] = REG_PERM64(p1, cols[26]);
    cols[27] = REG_PERM64(p2, cols[27]);
    cols[28] = REG_PERM64(p1, cols[28]);
    cols[29] = REG_PERM64(p2, cols[29]);
    cols[30] = REG_PERM64(p1, cols[30]);
    cols[31] = REG_PERM64(p2, cols[31]);
    //  store the results contiguously - this is 
    //  like a counter clockwise 90° rotation of sorts
    REG_STORE64((reg*) &frt[0],  cols[0]);
    REG_STORE64((reg*) &frt[8],  cols[1]);
    REG_STORE64((reg*) &frt[16], cols[2]);
    REG_STORE64((reg*) &frt[24], cols[3]);
    REG_STORE64((reg*) &frt[32], cols[4]);
    REG_STORE64((reg*) &frt[40], cols[5]);
    REG_STORE64((reg*) &frt[48], cols[6]);
    REG_STORE64((reg*) &frt[56], cols[7]);
    REG_STORE64((reg*) &frt[64], cols[8]);
    REG_STORE64((reg*) &frt[72], cols[9]);
    REG_STORE64((reg*) &frt[80], cols[10]);
    REG_STORE64((reg*) &frt[88], cols[11]);
    REG_STORE64((reg*) &frt[96], cols[12]);
    REG_STORE64((reg*) &frt[104],cols[13]);
    REG_STORE64((reg*) &frt[112],cols[14]);
    REG_STORE64((reg*) &frt[120],cols[15]);
    REG_STORE64((reg*) &frt[128],cols[16]);
    REG_STORE64((reg*) &frt[136],cols[17]);
    REG_STORE64((reg*) &frt[144],cols[18]);
    REG_STORE64((reg*) &frt[152],cols[19]);
    REG_STORE64((reg*) &frt[160],cols[20]);
    REG_STORE64((reg*) &frt[168],cols[21]);
    REG_STORE64((reg*) &frt[176],cols[22]);
    REG_STORE64((reg*) &frt[184],cols[23]);
    REG_STORE64((reg*) &frt[192],cols[24]);
    REG_STORE64((reg*) &frt[200],cols[25]);
    REG_STORE64((reg*) &frt[208],cols[26]);
    REG_STORE64((reg*) &frt[216],cols[27]);
    REG_STORE64((reg*) &frt[224],cols[28]);
    REG_STORE64((reg*) &frt[232],cols[29]);
    REG_STORE64((reg*) &frt[240],cols[30]);
    REG_STORE64((reg*) &frt[248],cols[31]);
  #else
    // The uglier Huracan Shift :(
    static const reg s1 = REG_SETR32(0,1,6,7,4,5,2,3);
    static const reg s2 = REG_SETR32(0,1,4,5,6,7,6,7);
    static const reg s3 = REG_SETR32(0,1,0,1,4,5,2,3);
    static const reg s4 = REG_SETR32(0,1,2,3,2,3,6,7);

    /*  now recreate the orders (0,5,4,1,1,6,2,2) and (4,4,1,6,2,3,3,7)
        in an alternating fashion. This is the final permutation of 
        elements before blending. */
    
    // OLD SET
    u64 t1, t2, t3, t4, t5, t6, t7, t8;
    u8 i{0};
    for (; i < 32; i+=8) {
      // COLUMN 1
      t1        = REG_EXT64(cols[i], 1);
      t2        = REG_EXT64(cols[i], 2);
      cols[i]   = REG_PERM2X128(cols[i], cols[1+i], 100);
      cols[1+i] = REG_INS64(cols[1+i], t2, 3);
      cols[i]   = REG_PERM8X32(cols[i], s1);
      cols[1+i] = REG_INS64(cols[1+i], t1, 0);
      cols[1+i] = REG_PERM8X32(cols[i], s2);
      // COLUMN 2
      t3        = REG_EXT64(cols[2+i], 2);
      t4        = REG_EXT64(cols[2+i], 3);
      cols[3+i] = REG_INS64(cols[3+i], t4, 1);
      t4        = REG_EXT64(cols[2+i], 1);
      cols[2+i] = REG_PERM2X128(cols[2+i], cols[3+i], 114);
      cols[3+i] = REG_INS64(cols[3+i], t3, 0);
      cols[2+i] = REG_INS64(cols[2+i], t4, 2);
      cols[3+i] = REG_PERM8X32(cols[3+i], s4);
      cols[2+i] = REG_PERM8X32(cols[2+i], s3);
      // COLUMN 3
      t5        = REG_EXT64(cols[4+i], 1);
      t6        = REG_EXT64(cols[4+i], 2);
      cols[4+i] = REG_PERM2X128(cols[4+i], cols[5+i], 100);
      cols[5+i] = REG_INS64(cols[5+i], t6, 3);
      cols[4+i] = REG_PERM8X32(cols[4+i], s1);
      cols[5+i] = REG_INS64(cols[5+i], t5, 0);
      cols[5+i] = REG_PERM8X32(cols[4+i], s2);
      // COLUMN 4
      t7        = REG_EXT64(cols[6+i], 2);
      t8        = REG_EXT64(cols[6+i], 3);
      cols[7+i] = REG_INS64(cols[7+i], t8, 1);
      t8        = REG_EXT64(cols[6+i], 1);
      cols[6+i] = REG_PERM2X128(cols[6+i], cols[7+i], 114);
      cols[7+i] = REG_INS64(cols[7+i], t7, 0);
      cols[6+i] = REG_INS64(cols[6+i], t8, 2);
      cols[7+i] = REG_PERM8X32(cols[7+i], s4);
      cols[6+i] = REG_PERM8X32(cols[6+i], s3);
    }
    // NEW SET
    u64 t9, t10, t11, t12, t13, t14, t15, t16;
    for (u8 j{32}; j < 64; j+=8) {
      // COLUMN 1
      t9        = REG_EXT64(cols[j], 1);
      t10       = REG_EXT64(cols[j], 2);
      cols[j]   = REG_PERM2X128(cols[j], cols[1+j], 100);
      cols[1+j] = REG_INS64(cols[1+j], t10, 3);
      cols[j]   = REG_PERM8X32(cols[j], s1);
      cols[1+j] = REG_INS64(cols[1+j], t9, 0);
      cols[1+j] = REG_PERM8X32(cols[j], s2);
      // COLUMN 2
      t11       = REG_EXT64(cols[2+j], 2);
      t12       = REG_EXT64(cols[2+j], 3);
      cols[3+j] = REG_INS64(cols[3+j], t12, 1);
      t12       = REG_EXT64(cols[2+j], 1);
      cols[2+j] = REG_PERM2X128(cols[2+j], cols[3+j], 114);
      cols[3+j] = REG_INS64(cols[3+j], t11, 0);
      cols[2+j] = REG_INS64(cols[2+j], t12, 2);
      cols[3+j] = REG_PERM8X32(cols[3+j], s4);
      cols[2+j] = REG_PERM8X32(cols[2+j], s3);
      // COLUMN 3
      t13       = REG_EXT64(cols[4+j], 1);
      t14       = REG_EXT64(cols[4+j], 2);
      cols[4+j] = REG_PERM2X128(cols[4+j], cols[5+j], 100);
      cols[5+j] = REG_INS64(cols[5+j], t14, 3);
      cols[4+j] = REG_PERM8X32(cols[4+j], s1);
      cols[5+j] = REG_INS64(cols[5+j], t13, 0);
      cols[5+j] = REG_PERM8X32(cols[4+j], s2);
      // COLUMN 4
      t15       = REG_EXT64(cols[6+j], 2);
      t16       = REG_EXT64(cols[6+j], 3);
      cols[7+j] = REG_INS64(cols[7+j], t16, 1);
      t16       = REG_EXT64(cols[6+j], 1);
      cols[6+j] = REG_PERM2X128(cols[6+j], cols[7+j], 114);
      cols[7+j] = REG_INS64(cols[7+j], t15, 0);
      cols[6+j] = REG_INS64(cols[6+j], t16, 2);
      cols[7+j] = REG_PERM8X32(cols[7+j], s4);
      cols[6+j] = REG_PERM8X32(cols[6+j], s3);
    }
    //  store the results contiguously - this is 
    //  like a counter clockwise 90° rotation of sorts
    u8 k{0};
    i = 0;  
    for (; i < 33; i+=32) {
      REG_STOREBITS((reg*) &frt[k],     cols[i]);
      REG_STOREBITS((reg*) &frt[4+k],   cols[1+i]);
      REG_STOREBITS((reg*) &frt[8+k],   cols[2+i]);
      REG_STOREBITS((reg*) &frt[12+k],  cols[3+i]);
      REG_STOREBITS((reg*) &frt[16+k],  cols[4+i]);
      REG_STOREBITS((reg*) &frt[20+k],  cols[5+i]);
      REG_STOREBITS((reg*) &frt[24+k],  cols[6+i]);
      REG_STOREBITS((reg*) &frt[28+k],  cols[7+i]);
      REG_STOREBITS((reg*) &frt[32+k],  cols[8+i]);
      REG_STOREBITS((reg*) &frt[36+k],  cols[9+i]);
      REG_STOREBITS((reg*) &frt[40+k],  cols[10+i]);
      REG_STOREBITS((reg*) &frt[44+k],  cols[11+i]);
      REG_STOREBITS((reg*) &frt[48+k],  cols[12+i]);
      REG_STOREBITS((reg*) &frt[52+k],  cols[13+i]);
      REG_STOREBITS((reg*) &frt[56+k],  cols[14+i]);
      REG_STOREBITS((reg*) &frt[60+k],  cols[15+i]);
      REG_STOREBITS((reg*) &frt[64+k],  cols[16+i]);
      REG_STOREBITS((reg*) &frt[68+k],  cols[17+i]);
      REG_STOREBITS((reg*) &frt[72+k],  cols[18+i]);
      REG_STOREBITS((reg*) &frt[76+k],  cols[19+i]);
      REG_STOREBITS((reg*) &frt[80+k],  cols[20+i]);
      REG_STOREBITS((reg*) &frt[84+k],  cols[21+i]);
      REG_STOREBITS((reg*) &frt[88+k],  cols[22+i]);
      REG_STOREBITS((reg*) &frt[92+k],  cols[23+i]);
      REG_STOREBITS((reg*) &frt[96+k],  cols[24+i]);
      REG_STOREBITS((reg*) &frt[100+k], cols[25+i]);
      REG_STOREBITS((reg*) &frt[104+k], cols[26+i]);
      REG_STOREBITS((reg*) &frt[108+k], cols[27+i]);
      REG_STOREBITS((reg*) &frt[112+k], cols[28+i]);
      REG_STOREBITS((reg*) &frt[116+k], cols[29+i]);
      REG_STOREBITS((reg*) &frt[120+k], cols[30+i]);
      REG_STOREBITS((reg*) &frt[124+k], cols[31+i]);
      k += 128;
    }
  #endif

  // now weave and consolidate

  // BLOCK 1                                // BLOCK 9
  adam_mix(0,1,2,3, 16,17,18,19,            128,129,130,131,144,145,146,147);
  adam_mix(32,33,34,35,48,49,50,51,         160,161,162,163,176,177,178,179);
  // BLOCK 2                                // BLOCK 10
  adam_mix(4,5,6,7, 20,21,22,23,            132,133,134,135,148,149,150,151);
  adam_mix(36,37,38,39,52,53,54,55,         164,165,166,167,180,181,182,183);
  // BLOCK 3                                // BLOCK 11
  adam_mix(8, 9, 10,11,24,25,26,27,         136,137,138,139,152,153,154,155);
  adam_mix(40,41,42,43,56,57,58,59,         168,169,170,171,184,185,186,187);
  // BLOCK 4                                // BLOCK 12
  adam_mix(12,13,14,15,28,29,30,31,         140,141,142,143,156,157,158,159);
  adam_mix(44,45,46,47,60,61,62,63,         172,173,174,175,188,189,190,191);
  // BLOCK 5                                // BLOCK 13
  adam_mix(64,65,66,67,80, 81, 82, 83,      192,193,194,195,208,209,210,211);
  adam_mix(96,97,98,99,112,113,114,115,     224,225,226,227,240,241,242,243);
  // BLOCK 6                                // BLOCK 14
  adam_mix(68, 69, 70, 71, 84, 85, 86, 87,  196,197,198,199,212,213,214,215);
  adam_mix(100,101,102,103,116,117,118,119, 228,229,230,231,244,245,246,247);
  // BLOCK 7                                // BLOCK 15
  adam_mix(72, 73, 74, 75, 88, 89, 90, 91,  200,201,202,203,216,217,218,219);
  adam_mix(104,105,106,107,120,121,122,123, 232,233,234,235,248,249,250,251);
  // BLOCK 8                                // BLOCK 16
  adam_mix(76, 77, 78, 79, 92, 93, 94, 95,  204,205,206,207,220,221,222,223);
  adam_mix(108,109,110,111,124,125,126,127, 236,237,238,239,252,253,254,255);
}

void CSPRNG::mangle() {
  /*  
    apply ChaCha20 quarter rounding function 4x per table.This also
    rotates right like BLAKE2b rather than left like the original ChaCha
  */
  
  // FIRST 2 ROUNDS - VERTICAL AND HORIZONTAL LIKE SALSA20

  // TABLE 1              TABLE 2                 TABLE 3                 TABLE 4
  QR(0,16,32,48);         QR(4,20,36,52);         QR(8,24,40,56);         QR(12,28,44,60);
  QR(17,33,49,1);         QR(21,37,53,5);         QR(25,41,57,9);         QR(29,45,61,13);
  QR(34,50,2,18);         QR(38,54,6,22);         QR(42,58,10,26);        QR(46,62,14,30);
  QR(51,3,19,35);         QR(55,7,23,39);         QR(59,11,27,43);        QR(63,15,31,47);

  QR(0,1,2,3);            QR(4,5,6,7);            QR(8,9,10,11);          QR(12,13,14,15);
  QR(17,18,19,16);        QR(21,22,23,20);        QR(25,26,27,24);        QR(29,30,31,28);
  QR(34,35,32,33);        QR(38,39,36,37);        QR(42,43,40,41);        QR(46,47,44,45);
  QR(51,48,49,50);        QR(55,52,53,54);        QR(59,56,57,58);        QR(63,60,61,62);
  
  // TABLE 5              TABLE 6                 TABLE 7                 TABLE 8
  QR(64,80,96,112);       QR(68,84,100,116);      QR(72,88,104,120);      QR(76,92,108,124);
  QR(81,97,113,65);       QR(85,101,117,69);      QR(73,89,105,121);      QR(93,109,125,77);
  QR(98,114,66,82);       QR(102,118,70,86);      QR(74,90,106,122);      QR(110,126,78,94);
  QR(115,67,83,99);       QR(119,71,87,103);      QR(75,91,107,123);      QR(127,79,95,111);
  
  QR(64,65,66,67);        QR(68,69,70,71);        QR(72,73,74,75);        QR(76,77,78,79);
  QR(81,82,83,80);        QR(85,86,87,84);        QR(89,90,91,88);        QR(93,94,95,92);
  QR(98,99,96,97);        QR(102,103,100,101);    QR(106,107,104,105);    QR(110,111,108,109);
  QR(115,112,113,114);    QR(119,116,117,118);    QR(123,120,121,122);    QR(127,124,125,126);
  
  // TABLE 9              TABLE 10                TABLE 11                TABLE 12
  QR(128,144,160,176);    QR(132,148,164,180);    QR(136,152,168,184);    QR(140,156,172,188);
  QR(145,161,177,129);    QR(149,165,181,133);    QR(153,169,185,137);    QR(157,173,189,141);
  QR(162,178,130,146);    QR(166,182,134,150);    QR(170,186,138,154);    QR(174,190,142,158);
  QR(179,131,147,163);    QR(183,135,151,167);    QR(187,139,155,171);    QR(191,143,159,175);

  QR(128,129,130,131);    QR(132,133,134,135);    QR(136,137,138,139);    QR(140,141,142,143);
  QR(145,146,147,144);    QR(149,150,151,148);    QR(153,154,155,152);    QR(157,158,159,156);
  QR(162,163,160,161);    QR(166,167,164,165);    QR(170,171,168,169);    QR(174,175,172,173);
  QR(179,176,177,178);    QR(183,180,181,182);    QR(187,184,185,186);    QR(191,188,189,190);

  // TABLE 13             TABLE 14                TABLE 15                TABLE 16
  QR(192,208,224,240);    QR(196,212,228,244);    QR(200,216,232,248);    QR(204,220,236,252);
  QR(209,225,241,193);    QR(213,229,245,197);    QR(217,233,249,201);    QR(221,237,253,205);
  QR(226,242,194,210);    QR(230,246,198,214);    QR(234,250,202,218);    QR(238,254,206,222);
  QR(243,195,211,227);    QR(247,199,215,231);    QR(251,203,219,235);    QR(255,207,223,239);
  
  QR(192,193,194,195);    QR(196,197,198,199);    QR(200,201,202,203);    QR(204,205,206,207);
  QR(209,210,211,208);    QR(213,214,215,212);    QR(217,218,219,216);    QR(221,222,223,220);
  QR(226,227,224,225);    QR(230,231,228,229);    QR(234,235,232,233);    QR(238,239,236,237);
  QR(243,240,241,242);    QR(247,244,245,246);    QR(251,248,249,250);    QR(255,252,253,254);

  // LAST 2 ROUNDS - VERTICAL AND DIAGONAL LIKE CHACHA20

  // TABLE 1              TABLE 2                 TABLE 3                 TABLE 4
  // QR(0,16,32,48);         QR(4,20,36,52);         QR(8,24,40,56);         QR(12,28,44,60);
  // QR(17,33,49,1);         QR(21,37,53,5);         QR(25,41,57,9);         QR(29,45,61,13);
  // QR(34,50,2,18);         QR(38,54,6,22);         QR(42,58,10,26);        QR(46,62,14,30);
  // QR(51,3,19,35);         QR(55,7,23,39);         QR(59,11,27,43);        QR(63,15,31,47);

  QR(0,17,34,51);         QR(4,21,38,55);         QR(8,25,42,59);         QR(12,29,46,63);
  QR(1,18,35,48);         QR(5,22,39,52);         QR(9,26,43,56);         QR(13,30,47,60);
  QR(2,19,32,49);         QR(6,23,36,53);         QR(10,27,40,57);        QR(14,31,44,61);
  QR(3,16,33,50);         QR(7,20,37,54);         QR(11,24,41,58);        QR(15,28,45,62);
  
  // TABLE 5              TABLE 6                 TABLE 7                 TABLE 8
  // QR(64,80,96,112);       QR(68,84,100,116);      QR(72,88,104,120);      QR(76,92,108,124);
  // QR(81,97,113,65);       QR(85,101,117,69);      QR(73,89,105,121);      QR(93,109,125,77);
  // QR(98,114,66,82);       QR(102,118,70,86);      QR(74,90,106,122);      QR(110,126,78,94);
  // QR(115,67,83,99);       QR(119,71,87,103);      QR(75,91,107,123);      QR(127,79,95,111);
  
  QR(64,81,98,115);       QR(68,85,102,119);      QR(72,89,106,123);      QR(76,93,110,127);
  QR(65,82,99,112);       QR(69,86,103,116);      QR(73,90,107,120);      QR(77,94,111,124);
  QR(66,83,96,113);       QR(70,87,100,117);      QR(74,91,104,121);      QR(78,95,108,125);
  QR(67,80,97,114);       QR(71,84,101,118);      QR(75,88,105,122);      QR(79,92,109,126);
  
  // TABLE 9              TABLE 10                TABLE 11                TABLE 12
  // QR(128,144,160,176);    QR(132,148,164,180);    QR(136,152,168,184);    QR(140,156,172,188);
  // QR(145,161,177,129);    QR(149,165,181,133);    QR(153,169,185,137);    QR(157,173,189,141);
  // QR(162,178,130,146);    QR(166,182,134,150);    QR(170,186,138,154);    QR(174,190,142,158);
  // QR(179,131,147,163);    QR(183,135,151,167);    QR(187,139,155,171);    QR(191,143,159,175);

  QR(128,145,162,179);    QR(132,149,166,183);    QR(136,153,170,187);    QR(140,157,174,191);
  QR(129,146,163,176);    QR(133,150,167,180);    QR(137,154,171,184);    QR(141,158,175,188);
  QR(130,147,160,177);    QR(134,151,164,181);    QR(138,155,168,185);    QR(142,159,172,189);
  QR(131,144,161,178);    QR(135,148,165,182);    QR(139,152,169,186);    QR(143,156,173,190);

  // TABLE 13             TABLE 14                TABLE 15                TABLE 16
  // QR(192,208,224,240);    QR(196,212,228,244);    QR(200,216,232,248);    QR(204,220,236,252);
  // QR(209,225,241,193);    QR(213,229,245,197);    QR(217,233,249,201);    QR(221,237,253,205);
  // QR(226,242,194,210);    QR(230,246,198,214);    QR(234,250,202,218);    QR(238,254,206,222);
  // QR(243,195,211,227);    QR(247,199,215,231);    QR(251,203,219,235);    QR(255,207,223,239);
  
  QR(192,209,226,243);    QR(196,213,230,247);    QR(200,217,234,251);    QR(204,221,238,255);
  QR(193,210,227,240);    QR(197,214,231,244);    QR(201,218,235,248);    QR(205,222,239,252);
  QR(194,211,224,241);    QR(198,215,228,245);    QR(202,219,232,249);    QR(206,223,236,253);
  QR(195,208,225,242);    QR(199,212,229,246);    QR(203,216,233,250);    QR(207,220,237,254);
}