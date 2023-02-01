#include "util.h"
#include <cpuid.h>

#define GENESIS_SIZE 624

bool rdrand_support();

// courtesy of https://codereview.stackexchange.com/a/150230
bool rdrand_support() {
	const unsigned int flag_RDRAND = (1 << 30);

    unsigned int eax, ebx, ecx, edx;
    __cpuid(1, eax, ebx, ecx, edx);

    return ((ecx & flag_RDRAND) == flag_RDRAND);
}

int main(int argc, char** argv) {
	u64 GENESIS[GENESIS_SIZE];
	FILE* fp;

	if (!rdrand_support()) {
		printf("%s\n%s\n", "Your system does not support the RDRAND CPU instruction.",
		"Exiting...");
		return 1;
	}

	if (!(fp = fopen("genesis", "rb"))) {
		puts("COULDN'T READ GENESIS");
		return 1;
	}

	fseek( fp , 0L , SEEK_END);
	const int lSize = ftell(fp) - GENESIS_SIZE;
	const int start = trng32() % lSize;
	fseek(fp, start, SEEK_SET);
	fread(&GENESIS, 8, GENESIS_SIZE, fp);
	for (int i = 0; i < GENESIS_SIZE; i+=4)
		printf("0x%llx, 0x%llx, 0x%llx, 0x%llx,\n", GENESIS[i], GENESIS[i + 1], GENESIS[i + 2], GENESIS[i + 3]);
	
	return 0;
}


