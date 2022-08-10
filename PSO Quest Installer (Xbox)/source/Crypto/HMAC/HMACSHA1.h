#pragma once
#ifndef HMACSHA1
#define HMACSHA1

#include "..\SHA1\SHA1.h"

void hmac_sha1(unsigned char* buffer, unsigned int buffer_length, unsigned char* buffer2, unsigned int buffer_length2, unsigned char* key, unsigned int key_length, unsigned char* result);

void hmacsha1_init(sha1_context* context, unsigned char* key, unsigned int key_length);
void hmacsha1_update(sha1_context* context, unsigned char* buffer, unsigned int length);
void hmacsha1_final(sha1_context* context, unsigned char* key, unsigned int key_length, unsigned char* result);

#endif 
