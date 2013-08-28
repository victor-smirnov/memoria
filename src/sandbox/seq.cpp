

#include <memoria/memoria.hpp>

#include <memoria/core/packed/packed_seq.hpp>

#include <memoria/tools/tools.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;


//#define ONES_STEP_4 ( 0x1111111111111111ULL )
//#define ONES_STEP_8 ( 0x0101010101010101ULL )
//#define ONES_STEP_16 ( 1ULL << 0 | 1ULL << 16 | 1ULL << 32 | 1ULL << 48 )
//#define MSBS_STEP_4 ( 0x8ULL * ONES_STEP_4 )
//#define MSBS_STEP_8 ( 0x80ULL * ONES_STEP_8 )
//#define MSBS_STEP_16 ( 0x8000ULL * ONES_STEP_16 )
//#define INCR_STEP_8 ( 0x80ULL << 56 | 0x40ULL << 48 | 0x20ULL << 40 | 0x10ULL << 32 | 0x8ULL << 24 | 0x4ULL << 16 | 0x2ULL << 8 | 0x1 )
//
//#define LEQ_STEP_8(x,y) ( ( ( ( ( (y) | MSBS_STEP_8 ) - ( (x) & ~MSBS_STEP_8 ) ) ^ (x) ^ (y) ) & MSBS_STEP_8 ) >> 7 )
//
//#define ZCOMPARE_STEP_8(x) ( ( ( x | ( ( x | MSBS_STEP_8 ) - ONES_STEP_8 ) ) & MSBS_STEP_8 ) >> 7 )




UBigInt BroadwordGTZ8_ (UBigInt x)
{
    UBigInt H8 = 0x8080808080808080ull;
    UBigInt L8 = 0x0101010101010101ull;

    return  ((x | (( x | H8) - L8)) & H8) >> 7;
}

UBigInt BroadwordLE8_ (UBigInt x, UBigInt y)
{
    UBigInt H8 = 0x8080808080808080ull;
    return ((((y | H8) - (x & ~H8)) ^ x ^ y) & H8 ) >> 7;
}

Int BroadwordSelect0(UBigInt x, Int r)
{
    UBigInt L8 = 0x0101010101010101ull;
    UBigInt s = x - ((x & 0xAAAAAAAAAAAAAAAAull) >> 1);
    s = (s & 0x3333333333333333ull) + ((s >> 2) & 0x3333333333333333ull);
    s = (s + (s >> 4)) & (0x0f * L8);
    s*= L8;

    UBigInt leq = BroadwordLE8_(s, r * L8);


    UBigInt b = (leq * L8 >> 53) & ~0x7;
    UBigInt l = r - (((s << 8) >> b) & 0xFF);


    s = BroadwordGTZ8_((x >> b & 0xFF) * L8 & 0x8040201008040201) * L8;

    return b + (BroadwordLE8_(s, l * L8) * L8 >> 56);
}

//static int select( const UBigInt x, const int k ) { /* k < 128; returns 72 if there are less than k ones in x. */
//
//   // Phase 1: sums by byte
//   register UBigInt byte_sums = x - ( ( x & 0xa * ONES_STEP_4 ) >> 1 );
//   byte_sums = (byte_sums & 0x3333333333333333ull) + ((byte_sums >> 2) & 0x3333333333333333ull);
//   byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
//   byte_sums *= ONES_STEP_8;
//
//   // Phase 2: compare each byte sum with k
//   const UBigInt k_step_8 = k * ONES_STEP_8;
//
//   UBigInt leq = LEQ_STEP_8( byte_sums, k_step_8 );
//
//   const UBigInt place = ( leq * ONES_STEP_8 >> 53 ) & ~0x7;
//
//   // Phase 3: Locate the relevant byte and make 8 copies with incrental masks
//   const int byte_rank = k - ( ( ( byte_sums << 8 ) >> place ) & 0xFF );
//
//   const UBigInt spread_bits = ( x >> place & 0xFF ) * ONES_STEP_8 & INCR_STEP_8;
//   const UBigInt bit_sums = ZCOMPARE_STEP_8( spread_bits ) * ONES_STEP_8;
//
//   // Compute the inside-byte location and return the sum
//   const UBigInt byte_rank_step_8 = byte_rank * ONES_STEP_8;
//
//   return place + ( LEQ_STEP_8( bit_sums, byte_rank_step_8 ) * ONES_STEP_8 >> 56 );
//}

size_t SelectR(UBigInt x, Int rank)
{
    for (size_t c = 0; c < sizeof(x)*8; c++)
    {
        if (PopCnt(x & MakeMask<UBigInt>(0, c)) == rank)
        {
            return c;
        }
    }

    return 72;
}

inline UBigInt PopCntB(UBigInt v)
{
    v -= ((v >> 1) & 0x5555555555555555);
    v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F;
    return v;
}

size_t SelectH(UBigInt arg, size_t rank)
{
    UBigInt v = arg;

    v -= ((v >> 1) & 0x5555555555555555);
    v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F;

    UInt argl = static_cast<UInt>(v + (v >> 32));
    argl += argl >> 16;
    size_t full_rank = (argl + (argl >> 8)) & 0x7F;

    if (full_rank >= rank)
    {
        size_t r = 0;

        for (Int shift = 0; shift < 64; shift += 8)
        {
            UBigInt popc =  (v >> shift) & 0xFF;

            if (r + popc >= rank)
            {
                UBigInt mask = 1ull << shift;

                for (size_t d = 0; d < 8; d++, mask <<= 1)
                {
                    if (r == rank)
                    {
                        return shift + d;
                    }
                    else {
                        r += ((arg & mask) != 0);
                    }
                }
            }
            else {
                r += popc;
            }
        }

        return 72; // this shouldn't happen
    }
    else {
        return 72;//100 + full_rank;
    }
}









int main(void) {

    Int seed = getTimeInMillis() % 10000;
    // Emulate seed;
    for (Int c = 0; c < seed; c++)
    {
        getRandom();
        getBIRandom();
    }


    cout<<SelectH(-1ull, 64)<<endl;


    UBigInt rankt = getBIRandom();

//  UBigInt rankt = 0xFFFF;

//  UByte rankb = 0x00;

//  cout<<"Arg="<<hex<<rankt<<dec<<endl;
//
//  cout<<SelectR(rankt, 20)<<endl;
//  cout<<SelectH(rankt, 20)<<endl;
//
//  for (Int c = 0; c < 64; c++)
//  {
//      cout<<c<<" ";
//      cout<<SelectR(rankt, c)<<" ";
//      cout<<SelectH(rankt, c)<<" ";
//      cout<<BroadwordSelect0(rankt, c)<<" ";
//
//      cout<<PopCnt(rankt & MakeMask<UBigInt>(0, c))<<endl;
//  }
//
//
//  cout<<hex<<PopCntB(rankt)<<endl;
//  cout<<PopCnt(rankt)<<endl;
//


    UBigInt sum = 0;
    Int     MAX = 100000000;
    BigInt t0 = getTimeInMillis();



    for (Int c = 0, rank = 1; c < MAX; c++, rank += 1)
    {
        if (rank > 31) {
            rank = 1;
        }

        sum += SelectH(rankt + c, rank);

    }

    BigInt t1 = getTimeInMillis();


    for (Int c = 0, rank = 1; c < MAX; c++, rank += 1)
    {
        if (rank > 31) {
            rank = 1;
        }

        sum += BroadwordSelect0(rankt + c, rank);
    }

    BigInt t2 = getTimeInMillis();

    cout<<FormatTime(t1-t0)<<endl;
    cout<<FormatTime(t2-t1)<<endl;

    cout<<sum<<endl;


//  UBigInt arg = 0xAABBCCDDEEFF1123;

//  UBigInt arg = -1ull;
//
//  size_t total = 0;
//  size_t stop = 64;
//
//  UBigInt count = 2;
//
//  bool result = intrnl2::SelectFW(arg, total, count, stop);
//
//  cout<<"rank: "<<PopCnt(arg & MakeMask<UBigInt>(0, stop))<<endl;
//
//  cout<<result<<" "<<stop<<endl;
//  cout<<select(arg, count)<<endl;
//  cout<<BroadwordSelect0(arg, count)<<endl;
//
//  cout<<SelectR(arg, count)<<endl;
//
//  for (size_t c = 0; c <= 64; c++)
//  {
//      size_t p1 = select(arg, c);
//      size_t p2 = BroadwordSelect0(arg, c);
//      size_t p3 = SelectR(arg, c);
//
//      cout<<c<<" "<<p1<<" "<<p2<<" "<<p3<<endl;
//  }


//  for (Int c = 0; c <= 100000000; c++)
//  {
//      UBigInt arg = 0xAABBCCDDEEFF1122;
//
////        total = 0;
////        bool result = intrnl2::SelectFW(arg + c, total, 10, stop);
//      size_t pos1 = SelectH(arg + c, 20);
//      size_t pos2 = BroadwordSelect0(arg + c, 20);
//
//      if (pos1 != pos2)
//      {
//          cout<<"Unequal! "<<arg + c<<" "<<pos1<<" "<<pos2<<" "<<c<<endl;
//      }
//  }

    return 0;
}

