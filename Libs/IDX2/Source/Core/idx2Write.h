#pragma once


#include "Common.h"
#include "Error.h"
#include "idx2Common.h"


namespace idx2
{


struct encode_data;
struct channel;
struct sub_channel;
struct idx2_file;
struct params;


error<idx2_err_code>
FlushChunkExponents(const idx2_file& Idx2, encode_data* E);

void
WriteChunkExponents(const idx2_file& Idx2, encode_data* E, sub_channel* Sc, i8 Level, i8 Subband);

error<idx2_err_code>
FlushChunks(const idx2_file& Idx2, encode_data* E);

void
WriteChunk(const idx2_file& Idx2, encode_data* E, channel* C, i8 Iter, i8 Level, i16 BitPlane);

//void
//WriteMetaFile(const idx2_file& Idx2, cstr FileName);

void
WriteMetaFile(const idx2_file& Idx2, const params& P, cstr FileName);

void
WriteChunkExponents_v2(const idx2_file& Idx2, encode_data* E, sub_channel* Sc, i8 Level, i8 Subband);

error<idx2_err_code>
FlushChunkExponents_v2(const idx2_file& Idx2, encode_data* E);

void
WriteChunk_v2(const idx2_file& Idx2, encode_data* E, channel* C, i8 Level, i8 Subband, i16 BitPlane);

error<idx2_err_code>
FlushChunks_v2(const idx2_file& Idx2, encode_data* E);

void
PrintStats(cstr MetaFileName);

void
PrintStats_v2(cstr MetaFileName);

} // namespace idx2

