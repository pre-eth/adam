#ifndef RNG_H
#define RNG_H
    #include "util.h"

    #define FRUIT_SIZE      256             // size of the fruit buffer
  
    alignas(2048) static u64 frt[FRUIT_SIZE];

    class CSPRNG {
        public:
            CSPRNG() {
                generate();
            };
            CSPRNG(u64 s) : seed(s) {
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