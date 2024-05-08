#include "../InputOutput.h"
#include "../BitStream.h"
#include "../Error.h"
#include "../Expected.h"
#include "../Timer.h"
#include "../VarInt.h"
#include "idx2Lookup_v2.h"
#include "idx2Read_v2.h"
#include "idx2Decode_v2.h"

namespace idx2
{

/* Given a brick address, open the file associated with the brick and cache its chunk information */
/* Structure of a file
* -------- beginning of file --------
* M
* L
* K
* J
* I
* H
* -------- exponent information ---------
* see function ReadFileExponents
* -------- end of file --------
*
* To parse the bit plane information, we parse the file backward:
* H : int32  = number of bit plane chunks
* I : int32  = size of J
* J : buffer = (zstd compressed) bit plane chunk addresses
* K : int32  = size (in bytes) of L
* L : buffer = (varint compressed) sizes of the bit plane chunks
* M : H buffers, whose sizes are encoded in L, each being one bit plane chunk
*/


/* Given a brick address, read the chunk associated with the brick and cache the chunk */

/* Read and decode the sizes of the compressed exponent chunks in a file */
/* Structure of a file
* -------- beginning of file --------
* bit plane information (see function ReadFile)
* -------- exponent information --------
* G
* F
* E
* D
* C
* B
* A
* -------- end of file --------
*
* To parse the exponent information, we parse the file backward:
*
* A : int32     = number of bytes for the exponent information
*               = A + B + C + D + E + F + G
* B : int32     = number of exponent chunks
* C : int32     = size (in bytes) of D
* D : buffer    = (zstd compressed) exponent chunk addresses
* E : int32     = size (in bytes) of F
* F : buffer    = (varint compressed) sizes of the exponent chunks
* G : B buffers, whose sizes are encoded in F, each being one exponent chunk
*/


/* Given a brick address, read the exponent chunk associated with the brick and cache it */
// TODO: remove the last two params (already stored in D)

} // namespace idx2

