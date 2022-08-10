#pragma once
#ifndef SHA1
#define SHA1

#include <string.h>

struct sha1_context {
	unsigned int state[5];
	unsigned int count[2];
	unsigned char buffer[64];
};

void sha1_init(sha1_context* context);
void sha1_update(sha1_context* context, unsigned char* buffer, unsigned int length);
void sha1_final(sha1_context* context, unsigned char* result);

#endif