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
#include <Visus/Log.h>

namespace Visus {


///////////////////////////////////////////////////////////////////////////////////////
class VISUS_DB_API Access
{
public:

  VISUS_NON_COPYABLE_CLASS(Access)

  //___________________________________________
  class VISUS_DB_API Statistics
  {
  public:

    Int64 rok=0,rfail=0;
    Int64 wok=0,wfail=0;
  };

  //empty by default
  String name;

  // if read/write ops are allowed 
  bool can_read=true;
  bool can_write=true;

  // bitsperblock for read/write operations 
  int bitsperblock=0;

  // statistics
  Statistics statistics;

  //bVerbose
  bool bVerbose = false;

  //constructor 
  Access() {
  }

  //destructor
  virtual ~Access() {
  }

  //this is the number of samples it will return in a read/write operation
  int getSamplesPerBlock() const {
    return 1 << bitsperblock;
  }

  //getFilename
  virtual String getFilename(Field field,double time,BigInt blockid) const {
    return "";
  }

  //getFilename
  String getFilename(SharedPtr<BlockQuery> query) const {
    return getFilename(query->field,query->time,query->getBlockNumber(bitsperblock));
  }

  //guessBlockFilenameTemplate
  static String guessBlockFilenameTemplate() {
    return "$(prefix)/time_$(time)/$(field)/$(block)$(suffix)";
  }

  //guessBlockFilename
  String guessBlockFilename(String prefix, Field field, double time, BigInt blockid, String suffix, String filename_template = guessBlockFilenameTemplate()) const;

  //getStartAddress
  Int64 getStartAddress(Int64 block_id) const {
    return block_id * getSamplesPerBlock();
  }

  //getEndAddress
  Int64 getEndAddress(Int64 block_id) const {
    return (block_id + 1) * getSamplesPerBlock();
  }

  //getMode
  String getMode() const {
    std::ostringstream out;
    if (isReading()) out << "r";
    if (isWriting()) out << "w";
    return out.str();
  }

  //isReading
  bool isReading() const {
    return bReading;
  }

  //isWriting
  bool isWriting() const {
    return bWriting;
  }


  //beginIO
  virtual void beginIO(String mode) {
    VisusAssert(!bReading && !bWriting);
    this->bReading = mode.find('r') != String::npos;
    this->bWriting = mode.find('w') != String::npos;
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query)=0;

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) = 0;

  //endIO
  virtual void endIO() {
    VisusAssert(bReading || bWriting);
    this->bReading = false;
    this->bWriting = false;
  }

  //beginRead
  void beginRead() {
    beginIO("r");
  }

  //endRead
  void endRead() {
    endIO();
  }

  //beginWrite
  void beginWrite() {
    beginIO("w");
  }

  //endWrite
  void endWrite() {
    endIO();
  }

  //beginReadWrite
  void beginReadWrite() {
    beginIO("rw");
  }

  //endWrite
  void endReadWrite() {
    endIO();
  }

  //in case you want first to read block, merge block samples, and finally write block you need a "lease" (using Microsoft Azure cloud terminology)
  virtual void acquireWriteLock(SharedPtr<BlockQuery> query) {
    ThrowException("not supported");
  }

  //releaseWriteLock
  virtual void releaseWriteLock(SharedPtr<BlockQuery> query) {
    ThrowException("not supported");
  }

  //resetStatistics
  void resetStatistics() {
    statistics=Statistics();
  }

  //printStatistics
  virtual void printStatistics() 
  {
    VisusInfo()<< "type(" << typeid(*this).name() << ") chdmod('" << (can_read ? "r" : "") << (can_write ? "w" : "") << "') bitsperblock(" << bitsperblock << ")";
    VisusInfo()<< "rok(" << statistics.rok << ") rfail(" << statistics.rfail << ")";
    VisusInfo()<< "wok(" << statistics.wok << ") wfail(" << statistics.wfail << ")";
  }

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) {
    ThrowException("not supported");
  }

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) {
    ThrowException("not supported");
  }

public:

  //readOk
  void readOk(SharedPtr<BlockQuery> query) {
    ++statistics.rok;
    query->setOk();
  }

  //readFailed
  void readFailed(SharedPtr<BlockQuery> query) {
    ++statistics.rfail;
    query->setFailed();
  }

  //writeOk
  void writeOk(SharedPtr<BlockQuery> query) {
    ++statistics.wok;
    query->setOk();
  }

  //writeFailed
  void writeFailed(SharedPtr<BlockQuery> query) {
    ++statistics.wfail;
    query->setFailed();
  }

private:

  bool bReading = false;
  bool bWriting = false;

}; //end class

} //namespace Visus

#endif //__VISUS_DB_ACCESS_H

