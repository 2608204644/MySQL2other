//
// Created by cha on 2022/3/12.
//

#ifndef MYSQL2REDIS__MEMBLOCK_H_
#define MYSQL2REDIS__MEMBLOCK_H_

#include <stdint.h>
#include <string.h>
#include <assert.h>

class MemBlock
{
 public:
  MemBlock();
  ~MemBlock();

  int Assign(char *pBuf, uint32_t uSize);
  char At(uint32_t uIndex) const;
  bool GetBit(uint32_t uPos) const;
  uint32_t GetBitsetCount() const;
  uint32_t GetBitsetCount(uint32_t uSize) const;

 public:
  uint32_t    m_uSize;
  char        *m_pBlock;
};
#endif //MYSQL2REDIS__MEMBLOCK_H_
