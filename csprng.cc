#include "csprng.h"
  
#define ISAAC_MIX \
gr[0]-=gr[4]; gr[5]^=gr[7]>>9;  gr[7]+=gr[0]; \
gr[1]-=gr[5]; gr[6]^=gr[0]<<9;  gr[0]+=gr[1]; \
gr[2]-=gr[6]; gr[7]^=gr[1]>>23; gr[1]+=gr[2]; \
gr[3]-=gr[7]; gr[0]^=gr[2]<<15; gr[2]+=gr[3]; \
gr[4]-=gr[0]; gr[1]^=gr[3]>>14; gr[3]+=gr[4]; \
gr[5]-=gr[1]; gr[2]^=gr[4]<<20; gr[4]+=gr[5]; \
gr[6]-=gr[2]; gr[3]^=gr[5]>>17; gr[5]+=gr[6]; \
gr[7]-=gr[3]; gr[4]^=gr[6]<<14; gr[6]+=gr[7]; \


#define ADAM_MIX(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)\
frt[a]-=frt[m]; frt[f]^=frt[p]>>9;  frt[p]+=frt[a]; \
frt[b]-=frt[n]; frt[g]^=frt[i]<<9;  frt[i]+=frt[b]; \
frt[c]-=frt[o]; frt[h]^=frt[j]>>23; frt[j]+=frt[c]; \
frt[d]-=frt[p]; frt[a]^=frt[k]<<15; frt[k]+=frt[d]; \
frt[e]-=frt[i]; frt[b]^=frt[l]>>14; frt[l]+=frt[e]; \
frt[f]-=frt[j]; frt[c]^=frt[m]<<20; frt[m]+=frt[f]; \
frt[g]-=frt[k]; frt[d]^=frt[n]>>17; frt[n]+=frt[g]; \
frt[h]-=frt[l]; frt[e]^=frt[o]<<14; frt[o]+=frt[h]; \

#define SEED_BLOCK(a,b,c,d,e,f,g,h,i,j,k,l,m,n,ind,x) \
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

// ChaCha rounding functions (64-bit versions adapted from BLAKE2b)
#define ROTR(a,b) ((frt[a] >> b) | (frt[a] << (( (-b) & 63))))
#define QR(a,b,c,d) (	\
	frt[a] += frt[b], frt[d] ^= frt[a], ROTR(d, 32),	\
	frt[c] += frt[d], frt[b] ^= frt[c], ROTR(b, 24),	\
	frt[a] += frt[b], frt[d] ^= frt[a], ROTR(d, 16),	\
	frt[c] += frt[d], frt[b] ^= frt[c], ROTR(b, 63)   \
)

// THE BOOK OF GENESIS (except not really at all)
static constexpr u64 GENESIS[GENESIS_SIZE] = {
0x18f37f54795f9f3a, 0x79ca1fcfd26edc1f, 0x6fb29969f0936178, 0x7c411beef71f8ecd,
0x69024b503123d471, 0xab78a30329836531, 0xf9b21a1557c140e1, 0xd1b7807d3cfcd43b,
0x7c23fcc05a6127ce, 0x9a202bce795c0bab, 0x7a39c38d04a5b07c, 0x2ee787ff7dcdb2c4,
0xda4faa6876e9010f, 0x96c1838250d35b87, 0x3fbb1d32051a824, 0xd1fa182545e789f7,
0xd50dd1fd7c80058a, 0xde2d81da2ab4e423, 0x622d1891bd79e5d1, 0x8450ae6dc89c5e02,
0x9be967d720394e3e, 0x542f1b6d0a8a0091, 0x329846ecb0778541, 0xb815511a56e40a60,
0x7088543621c28df4, 0x1bba5f2defe67b3e, 0x3eea81f71289d216, 0x83256e02bfa703ae,
0xbaf5d1fa31666471, 0x24dca2437a17c859, 0x9872995986b2944c, 0x5328f0163c4af354,
0x70120ee405db470f, 0x2ada3084554bbaca, 0xace564a7a6ebbf99, 0x367ccdc93bc204e1,
0xdee9093485d27118, 0x50b041bb0646bba7, 0xcdc821be4d914ab1, 0xbf66fa1fd1eafbf8,
0x6ccc271c281fe5af, 0xce63d5249def931, 0x4211509dc5543158, 0x6a643e278bbc7751,
0x393a1b0ff0539874, 0xbd66e2da1ade9f59, 0x6902b65ec536796a, 0x32c1ffec06ff2f5d,
0x5ad4f01023a5018f, 0xb51e113b51280a8e, 0x3874a5aad84c8b0b, 0x15812113d8093a10,
0xa774ab8c2ebfa405, 0xb3b08048f7acf0eb, 0x45be8569f8a905d7, 0x7328759b710330e8,
0x23d8168489fd88d7, 0x4bff7aef7d36901c, 0x28a1c2a26cfa172b, 0xccd0415021302570,
0x24a9f5aff9effc96, 0x81dbcbafbf84eaf0, 0x3be3537de4904132, 0x4c4536cc81f9ff1,
0x9f02d4db50d13df7, 0xe76a5fb363511d05, 0x36f75af554420c93, 0x66d7fd4d48634cd8,
0x5fd268662f116c8, 0x990acd99218aeae8, 0xdd6d47e40f009837, 0xd89c9a7bbd425f7,
0x5b138b83501915cf, 0xee2a5d49a9758e13, 0x60ade7884a54e57d, 0xb2baee9a1ad3c4b4,
0x1009f3312736e357, 0x1c2bac59b28ec5af, 0x2f364fd91d4ac9b5, 0x720acbc2783917de,
0x6835511feb0bc6dc, 0xa26ad49467532a3b, 0xbfbe2a0be7a2b4cf, 0x32a8a368ec12e1c8,
0xa2aa409ec7230d2c, 0xd7a235eac194bc3e, 0x5e0e50cd84b51ff0, 0xc3be8e63ca81d95d,
0x7f183b300e87d90d, 0x5b5ae1bd926f95ac, 0x16b2ef281863f8e0, 0x4bfa70c52a08a6c5,
0x970ce69b350a0f43, 0x93bd65d6a342d790, 0xfaa0adcc07074a72, 0x8e1fede1c4634fb8,
0xe0be92dd8e80b1b6, 0x2ca787bf3a608055, 0x4ae9cccc7c005165, 0xc2894804dfac1ffe,
0xbb14a4040a775df8, 0x709d7b4e68db6760, 0x97a7f977e8a4e53, 0xaee9ff360e546100,
0x498de1aab14ed20e, 0x48d308fe97ae16e7, 0x80cbb22ac0eb2982, 0x970083fc184845f2,
0x83ed69f6cd20be57, 0xa781cee1c1f7dbd4, 0x107ee476f9fd402, 0x5e8a1acb68d01b03,
0xd7c4006e5ef1fa0a, 0xf7413211b408cbff, 0xd3ea998e2aaeda23, 0x4b6d26029eea4dea,
0xb17c2dcc03073038, 0xb574e7d699577ac2, 0x88179c9a8ef72f49, 0xb838148f0f74cef8,
0xa34e3e25ace3a537, 0x1806be19a792c8a4, 0xd114bf1b2878fc4b, 0x3c6ac1299892b93a,
0xd64c4d01c967f68a, 0x27b30154a7742eb4, 0x2c7236ffc9d6956c, 0x483f1e8d9b03b8c8,
0x3b27b6bd545e0461, 0x86316ca3d0a9c0b1, 0x2f586acaf884d7f1, 0xb4ccd49f043d089,
0xe2deb22d71091755, 0xa41319d944cb5952, 0xa57110c673545f00, 0x3b1a94461cd95448,
0xc7d03a12722c0f3a, 0x48fc2fe769e77640, 0x151df3e8327f522c, 0x9c385cd543d655fb,
0xdcad22802919098f, 0xf06d090ee633423b, 0x99a4025a6ef86099, 0xa510238aa4a7e4db,
0x63000ec54ef98604, 0x203d116902fdb8e6, 0xa4f4c2137e34155e, 0x672c48840b8ecdfc,
0x2b8158130b87e57d, 0xb97aff150a90c8df, 0x2c1f6f66ac4dee5c, 0xf43e62c8692808dd,
0x207f98aa3640793, 0x2216957b177e48cc, 0x3a94deeae504252e, 0x208ae77ebadd9bdf,
0x6a598dfb47dc910, 0xddd7865f718a997d, 0xcfcdba926daaa771, 0xa8d1ab561b0e31e8,
0xcfc417ecaf94081a, 0x43e731af928356d7, 0x90a0a43258df4dca, 0x959275db91bd8f6d,
0x1b6609bab7f4ed99, 0xb3c1069fafe714f, 0x16247bbcf37df4c, 0xd48a60a21bb892fa,
0x66d6b6cfee9886f0, 0x3231da8c32f17304, 0x7c8639e620ec3b14, 0x5ebe9916e928e376,
0xec39edbb1c73b7e1, 0x31f36a91dd6aa505, 0x4385edee8d21210e, 0xffd32642d6ff590a,
0x7f87d75fbab041f9, 0x3b56e439da632472, 0x715ddb18e162ca44, 0x1679d3866da1d83c,
0x8cf6734e99199ef1, 0xce18e1a77713da5b, 0xf3bccc3a5b947493, 0x6515afead52bb92a,
0x39f291c5eab826ce, 0x911fe8ddd19950fd, 0x22d7d9186ed41c94, 0x12eece644aec0cdc,
0xbcfac322e02f26c, 0x5d958369565fcae5, 0x6b923fa552543990, 0xa3650585354eba25,
0x39650c45f45ff1a6, 0x65dfcc3e52f4cfed, 0x4a7eb1c10db2ec92, 0xf265a76c3bb55ba4,
0xea96548c6f054911, 0x94e55a8b9cb2e9a3, 0x9d9e21ac424e367, 0xcf60b6d3a508f35c,
0xfb8b848ceaa1d7b1, 0x38d64288b433c536, 0x24dd75c259d1ca85, 0x513789e2446a1e14,
0x84d940fa499f1143, 0x9436e026a3bb2fef, 0x42bfd17f12d50b8c, 0xdb387fbe7ee12e56,
0xdcfc868d953ee136, 0x8ac82ebb4ccbb705, 0xc0647951b541639b, 0xd04cfc07e8b54d52,
0x63688192e891dce0, 0xefde5cc3d0bb4a6f, 0x8fc61290f7a72b67, 0x31b59b6183275dc6,
0xb230b7d520ed5937, 0x2e18c197db7937c8, 0xf1ee9d71384fd474, 0x573ef6b89b429f3f,
0xc08937fcc21a36d, 0x6b9778256697a653, 0xf60f92e71e357c4e, 0xdd44480437ebe418,
0x65039073e810a4a1, 0xbf2f3cc2f2ce6d27, 0xc5f131c2f62042ae, 0x5b4e9b292dc05ce6,
0x221f5ae24fc37958, 0xe26694952a3cb12d, 0x414bca1b98a2f13, 0x2555a52d3092aca1,
0x3f10acc6d97ca602, 0xbfa8b19cd8ab9799, 0x35de40668b4e0fbd, 0x8b8a5c18c63ca524,
0xc295d4fb7e06c563, 0x61c0e4ca537a43ab, 0x8b65e879f7cad9cd, 0x399d1c514d9bf4e1,
0x8c2336a33d9d45e5, 0x559232a1e33d978f, 0xe052b1752e8c602c, 0xec8c33d16041e403,
0x1f7ec054239dc94, 0x7861a775455c9d49, 0xc0b39cc9df8c9e41, 0x31c7b2c98cba2377,
0x424016ea0bb80b28, 0xe3c9f69348f38bc5, 0x7aca76a2cbd709e9, 0x93adcfbaa32510df,
0xb80bea9da19c09d2, 0x9bdf5b9f220032cb, 0x7e41d8fe1227e02c, 0x733be44af9f5787d,
0x212fcd3e2ccb47a, 0xbf89c03a9351440, 0xfa333907b18b7220, 0xaf29342c32c2753c,
0x2567e2d6c94e6fb1, 0x979728166e3b5c01, 0xbfb094beafb005c9, 0x8b7cf675bbf656c2,
0x64a7419aade6115c, 0xe897fc40fc370f00, 0x7decb26a735d6b5e, 0x14b031627ee7715a,
0x5600d8859c7a22f7, 0x521cbc524a84329d, 0x6d1c2a5cf183cbd3, 0x1c45847e00454d60,
0x2e7cc711321c7256, 0x19e9f13e702afdf9, 0xdc49fa77aefa7d93, 0x1cb2101113fd9bf8,
0x3a84e91afbdb0da, 0x7752007bf0e9ed37, 0xcb8898aff86a63e1, 0x348e288df06b6ef6,
0xa3dc18e026fd0ea7, 0x81bba071511e8c38, 0x101f461e999cacb, 0xcbee83dbafc6e64c,
0x6c34b561a1a91312, 0xd4d89eb4586dddf9, 0x79a88089e8cafe6d, 0xbde782eeea4f468b,
0x201df705adc535dc, 0x60ec2361c6437b3, 0xb4d1e00ea641c6a8, 0xa62f337f5053a40e,
0x538e4eb9c179e093, 0x4d54d6b0665687fa, 0x3911f85294db6ada, 0xff652781e4b339f6,
0x257d2208299d16cb, 0x8edad1252723dad1, 0x77b8d404897f5f33, 0xbc5d35b60979e7d1,
0x624586a0e6b6aa5a, 0x8949c413fd0d4979, 0x3ca2f6232d7cc4e4, 0xa5b13c8ea29a875a,
0x5595362a530adb69, 0x75eeb5c430a78d07, 0x25bf7878305f0211, 0xa968d7a2a4004e,
0x4658192fb1ff5398, 0x5101843f39a79fc9, 0xa9b7532ab0cdee75, 0x4217f6a6f95ee237,
0x3ff0951024b48736, 0xcae12bd4c7e41e39, 0x3d452f7ee41308f8, 0xfdb944280af69f88,
0x31401aa93dd07816, 0x28f6f5d645d26245, 0xa6148bf82ebcf8dc, 0xf50e56dc0e0749b0,
0x7bf0776db8d93cad, 0x2475546f691e531c, 0xab896b6f7cf298d8, 0xe02f98bb2af5aa86,
0xbed3604535ccc073, 0xcd3740e7c9a5399f, 0x4abe4d6517ff1e99, 0xb552fb0f497b0e74,
0x736d2ea1dd9aabc1, 0x11f80438af414a3b, 0x6a7b497878458dcd, 0x1d5817829232abf1,
0xc374e98833fd7e37, 0xc4da2c1f469eb065, 0xea15fceba720d4e0, 0xe121a175ada5022f,
0x745b16b65755a10e, 0x1474a632f17bfe4c, 0xfca45c2bcbbc23fc, 0xa72c0e20e613e23c,
0x2377de728f03e1bd, 0x46680b176b307b01, 0x258c29893c30cd81, 0xac84a54eb71bddeb,
0xd0aafbf4a6a5fe56, 0x9db583e50006fc71, 0x829e6e5f65325faf, 0xcc01cf8854f1e33b,
0x2fc2536e6e7ff406, 0x24eeeba5aa0c9306, 0x3c219f0e588e5ef6, 0xdeeac17dc475ab20,
0x3a53c1c7685a5aaf, 0xb410dadbb2c3840, 0x5c952f6c8ca1e03e, 0x655f7e8c452abb98,
0xd96dde26b345fd2d, 0x8df4f3d57a0a83f7, 0x7da5b223f178f98d, 0x44fac8aa1ff3309c,
0xa058d12392b9a48f, 0xe4b0560adec61985, 0xeb6f7155d5e49fab, 0x9cbddc1ca60ba39d,
0x158be0504289fa7c, 0x7ca6e0f3298a8de3, 0xb7715124b75a0be, 0xf330693973d70bc,
0x976594228ea5b0b4, 0x1f35323b88531fca, 0xaba91de1ebcf1aa9, 0xe7cdf7c8c30e14f8,
0x67c09ab9a6772355, 0xbc7c5b90fc952aa6, 0x3a7b3283cc68ca71, 0xd6e40253cf45ab9e,
0xd1be96570f49e02b, 0x5e500497aef5fe55, 0x74bba954dfddb84, 0x7ebe52549ac08b01,
0xbfe7880ed952d482, 0xdefdb277186dd68f, 0xf256e0cd1e26a9ba, 0xb892e7f90c76cf1c,
0x926de6bc23377026, 0xd51c38236287538a, 0xa793bbb72236ce87, 0x8956073fa120afc0,
0xd32c21bba19958fb, 0x83909b279a3e7c1c, 0x2fbe0973e76ad5c3, 0xdefe567f914196a,
0x599ddb47f5a80d0f, 0xa343b8bd8878df2, 0x1fe8be4ad408e9e0, 0x317182f2cfd4bad,
0x3c2b855927eaf5a3, 0xf70e2719dc44d338, 0x994101d6e177acd5, 0xaefbc79c7faf40e3,
0xcfcda30cb9695dd5, 0xdf2a0e916c4775f3, 0x44eea335e9d82029, 0x88d8b265b7f0545,
0x144db48eae7d2e4a, 0xfbc3f0781638f9f3, 0xde42d72e684c48b5, 0xaf87df2487e86fe4,
0xbc691abbd351b089, 0xf36ac0cdb5b9b3c9, 0xd4c4306f9048a504, 0xe92d04c68fbea02f,
0xb53f9a8068de284, 0x77f7359518ec90d1, 0xd03313e8507af650, 0xd946f37eb314df9,
0x9be130fe77d406a9, 0x7141c5d9bd5e3dae, 0x94a1e0787a17e3b5, 0x6b5be251ad5f2aef,
0x15424a8727663817, 0xce06a6fe4dd50315, 0x8c511d46186ff8e3, 0xd05b34c437e9f3a7,
0x65697d9fb1ef1b86, 0x150ade94ef16c903, 0x67a061c830655050, 0xbb1d72885d273769,
0xcbb7f6494eff97a5, 0xb6bc79f807438b5c, 0x4105cb3bf3153dc2, 0xfca05b18935b94bf,
0xf573ef0c6ee15f39, 0xcbd6899ce0d13fb9, 0x29b3b06686a86166, 0xe4e48ce451fef43,
0xd579ad0fd5c42d2f, 0xd36c56e8f17f93ab, 0x46cef397e38837a3, 0x9684e642e267ed9f,
0x3ed595b1886f4d0f, 0x9372210170e8a3d4, 0xb9c866464ec6b505, 0x7adff75879756c3d,
0x42afced9785861bc, 0xc1185d57f4acda52, 0x1afc36c9f5cc8243, 0x99d918018dd23a1f,
0x23be94ddda51417e, 0x6e7b6ce4b0f7f94e, 0xbab7ec4a7215ca6e, 0xe244d4e96db6e898,
0x15901f6ddb0ec49, 0x828519deeaa64c7a, 0xaf83bde32bd57075, 0xd6ae97bebb630885,
0x473f5404c81b8a6, 0x1209e085b19d8b71, 0x5570527c62a7aced, 0xc02b0a8c97d8951d,
0x648791f4b1b76852, 0x8514f259e6815aad, 0x15a2c7e44210e4d7, 0x56948346e5235dcd,
0x5067032ca352a0b5, 0x84082b49e904bfd3, 0xb626955c50d97d2a, 0x245643e0084c1214,
0xbcf0e29140bef1fd, 0xb1a9431cfa8b2fc, 0x92ded5350ffd2eea, 0x4cd6997e228e03d9,
0x4751e1acaf830fc, 0xfc4b3b4a4a0debd0, 0xc049dbb0fff6f084, 0xbf8c58624ac1e20a,
0xebdc90af584ea999, 0x732f5d38fda995bc, 0x60cd08d1737844df, 0x7aea47e9b9c69ecf,
0x7e123fe8b7ce938d, 0x5a85f3c3f9ab9b72, 0x3b3531cb11677bea, 0xd02af6c01c32146c,
0x825ee57c3fdc1a3a, 0x38d5630b40fe2755, 0x424985f3651356db, 0xc48cd58c265367ea,
0x55c852bea030a49c, 0xb90b189014a075d8, 0x38a91902ffa6f55c, 0xa5875b9b0af2e0aa,
0xb78c2e31c73d8f95, 0x7a5acf370d62424d, 0xdd28530dc8f1d999, 0xa4383afab9ab5baa,
0x487acbad57055085, 0xe7fa95cf41c5e46a, 0x64f740a542ee49f6, 0xe4a220566ea96b55,
0x7e7bf9ee304cd5c1, 0x9128f85eda5c7209, 0xce7316463ddb5482, 0x67d0f0892fbd82f3,
0xbc09884416429b03, 0x55eca4a15c504165, 0x63421048b6c32c69, 0x5fbed2efdf68f451,
0x74f28796eda1a935, 0xd571b6d6cb6b412, 0x4f903fa4ef54814a, 0x5c41b7a4e441fdee,
0x8784c0b6477670ee, 0x2186cac84dcd1bc, 0x38366d83522bf1c9, 0x700d37806c1d9a2c,
0x9bf6674215542dd3, 0x61e598d47136ba89, 0x93b37c49bf39fc1e, 0x6b1158932c4715f,
0xd3136ea80085bf8a, 0xd62b6dd32f02dbb, 0x1b933144fb01dfcc, 0x8e531c98304a52dd,
0xd0d6990f9bb9176c, 0xa2fa93f4e4f47e71, 0x93fc9cc54ea35150, 0x93a3bd66125a283a,
0x40367eace1a05dc1, 0x83838d6cf343336c, 0x923417d3f3b0e543, 0x5508e256b8a304db,
0x52e555b396188916, 0xbcdf222499b0902a, 0xfd427cdec21131d7, 0x8af223f209b1e87b,
0x9939a3cbb61a8c3, 0xf088e283ce6f8796, 0x6478222bbe479353, 0x658c57de58768495,
0x9584585e5e97797, 0x9a0a17d502cfc5f0, 0x3f5cef1ec13a6d42, 0xea00f9521e4f3e1e,
0x179cdf10776f7b7c, 0xfd40a5d0dc43b4c, 0xf25fac980d060b92, 0x49f697bae913ceee,
0xef2ca88220d6b37e, 0x9f6c7b9a5b9afcfc, 0xf9675f01bd4f0c1b, 0x40427b83da8bb2c3,
0xc8198265b28f8adf, 0x36c79c18b8c09a9, 0x9e74d03b69328d82, 0xb69a8e1b746cd1b3,
0x8c875d1312fd228f, 0x66068738b42864ff, 0x9e1f20cc2338b476, 0xa6ef7556767e38ff,
0x17726f10e950efe, 0x240894f2dc769d57, 0xdc5ad68dcf20c97d, 0x48ae6cd4e7d6bf30,
0xcc45e044afd15175, 0xb69978a4a044ee85, 0xc19498f17e9d7930, 0x3c10463c37c39c11,
0xf6e62f1d1ad37147, 0x7543057adec16403, 0x630a2f237b0dc87d, 0xf795d926cb6469e6,
0x28d07f829a6771fd, 0x1377116520658369, 0xd1a778950cdec234, 0x58eaa9dc88917fa6,
0x6332047fe31565fc, 0x70beb388c60bb7f0, 0xe6618c67dba01d95, 0xa5a80aafee8dd035,
0xd95c150b16e74a4, 0x82a2880c52ac9631, 0x424f6debbce84743, 0xe76c3851f41a0b,
0x6e24e1f386be95f, 0xa8e620379e50842e, 0x581db20c4abf823c, 0xaa857ad23000adee,
0xa532663513756468, 0x597d7536bf3caae0, 0x387ca17fc1c668ce, 0x95f86ef6fc2909f6,
0x6992d0e75e42838a, 0xe553322e8606ca82, 0xed5345f4f115216a, 0x63e5b9f075e423a,
0xe6f4857da87450fd, 0x399bbc0b8a39edd7, 0xc9b5268deea9dcd8, 0x9ba3d9cd184cadce,
0x77536b7671acf1e2, 0xdc4020ca86296e9b, 0xb4f8c9171b3f49e0, 0x351b23157508c797,
0xe32aa848dc7fee06, 0xc5f4b895a38956b4, 0x5802bd3f6ebed25a, 0x6463129f7c85050d,
0x88e2c23c05a0f456, 0xaf23ea78e7505d86, 0x4ef13a75293616e9, 0xb549b9c29306d37,
0xa171e95df15a3346, 0x6546dd3a34140932, 0xd54bb08eaa6f0c8b, 0xb514ffe57261101c,
0x4d285640a84e0a93, 0xfd5995cfd40ecda, 0xe13049a45c68e752, 0xd4d9424d7e0b9a4b,
0x25c7847588a8cdaf, 0xbe24d9305f1da411, 0x291dbecc03a20fe2, 0x4672f4a73de5df75,

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
  // offset nonce for each S-Box
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
  SEED_BLOCK(0,1,2,3,16,17,18,19,32,33,34,35,49,50,gidx,frt[7]);
  // BLOCK 2
  SEED_BLOCK(5,6,20,21,22,23,36,37,38,39,52,53,54,55,gidx,frt[15]);
  // BLOCK 3
  SEED_BLOCK(8,9,10,11,24,25,26,27,40,41,42,43,57,58,gidx,frt[51]);
  // BLOCK 4
  SEED_BLOCK(13,14,28,29,30,31,44,45,46,47,60,61,62,63,gidx,frt[59]);
  // BLOCK 5
  SEED_BLOCK(64,65,66,67,80,81,82,83,96,97,98,99,113,114,gidx,frt[71]);
  // BLOCK 6
  SEED_BLOCK(69,70,84,85,86,87,100,101,102,103,116,117,118,119,gidx,frt[79]);
  // BLOCK 7
  SEED_BLOCK(72,73,74,75,88,89,90,91,104,105,106,107,121,122,gidx,frt[115]);
  // BLOCK 8
  SEED_BLOCK(77,78,92,93,94,95,108,109,110,111,124,125,126,127,gidx,frt[123]);
}

void CSPRNG::diffuse() {
  // initialize GOLDEN RATIO values AND THEN MIX!
  alignas(64) static u64 gr[8] = {
    0x9e3779b97f4a7c13LLU,
    0x9e3779b97f4a7c13LLU,
    0x9e3779b97f4a7c13LLU,
    0x9e3779b97f4a7c13LLU,
    0x9e3779b97f4a7c13LLU,
    0x9e3779b97f4a7c13LLU,
    0x9e3779b97f4a7c13LLU,
    0x9e3779b97f4a7c13LLU
  };

  ISAAC_MIX 
  ISAAC_MIX
  ISAAC_MIX 
  ISAAC_MIX

  // SIMD time baby!
  #ifdef __AVX512F__
    reg grm, m1, m2;
    for (int i{0}; i < FRUIT_SIZE / 2; i+=16) {
      // mix GR
      grm = REG_LOAD64((reg *) &gr[0]);
      // load 1024 bits of integer data
      m1 = REG_LOADBITS((reg*) &frt[i]); 
      m2 = REG_LOADBITS((reg*) &frt[i+8]);
      // apply the mix
      m1 = REG_ADD64(m1, grm);
      m2 = REG_ADD64(m2, grm);
      // store mixed results (note the index flip)
      REG_STOREBITS((reg*) &frt[i+8], m1);
      REG_STOREBITS((reg*) &frt[i], m2);
      ISAAC_MIX
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
      gr1 = REG_LOADBITS((reg *) &gr[0]);
      gr2 = REG_LOADBITS((reg *) &gr[4]);
      // load 1024 bits of integer data
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

      ISAAC_MIX
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
    alignas(128) static const u64 hrcn[16] = {
      0,5,4,1,1,6,2,2,4,4,1,6,2,3,3,7
    }
    const reg p1 = REG_LOAD64((reg *) &hrcn[0]);
    const reg p2 = REG_LOAD64((reg *) &hrcn[8]);
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
    // Need to stop and swap values and apply multiple permutations
    alignas(128) static u32 hrcn[32] = {
      0,1,6,7,4,5,2,3,
      0,1,4,5,6,7,6,7,
      0,1,0,1,4,5,2,3,
      0,1,2,3,2,3,6,7
    };

    const reg s1 = REG_LOADBITS((reg *) &hrcn[0]);
    const reg s2 = REG_LOADBITS((reg *) &hrcn[8]);
    const reg s3 = REG_LOADBITS((reg *) &hrcn[16]);
    const reg s4 = REG_LOADBITS((reg *) &hrcn[24]);

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
    apply ChaCha20 quarter rounding function 2x per table. This also
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
}
