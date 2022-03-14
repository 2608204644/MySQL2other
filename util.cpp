#include "util.h"

int DecodeBase64Char(unsigned int ch) {
  if (ch >= 'A' && ch <= 'Z')
    return ch - 'A' + 0;    // 0 range starts at 'A'
  if (ch >= 'a' && ch <= 'z')
    return ch - 'a' + 26;   // 26 range starts at 'a'
  if (ch >= '0' && ch <= '9')
    return ch - '0' + 52;   // 52 range starts at '0'
  if (ch == '+')
    return 62;
  if (ch == '/')
    return 63;
  return -1;
}

int Base64Decode(unsigned char *szSrc, int nSrcLen, unsigned char *pbDest, int *pnDestLen) {

  if (szSrc == NULL || pnDestLen == NULL) {
    return -1;
  }

  unsigned char *szSrcEnd = szSrc + nSrcLen;
  int nWritten = 0;

  int bOverflow = (pbDest == NULL) ? 1 : 0;
  //printf("over:%d\n", bOverflow);

  while (szSrc < szSrcEnd && (*szSrc) != 0) {
    unsigned int dwCurr = 0;
    int i;
    int nBits = 0;
    for (i = 0; i < 4; i++) {
      if (szSrc >= szSrcEnd)
        break;
      int nCh = DecodeBase64Char(*szSrc);
      szSrc++;
      if (nCh == -1) {
        // skip this char
        i--;
        continue;
      }
      dwCurr <<= 6;
      dwCurr |= nCh;
      nBits += 6;
    }

    if (!bOverflow && nWritten + (nBits / 8) > (*pnDestLen))
      bOverflow = 1;

    //printf("over:%d %d\n", bOverflow, szSrcEnd-szSrc);

    // dwCurr has the 3 bytes to write to the output buffer
    // left to right
    dwCurr <<= 24 - nBits;
    for (i = 0; i < nBits / 8; i++) {
      if (!bOverflow) {
        *pbDest = (unsigned char) ((dwCurr & 0x00ff0000) >> 16);
        pbDest++;
      }
      dwCurr <<= 8;
      nWritten++;
    }
  }

  *pnDestLen = nWritten;

  if (bOverflow) {
    if (pbDest != NULL) {
      //ATLASSERT(false);
    }

    return -2;
  }

  return 0;
}

bool DesDecrypt(const unsigned char *pInBuff,
                unsigned int uInLen,
                unsigned char *pOutBuff,
                unsigned int &uCtxLen,
                const unsigned char *pKey) {
  int nc = 0, ct_ptr = 0;
  unsigned int uBlockSize = 0;
  bool bRet = false;

  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
    return bRet;

  EVP_CIPHER_CTX_init(ctx);
  EVP_DecryptInit(ctx, EVP_des_ecb(), pKey, NULL);
  EVP_CIPHER_key_length(EVP_des_ecb());

  uBlockSize = EVP_CIPHER_CTX_block_size(ctx);
  uCtxLen = uInLen + uBlockSize - (uInLen % uBlockSize);

  unsigned char *cipherBuf = (unsigned char *) new char[uCtxLen + 1];
  memset(cipherBuf, 0, uCtxLen);

  do {
    if (EVP_DecryptUpdate(ctx, cipherBuf, &nc, pInBuff, uInLen) == 0)
      break;

    if (EVP_DecryptFinal(ctx, cipherBuf + nc, &ct_ptr) == 0)
      break;

    uCtxLen = nc + ct_ptr;
    memcpy(pOutBuff, cipherBuf, uCtxLen);
    bRet = true;
  } while (false);

  EVP_CIPHER_CTX_free(ctx);

  if (cipherBuf) {
    delete[] cipherBuf;
    cipherBuf = NULL;
  }

  return bRet;
}

//int<lenenc>是可变长度类型的整数，这种字符的长度可以是1, 3, 4, 9。
bool UnpackLenencInt(char *pBuf, uint32_t &uPos, uint64_t &uValue) {
  uint8_t u8Type = pBuf[0];
  if (u8Type < 0xfb) {
    uValue = u8Type;
    uPos += 1;
  } else if (u8Type == 0xfc) {
    uPos += 3;
    uValue = uint2korr(pBuf + 1);
  } else if (u8Type == 0xfd) {
    uPos += 4;
    uValue = uint3korr(pBuf + 1);
  } else if (u8Type == 0xfe) {
    uPos += 9;
    uValue = uint8korr(pBuf + 1);
  } else {
    return false;
  }
  return true;
}


/*
 * time parse
 * */

/*
   On disk we convert from signed representation to unsigned
   representation using TIMEF_OFS, so all values become binary comparable.
 */
#define TIMEF_OFS 0x800000000000LL
#define TIMEF_INT_OFS 0x800000LL

int64_t parse_time_packed_from_binary(const char *ptr, uint32_t &pos, uint32_t dec) {
  assert(dec <= DATETIME_MAX_DECIMALS);

  switch (dec) {
    case 0:
    default: {
      int64_t intpart = mi_uint3korr(ptr) - TIMEF_INT_OFS;
      pos += 3;
      return PARSE_PACKED_TIME_MAKE_INT(intpart);
    }
    case 1:
    case 2: {
      int64_t intpart = mi_uint3korr(ptr) - TIMEF_INT_OFS;
      int frac = (uint) ptr[3];
      pos += 3;
      if (intpart < 0 && frac) {
        /*
           Negative values are stored with reverse fractional part order,
           for binary sort compatibility.

           Disk value  intpart frac   Time value   Memory value
           800000.00    0      0      00:00:00.00  0000000000.000000
           7FFFFF.FF   -1      255   -00:00:00.01  FFFFFFFFFF.FFD8F0
           7FFFFF.9D   -1      99    -00:00:00.99  FFFFFFFFFF.F0E4D0
           7FFFFF.00   -1      0     -00:00:01.00  FFFFFFFFFF.000000
           7FFFFE.FF   -1      255   -00:00:01.01  FFFFFFFFFE.FFD8F0
           7FFFFE.F6   -2      246   -00:00:01.10  FFFFFFFFFE.FE7960

           Formula to convert fractional part from disk format
           (now stored in "frac" variable) to absolute value: "0x100 - frac".
           To reconstruct in-memory value, we shift
           to the next integer value and then substruct fractional part.
         */
        intpart++;    /* Shift to the next integer value */
        frac -= 0x100; /* -(0x100 - frac) */
      }
      return PARSE_PACKED_TIME_MAKE(intpart, frac * 10000);
    }

    case 3:
    case 4: {
      int64_t intpart = mi_uint3korr(ptr) - TIMEF_INT_OFS;
      int frac = mi_uint2korr(ptr + 3);
      pos += 5;
      if (intpart < 0 && frac) {
        /*
           Fix reverse fractional part order: "0x10000 - frac".
           See comments for FSP=1 and FSP=2 above.
         */
        intpart++;      /* Shift to the next integer value */
        frac -= 0x10000; /* -(0x10000-frac) */
      }
      return PARSE_PACKED_TIME_MAKE(intpart, frac * 100);
    }

    case 5:
    case 6:pos += 6;
      return ((int64_t) mi_uint6korr(ptr)) - TIMEF_OFS;
  }
}

#define DATETIMEF_INT_OFS 0x8000000000LL

/**
  Convert on-disk datetime representation
  to in-memory packed numeric representation.

  @param ptr   The pointer to read value at.
  @param dec   Precision.
  @return      In-memory packed numeric datetime representation.
 */
int64_t parse_datetime_packed_from_binary(const char *ptr, uint32_t &pos, uint32_t dec) {
  int64_t intpart = mi_uint5korr(ptr) - DATETIMEF_INT_OFS;
  pos += 5;
  int frac;
  assert(dec <= DATETIME_MAX_DECIMALS);
  switch (dec) {
    case 0:
    default:return PARSE_PACKED_TIME_MAKE_INT(intpart);
    case 1:
    case 2:frac = ((int) (signed char) ptr[5]) * 10000;
      pos += 1;
      break;
    case 3:
    case 4:frac = mi_sint2korr(ptr + 5) * 100;
      pos += 2;
      break;
    case 5:
    case 6:frac = mi_sint3korr(ptr + 5);
      pos += 3;
      break;
  }
  return PARSE_PACKED_TIME_MAKE(intpart, frac);
}

/**
  Convert binary timestamp representation to in-memory representation.

  @param  OUT tm  The variable to convert to.
  @param      ptr The pointer to read the value from.
  @param      dec Precision.
 */
void parse_timestamp_from_binary(struct timeval *tm, const char *ptr, uint32_t &pos, uint32_t dec) {
  assert(dec <= DATETIME_MAX_DECIMALS);
  tm->tv_sec = mi_uint4korr(ptr);
  pos += 4;
  switch (dec) {
    case 0:
    default:tm->tv_usec = 0;
      break;
    case 1:
    case 2:tm->tv_usec = ((int) ptr[4]) * 10000;
      pos += 4;
      break;
    case 3:
    case 4:tm->tv_usec = mi_sint2korr(ptr + 4) * 100;
      pos += 2;
      break;
    case 5:
    case 6:tm->tv_usec = mi_sint3korr(ptr + 4);
      pos += 3;
  }
}

