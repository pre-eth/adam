#ifndef RNG_H
#define RNG_H
    #include "util.h"

    #define GENESIS_SIZE    624             // Size of Genesis buffer. Derived from 512 + SEED_SIZE
    #define FRUIT_SIZE      256             // size of the fruit buffer
    #define SEED_SIZE       112             // (Adam's age of death - 1) / 8 - 4.       
                                            // ^ Also the # of u64's extracted from Genesis.       
    #define TAU_BLOCKS      512             // The size of the static TAU array
    #define PARTITION       64              // Size of each table         
    #define TABLE_SIZE      16              // Size of the 8 4x4 64-bit blocks

    #define ROUNDS          7               // Runs mangle for 4*ROUNDS iterations, configurable
   
    alignas(16384) static u64 frt[FRUIT_SIZE];

    class CSPRNG {
        public:
            enum Cycle { REGULAR, DOUBLE, QUADRUPLE };

            CSPRNG() {
                generate();
            };

            u64 get(u8 ind);
            u64 get() { return get(size - 1); };
            u8 rerun() { return generate(); };
            void fill();

        protected:
            u8 generate();

            // size is updated as integers are removed (zeroed out)
            int size{FRUIT_SIZE};
            u8 rounds{ROUNDS};
            u8 cycle{0};
            u8 precision{64};
            bool flip{false};
            bool regen{false};

        private:
            void accumulate();
            void diffuse();
            void augment();
            void mangle();

            // utility functions
            void regenerate();
            void augshift();
            void undulate();
    };
#endif