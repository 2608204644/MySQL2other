//
// Created by cha on 2022/3/10.
//

#ifndef MYSQL2REDIS__UTIL_H_
#define MYSQL2REDIS__UTIL_H_

#include <string.h>
#include <stdint.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "cassert"

/*
 * convert big endian and little endian
 * */
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef unsigned char uchar;

#define sint2korr(A)    (*((int16 *) (A)))

#define sint3korr(A)    ((int32) ((((uint8) (A)[2]) & 128) ? \
                  (((uint32) 255L << 24) | \
                   (((uint32) (uint8) (A)[2]) << 16) |\
                   (((uint32) (uint8) (A)[1]) << 8) | \
                   ((uint32) (uint8) (A)[0])) : \
                  (((uint32) (uint8) (A)[2]) << 16) |\
                  (((uint32) (uint8) (A)[1]) << 8) | \
                  ((uint32) (uint8) (A)[0])))
#define sint4korr(A)    (*((int32 *) (A)))

#define uint8korr(A)    ((uint64)(((uint32) ((uint8) (A)[0])) +\
                    (((uint32) ((uint8) (A)[1])) << 8) +\
                    (((uint32) ((uint8) (A)[2])) << 16) +\
                    (((uint32) ((uint8) (A)[3])) << 24)) +\
            (((uint64) (((uint32) ((uint8) (A)[4])) +\
                    (((uint32) ((uint8) (A)[5])) << 8) +\
                    (((uint32) ((uint8) (A)[6])) << 16) +\
                    (((uint32) ((uint8) (A)[7])) << 24))) <<\
                    32))

#define sint8korr(A)    (int64) uint8korr(A)

#define uint2korr(A)    (*((uint16*) (A)))

#define uint3korr(A)    (uint32) (((uint32) ((uint8) (A)[0])) +\
                  (((uint32) ((uint8) (A)[1])) << 8) +\
                  (((uint32) ((uint8) (A)[2])) << 16))

#define uint4korr(A)    (*((uint32 *) (A)))

#define uint5korr(A) ((uint64_t)(((uint32_t) (((uint8_t *) (A))[4])) +\
            (((uint32_t) (((uint8_t *) (A))[3])) << 8) +\
            (((uint32_t) (((uint8_t *) (A))[2])) << 16) +\
            (((uint32_t) (((uint8_t *) (A))[1])) << 24)) +\
            (((uint64_t) (((uint8_t *) (A))[0])) << 32))

#define uint6korr(A)    ((int64)(((uint32)    ((uint8) (A)[0]))          + \
                                     (((uint32)    ((uint8) (A)[1])) << 8)   + \
                                     (((uint32)    ((uint8) (A)[2])) << 16)  + \
                                     (((uint32)    ((uint8) (A)[3])) << 24)) + \
                         (((int64) ((uint8) (A)[4])) << 32) +       \
                         (((int64) ((uint8) (A)[5])) << 40))

#define mi_sint1korr(A) ((int8)(*A))
#define mi_uint1korr(A) ((uint8)(*A))

#define mi_sint2korr(A) ((int16) (((int16) (((uint8_t*) (A))[1])) +\
                                  ((int16) ((int16) ((char*) (A))[0]) << 8)))
#define mi_sint3korr(A) ((int32) (((((uint8_t*) (A))[0]) & 128) ? \
                                  (((uint32) 255L << 24) | \
                                   (((uint32) ((uint8_t*) (A))[0]) << 16) |\
                                   (((uint32) ((uint8_t*) (A))[1]) << 8) | \
                                   ((uint32) ((uint8_t*) (A))[2])) : \
                                  (((uint32) ((uint8_t*) (A))[0]) << 16) |\
                                  (((uint32) ((uint8_t*) (A))[1]) << 8) | \
                                  ((uint32) ((uint8_t*) (A))[2])))
#define mi_sint4korr(A) ((int32) (((int32) (((uint8_t*) (A))[3])) +\
                                  ((int32) (((uint8_t*) (A))[2]) << 8) +\
                                  ((int32) (((uint8_t*) (A))[1]) << 16) +\
                                  ((int32) ((int16) ((char*) (A))[0]) << 24)))
#define mi_sint8korr(A) ((longlong) mi_uint8korr(A))
#define mi_uint2korr(A) ((uint16) (((uint16) (((uint8_t*) (A))[1])) +\
                                   ((uint16) (((uint8_t*) (A))[0]) << 8)))
#define mi_uint3korr(A) ((uint32) (((uint32) (((uint8_t*) (A))[2])) +\
                                   (((uint32) (((uint8_t*) (A))[1])) << 8) +\
                                   (((uint32) (((uint8_t*) (A))[0])) << 16)))
#define mi_uint4korr(A) ((uint32) (((uint32) (((uint8_t*) (A))[3])) +\
                                   (((uint32) (((uint8_t*) (A))[2])) << 8) +\
                                   (((uint32) (((uint8_t*) (A))[1])) << 16) +\
                                   (((uint32) (((uint8_t*) (A))[0])) << 24)))
#define mi_uint5korr(A) ((uint64_t)(((uint32) (((uint8_t*) (A))[4])) +\
                                    (((uint32) (((uint8_t*) (A))[3])) << 8) +\
                                    (((uint32) (((uint8_t*) (A))[2])) << 16) +\
                                    (((uint32) (((uint8_t*) (A))[1])) << 24)) +\
                                    (((uint64_t) (((uint8_t*) (A))[0])) << 32))
#define mi_uint6korr(A) ((uint64_t)(((uint32) (((uint8_t*) (A))[5])) +\
                                    (((uint32) (((uint8_t*) (A))[4])) << 8) +\
                                    (((uint32) (((uint8_t*) (A))[3])) << 16) +\
                                    (((uint32) (((uint8_t*) (A))[2])) << 24)) +\
                        (((uint64_t) (((uint32) (((uint8_t*) (A))[1])) +\
                                    (((uint32) (((uint8_t*) (A))[0]) << 8)))) <<\
                                     32))
#define mi_uint7korr(A) ((uint64_t)(((uint32) (((uint8_t*) (A))[6])) +\
                                    (((uint32) (((uint8_t*) (A))[5])) << 8) +\
                                    (((uint32) (((uint8_t*) (A))[4])) << 16) +\
                                    (((uint32) (((uint8_t*) (A))[3])) << 24)) +\
                        (((uint64_t) (((uint32) (((uint8_t*) (A))[2])) +\
                                    (((uint32) (((uint8_t*) (A))[1])) << 8) +\
                                    (((uint32) (((uint8_t*) (A))[0])) << 16))) <<\
                                     32))
#define mi_uint8korr(A) ((uint64_t)(((uint32) (((uint8_t*) (A))[7])) +\
                                    (((uint32) (((uint8_t*) (A))[6])) << 8) +\
                                    (((uint32) (((uint8_t*) (A))[5])) << 16) +\


#define float4get(V, M)   do { *((float *) &(V)) = *((float*) (M)); } while(0)

typedef union {
  double v;
  long m[2];
} doubleget_union;

#define doubleget(V, M)  \
do { doubleget_union _tmp; \
     _tmp.m[0] = *((long*)(M)); \
     _tmp.m[1] = *(((long*) (M))+1); \
     (V) = _tmp.v; } while(0)

#define float8get(V, M)   doubleget((V),(M))

#define int2store(T, A)       do { uint def_temp= (uint) (A) ;\
        *((uchar*) (T))=  (uchar)(def_temp); \
        *((uchar*) (T)+1)=(uchar)((def_temp >> 8)); \
} while(0)

#define int3store(T, A)       do { /*lint -save -e734 */\
        *((uchar*)(T))=(uchar) ((A));\
        *((uchar*) (T)+1)=(uchar) (((A) >> 8));\
        *((uchar*)(T)+2)=(uchar) (((A) >> 16)); \
        /*lint -restore */} while(0)
#define int4store(T, A)       do { *((char *)(T))=(char) ((A));\
        *(((char *)(T))+1)=(char) (((A) >> 8));\
        *(((char *)(T))+2)=(char) (((A) >> 16));\
        *(((char *)(T))+3)=(char) (((A) >> 24)); } while(0)

bool str2num(const char *str, long &num);

int DecodeBase64Char(unsigned int ch);

int Base64Decode(unsigned char *szSrc, int nSrcLen, unsigned char *pbDest, int *pnDestLen);

bool DesDecrypt(const unsigned char *pInBuff,
                unsigned int uInLen,
                unsigned char *pOutBuff,
                unsigned int &uCtxLen,
                const unsigned char *pKey);

/*解析压缩过的int类型，lenenc-int*/
bool UnpackLenencInt(char *pBuf, uint32_t &uPos, uint64_t &uValue);


/*
 * time parse
 * */

#define DATETIME_MAX_DECIMALS 6


#define PARSE_PACKED_TIME_GET_INT_PART(x)     ((x) >> 24)
#define PARSE_PACKED_TIME_GET_FRAC_PART(x)    ((x) % (1LL << 24))
#define PARSE_PACKED_TIME_MAKE(i, f)          ((((int64_t) (i)) << 24) + (f))
#define PARSE_PACKED_TIME_MAKE_INT(i)         ((((int64_t) (i)) << 24))

int64_t parse_time_packed_from_binary(const char *ptr, uint32_t &pos,  uint32_t dec);

int64_t parse_datetime_packed_from_binary(const char *ptr, uint32_t &pos, uint32_t dec);

void parse_timestamp_from_binary(struct timeval *tm, const char *ptr, uint32_t &pos, uint32_t dec);

#endif //MYSQL2REDIS__UTIL_H_
