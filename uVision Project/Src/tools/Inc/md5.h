#ifndef __MD5_H__
#define __MD5_H__

//**************************************************************************************************
//***** Includes ***********************************************************************************

#include "flash_if.h"
#include <stdint.h>
#include <stdbool.h>

//**************************************************************************************************
//***** Global Typedefs and Class Declarations *****************************************************

typedef struct
{
	unsigned int count[2];
	unsigned int state[4];
	unsigned char buffer[64];
} MD5_CTX;

//**************************************************************************************************
//***** Global (Public) Defines and Consts *********************************************************

#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))

#define FF(a,b,c,d,x,s,ac) \
{ \
	a += F(b,c,d) + x + ac; \
	a = ROTATE_LEFT(a,s); \
	a += b; \
}
#define GG(a,b,c,d,x,s,ac) \
{ \
	a += G(b,c,d) + x + ac; \
	a = ROTATE_LEFT(a,s); \
	a += b; \
}
#define HH(a,b,c,d,x,s,ac) \
{ \
	a += H(b,c,d) + x + ac; \
	a = ROTATE_LEFT(a,s); \
	a += b; \
}
#define II(a,b,c,d,x,s,ac) \
{ \
	a += I(b,c,d) + x + ac; \
	a = ROTATE_LEFT(a,s); \
	a += b; \
}

//**************************************************************************************************
//***** Prototypes of Global Functions *************************************************************

void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int inputlen);
void MD5Final(MD5_CTX *context, unsigned char digest[16]);
void MD5Transform(unsigned int state[4], unsigned char block[64]);
void MD5Encode(unsigned char *output, unsigned int *input, unsigned int len);
void MD5Decode(unsigned int *output, unsigned char *input, unsigned int len);

int MD5_File_Compute(Flash_FILE *file, uint8_t *md5_value);
bool MD5_File(uint8_t fileBank, uint32_t filePage, uint32_t fileSize, uint8_t *md5_check);

#endif  /* __MD5_H__ */
