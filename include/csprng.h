#ifndef RNG_H
#define RNG_H
    #include "util.h"

    #define GENESIS_SIZE    624             // Size of Genesis buffer. Derived from 512 + SEED_SIZE
    #define FRUIT_SIZE      256             // size of the fruit buffer
    #define SEED_SIZE       112             // (Adam's age of death - 1) / 8 - 4.       
                                            // ^ Also the # of u64's extracted from Genesis.       
    #define TAU_BLOCKS      512             // The size of the static TAU array
  
    alignas(1024) static u64 frt[FRUIT_SIZE];

    class CSPRNG {
        public:
            CSPRNG() {};
            CSPRNG(u64 s) : seed(s) {}; 

            u64 get(u8 ind);
            u64 get() { return get(size - 1); };
            u8 rerun() { return generate(); };
            void fill();

        protected:
            u8 generate();

            // size is updated as integers are removed (zeroed out)
            int size{FRUIT_SIZE};
            u8 rounds{7};
            u8 precision{64};
            bool flip{false};
            bool regen{false};
            u64 seed{trng64()};            

        private:
            void accumulate();
            void diffuse();
            void augment();
            void mangle();

            // utility functions
            void undulate();
            void regenerate();

    };
#endif