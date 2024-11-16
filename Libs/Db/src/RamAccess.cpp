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

#include <Visus/RamAccess.h>
#include <Visus/Dataset.h>

namespace Visus {

////////////////////////////////////////////////////////////////////
class RamAccess::Shared 
{
public:

  //_____________________________________________________________
  class Key
  {
  public:
  
    String   fieldname;
    double   time;
    BigInt   blockid; 

    //constructor
    inline Key(String fieldname_,double time_,BigInt blockid_) : fieldname(fieldname_),time(time_), blockid(blockid_)
    {}

    //operator==
    inline bool operator==(const Key& other) const 
    {return blockid ==other.blockid && time==other.time && fieldname==other.fieldname;}

    //operator<
    inline bool operator<(const Key& other) const 
    {
      return (blockid <other.blockid)
          || (blockid ==other.blockid && time<other.time)
          || (blockid ==other.blockid && time==other.time && fieldname<other.fieldname);
    }

    //valid
    inline bool valid() 
    {return blockid >=0 && !fieldname.empty();}

  };

  //________________________________________________________________
  class Cached
  {
  public:
    Field                        field;
    double                       time=0;
    BigInt                       blockid =0;
    Array                        buffer;
    Cached                       *prev=nullptr,*next=nullptr;

    inline Key key() {
      return Key(field.name,time, blockid);
    }
  };

  CriticalSection        lock;
  Int64                  available=0,used=0;
  Cached                 *front=nullptr,*back=nullptr;
  std::map<Key,Cached*>  index;

  //constructor
  Shared(Int64 available_) : available(available_) {  
  }

  //destructor
  ~Shared() {
    while (!index.empty()) 
      delete remove(back);

    VisusAssert(used==0);
  }

  //read
  bool read(SharedPtr<BlockQuery> query) 
  {
    ScopedLock lock(this->lock);

    Key key(query->field.name, query->time, query->blockid);
    auto it = index.find(key);
    VisusAssert(it == index.end() || it->second->key() == key);

    if (it == index.end())
      return false;

    query->buffer = it->second->buffer.clone();

    VisusAssert(query->field.name == it->second->field.name);
    VisusAssert(query->time == it->second->time);
    VisusAssert(query->blockid == it->second->blockid);

    if (it->second != front)
      push_front(remove(it->second));

    //TODO: statistics without lock?
    return true;
  }

  //write
  bool write(SharedPtr<BlockQuery> query) 
  {
    ScopedLock lock(this->lock);

    Key key(query->field.name, query->time, query->blockid);
    auto it = index.find(key);
    VisusAssert(it == index.end() || it->second->key() == key);

    Cached* cached = (it == index.end()) ? 0 : it->second;
    if (!cached)
    {
      while (available>0 && used >= available) 
        delete remove(back);
      cached = new Cached();
    }

    cached->buffer = query->buffer.clone();
    cached->field = query->field;
    cached->time = query->time;
    cached->blockid = query->blockid;

    if (it == index.end())
      push_front(cached);

    else if (cached != front)
      push_front(remove(cached));

    //TODO: statistics without lock?
    return true;
  }

private:

  //push_front
  void push_front(Cached* cached)
  {
    VisusAssert(cached->key().valid());
    VisusAssert(!cached->prev && !cached->next);
    cached->prev=0;
    cached->next=this->front;
    if (cached->prev) cached->prev->next=cached; else this->front = cached;
    if (cached->next) cached->next->prev=cached; else this->back  = cached;
    VisusAssert((cached->prev || front==cached) && (cached->next || back==cached));
    this->used+=cached->buffer.c_size();
    index[cached->key()]=cached;
  }

  //remove
  Cached* remove(Cached* cached)
  {
    VisusAssert(cached->key().valid());
    VisusAssert((cached->prev || front==cached) && (cached->next || back==cached));
    if (cached->prev) cached->prev->next=cached->next; else front=cached->next;
    if (cached->next) cached->next->prev=cached->prev; else back =cached->prev;
    cached->prev=cached->next=0;
    index.erase(cached->key());
    this->used-=cached->buffer.c_size();
    return cached;
  }

};


////////////////////////////////////////////////////////////////////////////////
RamAccess::RamAccess(int bitsperblock,StringTree config) {
  this->name = "RamAccess";
  this->bitsperblock = bitsperblock;
  this->can_read = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "r");
  this->can_write = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "w");
  this->setAvailableMemory(StringUtils::getByteSizeFromString(config.readString("available", "128mb")));

}


////////////////////////////////////////////////////////////////////////////////
RamAccess::~RamAccess() {
}

////////////////////////////////////////////////////////////////////////////////
void RamAccess::setAvailableMemory(Int64 value)
{
  this->shared = std::make_shared<Shared>(value);
}

////////////////////////////////////////////////////////////////////////////////
void RamAccess::shareMemoryWith(SharedPtr<RamAccess> value)
{
  this->shared = value->shared;
}

////////////////////////////////////////////////////////////////////////////////
void RamAccess::readBlock(SharedPtr<BlockQuery> query)  
{
  return shared->read(query)? readOk(query):readFailed(query,"not found");
}

////////////////////////////////////////////////////////////////////////////////
void RamAccess::writeBlock(SharedPtr<BlockQuery> query)  
{
  return shared->write(query)? writeOk(query):writeFailed(query,"not found");
}


////////////////////////////////////////////////////////////////////////////////
void RamAccess::printStatistics()  {
  
  Access::printStatistics();

  {
    ScopedLock lock(shared->lock);
    PrintInfo("RAM used", StringUtils::getStringFromByteSize(shared->used));
    PrintInfo("RAM available", StringUtils::getStringFromByteSize(shared->available));
  }
}

} //namespace Visus 

