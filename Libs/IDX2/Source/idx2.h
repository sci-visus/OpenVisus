#pragma once

#include "Core/Core.h"


namespace idx2
{

/*
Initialize IDX2 with given parameters.
Call this function first.
*/
error<idx2_err_code>
InitFromBuffer(idx2_file* Idx2, params& P, buffer& Buf);

/*
Initialize IDX2 with given parameters.
Call this function first.
*/
error<idx2_err_code>
Init(idx2_file* Idx2, params& P);

struct brick_copier;


/*
Encode a volume.
*/
error<idx2_err_code>
Encode(idx2_file* Idx2, const params& P, brick_copier* Copier);


/*
Return the output grid.
*/
idx2::grid
GetOutputGrid(const idx2_file& Idx2, const params& P);


/*
Decode into a buffer.
*/
error<idx2_err_code>
Decode(idx2_file* Idx2, params& P, buffer* OutBuf);


/*
Deallocate all internal memory used by IDX2.
Call this function last to clean up.
*/
error<idx2_err_code>
Destroy(idx2_file* Idx2);


} // end namespace idx2
