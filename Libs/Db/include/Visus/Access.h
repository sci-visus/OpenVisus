/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#ifndef __VISUS_DB_ACCESS_H
#define __VISUS_DB_ACCESS_H

#include <Visus/Db.h>
#include <Visus/BlockQuery.h>

namespace Visus {


///////////////////////////////////////////////////////////////////////////////////////
class VISUS_DB_API AccessStatistics
{
public:

  Int64 rok = 0, rfail = 0;
  Int64 wok = 0, wfail = 0;

  //reset
  void reset()
  {
    rok = rfail = 0;
    wok = wfail = 0;
  }
};


///////////////////////////////////////////////////////////////////////////////////////
class VISUS_DB_API Access
{
public:

  VISUS_NON_COPYABLE_CLASS(Access)

    typedef AccessStatistics Statistics;

  static const String DefaultChMod;

  //empty by default
  String name;

  // if read/write ops are allowed 
  bool can_read = true;
  bool can_write = true;

  // bitsperblock for read/write operations 
  int bitsperblock = 0;

  // statistics
  Statistics statistics;

  //verbose (1==r and w; 2==only w)
  int verbose = 0;

  //bDisableWriteLocks (to speed up writing with only one writer)
  bool bDisableWriteLocks = false;

  //internal use only
  /*
    Remote:
      CloudAccess    (*) Access::compression (*) zip
      ModVisusAccess (*) Access::compression (*) zip

    Local:
      IdxDiskAccess  (*) Access::compression (*) field.default_compression
      DiskAccess     (*) Access::compression (*) field.default_compression
  */
  String compression;

  //constructor 
  Access() {
  }

  //getAccessTypeName
  String getAccessTypeName() {
    return typeid(*this).name();
  }

  //destructor
  virtual ~Access() {
  }

  //this is the number of samples it will return in a read/write operation
  int getSamplesPerBlock() const {
    return 1 << bitsperblock;
  }

  //getFilename
  virtual String getFilename(Field field, double time, BigInt blockid) const {
    return "";
  }

  //getFilename
  String getFilename(SharedPtr<BlockQuery> query) const {
    return getFilename(query->field, query->time, query->blockid);
  }

  //getCompression
  String getCompression(String default_compression = "zip") const {
    return !this->compression.empty() ? this->compression : default_compression;
  }

  //getStartAddress
  Int64 getStartAddress(Int64 block_id) const {
    return block_id * getSamplesPerBlock();
  }

  //getEndAddress
  Int64 getEndAddress(Int64 block_id) const {
    return (block_id + 1) * getSamplesPerBlock();
  }

  //getMode
  int getMode() const {
    return this->mode;
  }

  //isWriting
  bool isWriting() const {
    return this->mode == 'w';
  }

  //isReading
  bool isReading() const {
    return this->mode == 'r';
  }

  //beginIO
  virtual void beginIO(int mode) {
    VisusReleaseAssert(this->mode == 0);
    this->mode = mode;
  }

  //endIO
  virtual void endIO() {
    VisusReleaseAssert(this->mode != 0);
    this->mode = 0;
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) = 0;

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) = 0;

  //beginRead
  void beginRead() {
    beginIO('r');
  }

  //endRead
  void endRead() {
    endIO();
  }

  //beginWrite
  void beginWrite() {
    beginIO('w');
  }

  //endWrite
  void endWrite() {
    endIO();
  }

  //in case you want first to read block, merge block samples, and finally write block you need a "lease" (using Microsoft Azure cloud terminology)
  virtual void acquireWriteLock(SharedPtr<BlockQuery> query) {
    VisusAssert(isWriting());
    if (bDisableWriteLocks) return;
    ThrowException("Access::releaseWriteLock not supported");
  }

  //releaseWriteLock
  virtual void releaseWriteLock(SharedPtr<BlockQuery> query) {
    VisusAssert(isWriting());
    if (bDisableWriteLocks) return;
    ThrowException("Access::releaseWriteLock not supported");
  }

  //resetStatistics
  void resetStatistics() {
    statistics = Statistics();
  }

  //printStatistics
  virtual void printStatistics()
  {
    PrintInfo("type", typeid(*this).name(), "chmod", can_read ? "r" : "", can_write ? "w" : "", "bitsperblock", bitsperblock);
    PrintInfo("rok", statistics.rok, "rfail", statistics.rfail);
    PrintInfo("wok", statistics.wok, "wfail", statistics.wfail);
  }

  //write
  virtual void write(Archive& ar) const {
    ThrowException("not supported");
  }

  //read
  virtual void read(Archive& ar) {
    ThrowException("not supported");
  }

public:

  //readOk
  void readOk(SharedPtr<BlockQuery> query) {
    ++statistics.rok;
    query->setOk();
  }

  //readFailed
  void readFailed(SharedPtr<BlockQuery> query, String errormsg) {
    ++statistics.rfail;
    query->setFailed(errormsg);
  }

  //writeOk
  void writeOk(SharedPtr<BlockQuery> query) {
    ++statistics.wok;
    query->setOk();
  }

  //writeFailed
  void writeFailed(SharedPtr<BlockQuery> query, String errormsg) {
    ++statistics.wfail;
    query->setFailed(errormsg);
  }

public:

  //getBlockFilename
  static String getBlockFilename(Dataset* dataset, int bitsperblock, String filename_template, Field field, double time, String compression, BigInt blockid, bool reverse_filename);

  //disableWriteLocks
  //99% of the times I am not writing in parallel to avoid the file lock thingy
  void disableWriteLocks() {
    this->bDisableWriteLocks = true;
  }

  //disableCompression (need it anytime I am executing a BoxQuery that is going to write the sample blocks multiple times)
  void disableCompression() {
    this->compression = "raw"; //note: setting a non-empty value i force the Access::compression to be returned
  }

  //disableAsync
  virtual void disableAsync() {
    //nop here
  }

private:

  int mode=0;

}; //end class

} //namespace Visus

#endif //__VISUS_DB_ACCESS_H

