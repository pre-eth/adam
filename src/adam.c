#include <threads.h>

#include "adam.h"

typedef struct thread_data {
  u64 *start;
  u64 *end;
  double seed;
} thdata;

static double mod_table[BUF_SIZE] ALIGN(SIMD_LEN) = {
  72340172838076672.0,   72624976668147840.0,   72912031911895456.0,    73201365371863296.0, 
  73493004277727296.0,   73786976294838208.0,   74083309532970080.0,    74382032555280448.0, 
  74683174387488064.0,   74986764527274608.0,   75292832953916544.0,    75601410138153904.0, 
  75912527052302688.0,   76226215180617984.0,   76542506529915152.0,    76861433640456464.0, 
  77183029597111088.0,   77507328040796432.0,   77834363180209072.0,    78164169803854032.0, 
  78496783292381072.0,   78832239631237408.0,   79170575423646144.0,    79511827903920480.0, 
  79856034951123600.0,   80203235103085008.0,   80553467570784064.0,    80906772253112064.0, 
  81263189752024464.0,   81622761388095360.0,   81985529216486896.0,    82351536043346208.0, 
  82720825442643728.0,   83093441773466448.0,   83469430197780784.0,    83848836698679776.0, 
  84231708099130368.0,   84618092081236480.0,   85008037206034800.0,    85401592933840512.0, 
  85798809645160704.0,   86199738662194160.0,   86604432270936864.0,    87012943743912976.0, 
  87425327363552384.0,   87841638446235968.0,   88261933367031344.0,    88686269585142080.0, 
  89114705670094448.0,   89547301328687136.0,   89984117432729520.0,    90425216047595840.0, 
  90870660461623408.0,   91320515216383920.0,   91774846137858464.0,    92233720368547760.0, 
  92697206400550512.0,   93165374109644208.0,   93638294790403808.0,    94116041192395664.0, 
  94598687557484880.0,   95086309658296656.0,   95578984837873328.0,    96076792050570576.0, 
  96579811904238496.0,   97088126703734480.0,   97601820495817728.0,    98120979115476336.0, 
  98645690233740912.0,   99176043407040608.0,   99712130128159744.0,    100254043878856256.0, 
  100801880184205200.0,  101355736668733808.0,  101915713114417408.0,   102481911520608624.0, 
  103054436165975152.0,  103633393672525568.0,  104218893071805376.0,   104811045873349728.0, 
  105409966135483152.0,  106015770538560640.0,  106628578460748848.0,   107248512056450880.0, 
  107875696337482752.0,  108510259257115008.0,  109152331797097936.0,   109802048057794944.0, 
  110459545351554208.0,  111124964299455136.0,  111798448931573040.0,   112480146790911904.0, 
  113170209041162896.0,  113868790578454016.0,  114576050147264288.0,   115292150460684704.0, 
  116017258325217312.0,  116751544770313616.0,  117495185182863392.0,   118248359446856096.0, 
  119011252088448720.0,  119784052426685408.0,  120566954730127792.0,   121360158379668096.0, 
  122163868037811600.0,  122978293824730352.0,  123803651501406384.0,   124640162660199680.0, 
  125488054923194224.0,  126347562148695552.0,  127218924646272768.0,   128102389400760768.0, 
  128998210305661200.0,  129906648406405296.0,  130827972153968448.0,   131762457669353936.0, 
  132710389019493184.0,  133672058505141680.0,  134647766961383584.0,   135637824071393760.0, 
  136642548694144832.0,  137662269206787696.0,  138697323862477824.0,   139748061164466304.0, 
  140814840257324816.0,  141898031336227328.0,  142998016075267840.0,   144115188075855872.0, 
  145249953336295680.0,  146402730743726592.0,  147573952589676416.0,   148764065110560896.0, 
  149973529054549216.0,  151202820276307808.0,  152452430361235968.0,   153722867280912928.0, 
  155014656081592864.0,  156328339607708064.0,  157664479262474816.0,   159023655807840960.0, 
  160406470206170016.0,  161813544506224128.0,  163245522776190720.0,   164703072086692416.0, 
  166186883546932896.0,  167697673397359552.0,  169236184162472960.0,   170803185867681024.0, 
  172399477324388320.0,  174025887487825952.0,  175683276892471936.0,   177372539170284160.0, 
  179094602657374272.0,  180850432095191680.0,  182641030432767840.0,   184467440737095520.0, 
  186330748219288416.0,  188232082384791328.0,  190172619316593312.0,   192153584101141152.0, 
  194176253407468960.0,  196241958230952672.0,  198352086814081216.0,   200508087757712512.0,  
  202711473337467616.0,  204963823041217248.0,  207266787345051136.0,   209622091746699456.0,  
  212031541077121280.0,  214497024112901760.0,  217020518514230016.0,   219604096115589888.0,  
  222249928598910272.0,  224960293581823808.0,  227737581156908032.0,   230584300921369408.0,  
  233503089540627232.0,  236496718893712192.0,  239568104853370816.0,   242720316759336192.0,  
  245956587649460704.0,  249280325320399360.0,  252695124297391104.0,   256204778801521536.0,  
  259813296812810592.0,  263524915338707872.0,  267344117010283360.0,   271275648142787520.0,  
  275324538413575392.0,  279496122328932608.0,  283796062672454656.0,   288230376151711744.0,  
  292805461487453184.0,  297528130221121792.0,  302405640552615616.0,   307445734561825856.0,  
  312656679215416128.0,  318047311615681920.0,  323627089012448256.0,   329406144173384832.0,  
  335395346794719104.0,  341606371735362048.0,  348051774975651904.0,   354745078340568320.0,  
  361700864190383360.0,  368934881474191040.0,  376464164769582656.0,   384307168202282304.0,  
  392483916461905344.0,  401016175515425024.0,  409927646082434496.0,   419244183493398912.0,  
  428994048225803520.0,  439208192231179776.0,  449920587163647616.0,   461168601842738816.0,  
  472993437787424384.0,  485440633518672384.0,  498560650640798720.0,   512409557603043072.0,  
  527049830677415744.0,  542551296285575040.0,  558992244657865216.0,   576460752303423488.0,  
  595056260442243584.0,  614891469123651712.0,  636094623231363840.0,   658812288346769664.0,  
  683212743470724096.0,  709490156681136640.0,  737869762948382080.0,   768614336404564608.0,  
  802032351030850048.0,  838488366986797824.0,  878416384462359552.0,   922337203685477632.0,  
  970881267037344768.0,  1024819115206086144.0, 1085102592571150080.0,  1152921504606846976.0, 
  1229782938247303424.0, 1317624576693539328.0, 1418980313362273280.0,  1537228672809129216.0, 
  1676976733973595648.0, 1844674407370955264.0, 2049638230412172288.0,  2305843009213693952.0, 
  2635249153387078656.0, 3074457345618258432.0, 3689348814741910528.0,  4611686018427387904.0, 
  6148914691236516864.0, 9218868437227405312.0, 9223372036854775808.0,  18446744073709551616.0,  
};

FORCE_INLINE static void accumulate(u64 *restrict _ptr, u64 seed, const u64 nonce, double *chseeds) {
  /*
    8 64-bit IV's that correspond to the verse:
    "Be fruitful and multiply, and replenish the earth (Genesis 1:28)"
  */
  u64 IV[8] ALIGN(SIMD_LEN) = {
    0x4265206672756974UL ^  nonce, 
    0x66756C20616E6420UL ^ ~nonce, 
    0x6D756C7469706C79UL ^  nonce,
    0x2C20616E64207265UL ^ ~nonce, 
    0x706C656E69736820UL ^  nonce,
    0x7468652065617274UL ^ ~nonce, 
    0x68202847656E6573UL ^  nonce,
    0x697320313A323829UL ^ ~nonce
  };

  ISAAC_MIX(IV[0], IV[1], IV[2], IV[3], IV[4], IV[5], IV[6], IV[7]);
  ISAAC_MIX(IV[0], IV[1], IV[2], IV[3], IV[4], IV[5], IV[6], IV[7]);
  ISAAC_MIX(IV[0], IV[1], IV[2], IV[3], IV[4], IV[5], IV[6], IV[7]);
  ISAAC_MIX(IV[0], IV[1], IV[2], IV[3], IV[4], IV[5], IV[6], IV[7]);

  reg r1, r2;
  register u8 maps_filled = 0;
  register u8 i;
  fill_the_earth:
    i = 0;
    r1 = SIMD_LOADBITS((reg*) IV);
    r2 = SIMD_LOADBITS((reg*) &IV[(!!(SIMD_LEN & 63) << 2)]);    
    do {

      #ifdef __AVX512F__
        SIMD_STOREBITS((reg*) &_ptr[i],      r1);
        SIMD_STOREBITS((reg*) &_ptr[i + 8],  r2);
        SIMD_STOREBITS((reg*) &_ptr[i + 16], r1); 
        SIMD_STOREBITS((reg*) &_ptr[i + 24], r2);    
      #else
        SIMD_STOREBITS((reg*) &_ptr[i],      r1);
        SIMD_STOREBITS((reg*) &_ptr[i + 4],  r2);
        SIMD_STOREBITS((reg*) &_ptr[i + 8],  r1); 
        SIMD_STOREBITS((reg*) &_ptr[i + 12], r2);    
      #endif
      i += (SIMD_LEN >> 1) - (i == BUF_SIZE - (SIMD_LEN >> 1));
    } while (i < BUF_SIZE - 1);

    if (++maps_filled < 3) {
      ISAAC_MIX(IV[0], IV[1], IV[2], IV[3], IV[4], IV[5], IV[6], IV[7]);
      ISAAC_MIX(IV[0], IV[1], IV[2], IV[3], IV[4], IV[5], IV[6], IV[7]);
      _ptr += BUF_SIZE;
      goto fill_the_earth;
    }

  const regd div = SIMD_SETPD((double)__UINT64_MAX__),
             limit = SIMD_SETPD(0.5);

  regd seeds;

  // We process 8 seeds at a time with AVX-512, so this adjusts
  // the rounds upper bound for the loop appropriately
  const u8 rounds = ROUNDS >> !(SIMD_LEN & 63);
  
  i = 0;
  do {
    IV[0] += seed, IV[1] += seed, IV[2] += seed, IV[3] += seed,
    IV[4] += seed, IV[5] += seed, IV[6] += seed, IV[7] += seed;

    seeds = SIMD_SETRPD((double)(IV[0] ^ IV[7]), 
                        (double)(IV[1] ^ IV[6]),
                        (double)(IV[2] ^ IV[5]),
                        (double)(IV[3] ^ IV[4]));
                        
    seeds = SIMD_DIVPD(seeds, div);
    seeds = SIMD_MULPD(seeds, limit);

    #ifdef __AVX512F__
      SIMD_STOREPD(&chseeds[i << 3], seeds);
    #else
      SIMD_STOREPD(&chseeds[i << 2], seeds);
    #endif
               
    ISAAC_MIX(IV[0], IV[1], IV[2], IV[3], IV[4], IV[5], IV[6], IV[7]);
  } while (++i < rounds);
}

FORCE_INLINE static void diffuse(u64 *restrict _ptr, const u64 nonce) {
  // Following code is derived from Bob Jenkins, author of ISAAC64

  register u64 a, b, c, d, e, f, g, h;
  a = b = c = d = e = f = g = h = (_ptr[nonce & 0xFF] ^ (GOLDEN_RATIO ^ _ptr[(nonce >> (nonce & 31)) & 0xFF]));

  // Scramble it
  ISAAC_MIX(a, b, c, d, e, f, g, h);
  ISAAC_MIX(a, b, c, d, e, f, g, h);
  ISAAC_MIX(a, b, c, d, e, f, g, h);
  ISAAC_MIX(a, b, c, d, e, f, g, h);

  register u8 i = 0;
  do {
    a += _ptr[i];     b += _ptr[i + 1]; c += _ptr[i + 2]; d += _ptr[i + 3];
    e += _ptr[i + 4]; f += _ptr[i + 5]; g += _ptr[i + 6]; h += _ptr[i + 7];

    ISAAC_MIX(a, b, c, d, e, f, g, h);

    _ptr[i]     = a; _ptr[i + 1] = b; _ptr[i + 2] = c; _ptr[i + 3] = d;
    _ptr[i + 4] = e; _ptr[i + 5] = f; _ptr[i + 6] = g; _ptr[i + 7] = h;

  } while ((i += 8 - (i == 248)) < BUF_SIZE - 1);
}

static double multi_thread(thdata *data) {
  register double x = data->seed;
  register u8 i = 0;
  do {
    // x = chaotic_iter(data->end, data->start, x);

  } while (++i < ITER);
  
  return x;
}

/* FORCE_INLINE static void apply(u64 *restrict _ptr, double *restrict chseed) {
  // Number of threads we will use
  #define THREAD_COUNT  (1 << THREAD_EXP)

  // Divide sequence bits by thread count, then divide that by 64 
  // to get the increment value for _ptr and ctz to get shift count
  #define THREAD_INC    CTZ((SEQ_SIZE / THREAD_COUNT) >> 6)

  register u8 i = 0;
  register u16 start = 0, end = BUF_SIZE; 

  thrd_t threads[THREAD_COUNT];
  thdata chdata[THREAD_COUNT];

  THSEED(0, chseed),
  THSEED(1, chseed),
  THSEED(2, chseed),
  THSEED(3, chseed),
  THSEED(4, chseed),
  THSEED(5, chseed),
  THSEED(6, chseed),
  THSEED(7, chseed); 

  eval_fn:
    for(; i < THREAD_COUNT; ++i) {
      chdata[i].start = _ptr + start  + (i << THREAD_INC);
      chdata[i].end   = _ptr + end + (i << THREAD_INC);
      thrd_create(&threads[i], multi_thread, chdata);
    }

    i = 0;
    for(; i < THREAD_COUNT; ++i)
      thrd_join(threads[i], &chdata[i].seed);
    
    i = 0;
    end += start += 256;
    end *= (end <= 512);
    if (start <= 512) goto eval_fn;
} */

FORCE_INLINE static void apply(u64 *restrict _ptr, double *seeds) {
  /* 
    BETA is derived from the length of the significant digits
    ADAM uses the max double accuracy length of 15
  */
  #define BETA                  10E15 

  static u64 arr[SIMD_LEN >> 3] ALIGN(SIMD_LEN);
  
  const regd beta = SIMD_SETPD(BETA),
             coefficient = SIMD_SETPD(3.9999),
             one = SIMD_SETPD(1.0);

  const reg mask = SIMD_SET64(0xFFUL),
            inc  = SIMD_SET64(4UL);

  u64 *map_a = _ptr, *map_b = _ptr + BUF_SIZE;

  register u8 rounds = 0, i = 0,  j;

  regd d1, d2, d3;

  reg r1, scale;

  chaotic_iter:
    d1 = SIMD_LOADPD(&seeds[rounds]);
    j = 0;
    scale = SIMD_SETR64(1UL, 2UL, 3UL, 4UL
                     #ifdef __AVX512__  
                      , 5UL, 6UL, 7UL, 8UL
                     #endif 
                       );

    do {
      // 3.9999 * X * (1 - X) for all X in the register
      d2 = SIMD_SUBPD(one, d1);
      d2 = SIMD_MULPD(d2, coefficient);
      d1 = SIMD_MULPD(d1, d2);

      d2 = SIMD_MULPD(d1, beta);
      d3 = SIMD_LOADPD(&mod_table[j]);
      d2 = SIMD_MULPD(d2, d3);

      // Cast to u64, add the scaling factor
      // Mask so idx stays in range of buffer
      r1 = SIMD_CASTPD(d2);
      r1 = SIMD_ADD64(r1, scale);
      r1 = SIMD_ANDBITS(r1, mask);
      SIMD_STOREBITS((reg*) arr, r1);   

      map_b[j]     ^= map_a[arr[0]];
      map_b[j + 1] ^= map_a[arr[1]];
      map_b[j + 2] ^= map_a[arr[2]];
      map_b[j + 3] ^= map_a[arr[3]];

    #ifdef __AVX512__
      map_b[j + 4] ^= map_a[arr[4]];
      map_b[j + 5] ^= map_a[arr[5]];
      map_b[j + 6] ^= map_a[arr[6]];
      map_b[j + 7] ^= map_a[arr[7]];
    #endif

      scale = SIMD_ADD64(scale, inc);
      j += (SIMD_LEN >> 3) - (j == BUF_SIZE - (SIMD_LEN >> 3));
    } while (j < BUF_SIZE - 1); 

    // Use the next 4 (or 8 for AVX-512) seeds to start next iteration
    rounds += (SIMD_LEN >> 3);

    if (++i < ITER) 
      goto chaotic_iter;
             
    if (rounds < (ROUNDS << 2)) {
      i = (rounds == (ITER << 3));
      map_a += BUF_SIZE;
      map_b += ((u16)!i << 8) - ((u16)i << 9); 
      i = 0;    
      goto chaotic_iter;
    } 
}

FORCE_INLINE static void mix(u64 *restrict _ptr) {
  register u8 i = 0;

  reg r1, r2;
  do {
    r1 = SIMD_SETR64(
                      XOR_MAPS(i)
                  #ifdef __AVX512F__
                    , XOR_MAPS(i + 4)
                  #endif
                    );

    r2 = SIMD_SETR64(
                      XOR_MAPS(i + (SIMD_LEN >> 3))
                  #ifdef __AVX512F__
                    , XOR_MAPS(i + (SIMD_LEN >> 3) + 4)
                  #endif
                    );

    SIMD_STOREBITS((reg*) &_ptr[i], r1);   
    SIMD_STOREBITS((reg*) &_ptr[i + (SIMD_LEN >> 3)], r2);   

    i += (SIMD_LEN >> 2) - (i == BUF_SIZE - (SIMD_LEN >> 2));
  } while (i < BUF_SIZE - 1);
}

double adam(u64 *restrict _ptr, const u64 seed, const u64 nonce) {
  double seeds[ROUNDS << 2] ALIGN(SIMD_LEN);

  clock_t start = clock();

  accumulate(_ptr, seed, nonce, seeds);
  diffuse(_ptr, nonce);
  apply(_ptr, seeds);
  mix(_ptr); 

  return (double) (clock() - start) / CLOCKS_PER_SEC;
}