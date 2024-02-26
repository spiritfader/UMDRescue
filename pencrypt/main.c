#ifdef WIN32
	#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"
#include "endian.h"
#include "kirk_engine.h"
#include "psp_headers.h"

unsigned char pspHeader[336] = {
	0x7E, 0x50, 0x53, 0x50, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x34, 0x83, 0x57, 0x00, 0x90, 0x84, 0x57, 0x00, 
	0x9C, 0x26, 0x05, 0x00, 0x74, 0x71, 0x07, 0x00, 0x74, 0xD4, 0x16, 0x00, 0x40, 0x00, 0x40, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x58, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x3C, 0x58, 0x55, 0x00, 0x7C, 0xD5, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 
	0x70, 0x66, 0x40, 0xF4, 0x1B, 0x05, 0x39, 0x26, 0x8B, 0xFF, 0x03, 0xE2, 0x27, 0x2E, 0x66, 0x8B, 
	0xE5, 0x70, 0x5E, 0x17, 0x53, 0xF9, 0xAA, 0x95, 0x35, 0x8D, 0xD3, 0x43, 0x53, 0xBF, 0x2F, 0xB8, 
	0x86, 0xEE, 0x0A, 0xF1, 0xEF, 0x1F, 0xD1, 0x43, 0x29, 0xA7, 0x53, 0xF2, 0x28, 0x44, 0x29, 0xFE, 
	0x34, 0x83, 0x57, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x0C, 0x7F, 0x85, 0x78, 0x8C, 0xEE, 0x1E, 0x63, 0xF8, 0x85, 0x18, 0x2F, 0x6F, 
	0xF0, 0xA0, 0xD6, 0x35, 0x51, 0x73, 0xAD, 0x9F, 0xA8, 0x6C, 0x2F, 0xCA, 0x7F, 0x44, 0x74, 0x19, 
	0xCF, 0x5A, 0x34, 0x1E, 0x33, 0x5A, 0x17, 0x8B, 0x29, 0x28, 0x44, 0xF1, 0x33, 0xC1, 0xAA, 0x25, 
	0x41, 0x1F, 0x6D, 0x37, 0x2A, 0xB0, 0x73, 0xD3, 0x1C, 0xA3, 0x9E, 0x9C, 0xE7, 0x12, 0xB5, 0xB7, 
	0x5A, 0xA0, 0x33, 0x3B, 0x23, 0xD9, 0xC7, 0x8D, 0x36, 0xA9, 0x77, 0xDC, 0x01, 0x83, 0x7E, 0x5B, 
	0xA8, 0x44, 0x52, 0x01, 0x50, 0x4A, 0x22, 0x52, 0xD2, 0xD1, 0x63, 0x69, 0x3D, 0xC6, 0xE9, 0x56, 
	0xDE, 0x4B, 0x6A, 0xE2, 0xB8, 0x1A, 0xA3, 0x54, 0xEC, 0x2B, 0x43, 0x4A, 0x99, 0x12, 0x35, 0x72, 
	0xEF, 0x09, 0x29, 0x52, 0x71, 0xE6, 0x18, 0xDF, 0x59, 0x46, 0x19, 0xDF, 0x15, 0x33, 0xE0, 0x7D, 
};

unsigned char kirkHeader[272] = {
	0xED, 0x45, 0x47, 0x4F, 0xA0, 0xAE, 0xE3, 0x17, 0x93, 0xBF, 0x68, 0xD2, 0x5D, 0x1C, 0xFA, 0xD1, 
	0xDA, 0xD7, 0x69, 0x4C, 0xAF, 0x28, 0x81, 0xC3, 0xF0, 0x3F, 0x9C, 0x02, 0x0A, 0x8E, 0x6F, 0xCE, 
	0x18, 0x0E, 0x0E, 0xF9, 0x09, 0x63, 0x50, 0x46, 0x26, 0xF2, 0xF1, 0x77, 0x80, 0xE1, 0x44, 0x80, 
	0xA6, 0xD5, 0x4E, 0xBE, 0xF4, 0xB8, 0xA6, 0x47, 0x04, 0x9A, 0xF5, 0xEA, 0xCE, 0xA0, 0x39, 0x5C, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x34, 0x83, 0x57, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x7E, 0x50, 0x53, 0x50, 0x00, 0x02, 0x00, 0x00, 0x01, 0x01, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x34, 0x83, 0x57, 0x00, 0x90, 0x84, 0x57, 0x00, 
	0x9C, 0x26, 0x05, 0x00, 0x74, 0x71, 0x07, 0x00, 0x74, 0xD4, 0x16, 0x00, 0x40, 0x00, 0x40, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x58, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x3C, 0x58, 0x55, 0x00, 0x7C, 0xD5, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 
};

u8 in_buffer[1024*1024*10] __attribute__((aligned(0x40)));
u8 out_buffer[1024*1024*10] __attribute__((aligned(0x40)));

u8 kirk_raw[1024*1024*10] __attribute__((aligned(0x40)));
u8 kirk_enc[1024*1024*10] __attribute__((aligned(0x40)));
u8 elf[1024*1024*10] __attribute__((aligned(0x40)));

typedef struct header_keys {
	u8 AES[16];
	u8 CMAC[16];
}header_keys;

int load_elf(char *elff) {
	FILE *fp = fopen(elff, "rb");
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fread(elf, 1, size, fp);
	fclose(fp);
	return size;
}

void dumpFile(char *name, void *in, int size) {
	FILE *fp = fopen(name, "wb");
	fwrite(in, 1, size, fp);
	fclose(fp);
}

int main(int argc, char **argv) {
	header_keys keys;
	u8 rawkheaderBk[0x90];

	if (argc < 2) {
		printf("USAGE: [exe] [prx]\n");
		return 0;
	}

	memset(in_buffer, 0, 1024*1024*10);
	memset(out_buffer, 0, 1024*1024*10);
	memset(kirk_raw, 0, 1024*1024*10);
	memset(kirk_enc, 0, 1024*1024*10);
	memset(elf, 0, 1024*1024*10);

	kirk_init();
	int decSize = pspHeader[0x28] | ((int)pspHeader[0x29] << 8) | ((int)pspHeader[0x2A] << 16) | ((int)pspHeader[0x2B] << 24);
	int encSize = pspHeader[0x2C] | ((int)pspHeader[0x2D] << 8) | ((int)pspHeader[0x2E] << 16) | ((int)pspHeader[0x2F] << 24);
	int krawSize = encSize - 0x40;
	int elfSize = load_elf(argv[1]);

	if (elfSize > krawSize - 0x110) {
		printf("PRX SIGNER: Elf is to big\n");
		return 0;
	}

	memcpy(kirk_raw, kirkHeader, 0x110);
	memcpy(rawkheaderBk, kirk_raw, 0x90);

	kirk_decrypt_keys((u8*)&keys, kirk_raw);
	memcpy(kirk_raw, &keys, sizeof(header_keys));
	memcpy(kirk_raw+0x110, elf, elfSize);

	if (kirk_CMD0(kirk_enc, kirk_raw, 1024*1024*10, 0) != 0) {
		printf("PRX SIGNER: Could not encrypt elf\n");
		return 0;
	}

	memcpy(kirk_enc, rawkheaderBk, 0x90);

	if (kirk_forge(kirk_enc, 1024*1024*10) != 0) {
		printf("PRX SIGNER: Could not forge cmac block\n");
		return 0;
	}

	memcpy(out_buffer, pspHeader, 0x150);
	memcpy(out_buffer+0x150, kirk_enc+0x110, krawSize-0x110);

	dumpFile("./data.psp", out_buffer, (krawSize-0x110)+0x150);

	return 0;
}