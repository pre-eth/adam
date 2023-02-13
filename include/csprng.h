#ifndef RNG_H
#define RNG_H
    #include "util.h"

    #define FRUIT_SIZE      256             // size of the fruit buffer
    #define UNDULATION      3               // default srlv parameter
    
    #ifdef __AVX512F__
        #define FRUIT_LOOP      32
        #define FRUIT_FACTOR    0
    #else
        #define FRUIT_LOOP      16
        #define FRUIT_FACTOR    1
    #endif

    alignas(2048) static u64 frt[FRUIT_SIZE] = {0};

    class CSPRNG {
        public:
            CSPRNG() {
                generate();
            };
            CSPRNG(u64 s) : seed(s) {
                generate();
            }; 

            u64 get(u8 ind);
            u64 get() { return size ? get(size - 1) : 0; };
            u8 rerun() { return generate(); };
            void fill();

        protected:
            u8 generate();

            // size is updated as integers are removed (zeroed out)
            int size{FRUIT_SIZE};
            u8 rounds{7};
            u8 precision{64};
            bool regen{false};
            u64 seed{trng64()};
            u8  undulation{UNDULATION};            

        private:
            void accumulate();
            void diffuse();
            void assimilate();
            void mangle();

            // utility functions
            void regenerate();

    };
#endif