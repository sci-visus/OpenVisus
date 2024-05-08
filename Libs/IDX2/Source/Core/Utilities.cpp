#include "Utilities.h"


namespace idx2
{


u32
Murmur3_32(u8* Key, int Len, u32 Seed)
{
  u32 H = Seed;
  if (Len > 3)
  {
    u32* Key_x4 = (u32*)Key;
    int I = Len >> 2;
    do
    {
      u32 K = *Key_x4++;
      K *= 0xcc9e2d51;
      K = (K << 15) | (K >> 17);
      K *= 0x1b873593;
      H ^= K;
      H = (H << 13) | (H >> 19);
      H = (H * 5) + 0xe6546b64;
    } while (--I);
    Key = (u8*)Key_x4;
  }
  if (Len & 3)
  {
    int I = Len & 3;
    u32 K = 0;
    Key = &Key[I - 1];
    do
    {
      K <<= 8;
      K |= *Key--;
    } while (--K);
    K *= 0xcc9e2d51;
    K = (K << 15) | (K >> 17);
    K *= 0x1b873593;
    H ^= K;
  }
  H ^= Len;
  H ^= H >> 16;
  H *= 0x85ebca6b;
  H ^= H >> 13;
  H *= 0xc2b2ae35;
  H ^= H >> 16;
  return H;
}


} // namespace idx2
