
import OpenVisus as ov
import os,sys

if __name__=="__main__":
	decoded=ov.LoadBinaryDocument("README.md")
	dims=ov.PointNi([decoded.c_size()])
	dtype=ov.DType.fromString("uint8")
	encoded = ov.Encode("zip",dims,dtype,decoded)
	decoded_check=ov.Decode("zip",dims,dtype,encoded)
	assert(ov.HeapMemory.equals(decoded,decoded_check))