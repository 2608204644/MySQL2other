#include "memblock.h"

MemBlock::MemBlock() {
  m_uSize = 0;
  m_pBlock = nullptr;
}

MemBlock::~MemBlock() {
  if (m_pBlock != nullptr) {
    delete m_pBlock;
    m_pBlock = nullptr;
    m_uSize = 0;
  }
}

int MemBlock::Assign(char *pBuf, uint32_t uSize) {
  if (m_pBlock != nullptr) {
    delete m_pBlock;
    m_pBlock = nullptr;
    m_uSize = 0;
  }

  m_pBlock = new char[uSize];
  if (m_pBlock == nullptr)
    return -1;

  memcpy(m_pBlock, pBuf, uSize);
  m_uSize = uSize;

  return 0;
}

char MemBlock::At(uint32_t uIndex) const {
  assert(uIndex < m_uSize);
  return *(m_pBlock + uIndex);
}

//判断某个bit位是否为 1
bool MemBlock::GetBit(uint32_t uPos) const {
  uint32_t mchar = uPos / 8;
  uint32_t nbit = uPos & 0x07;
  assert(mchar < m_uSize);
  return ((m_pBlock[mchar] >> nbit) & 0x01) == 0x01;
}

uint32_t MemBlock::GetBitsetCount() const {
  uint32_t uCount = 0;
  for (uint32_t i = 0; i < m_uSize; i++) {
    uint8_t p = *(m_pBlock + i);
    while (p != 0) {
      if ((p & 0x01) != 0)
        uCount++;
      p = p >> 1;
    }
  }

  return uCount;
}

uint32_t MemBlock::GetBitsetCount(uint32_t uBitsetSize) const {
  uint32_t uCount = 0;
  assert(uBitsetSize <= m_uSize * 8);
  for (uint32_t i = 0; i < uBitsetSize; i++) {
    if (GetBit(i))
      uCount++;
  }

  return uCount;
}
