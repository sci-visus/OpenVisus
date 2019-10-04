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


#include <Visus/Model.h>
#include <Visus/Diff.h>

namespace Visus {

///////////////////////////////////////////////////////////////
Model::Model() {
  clearHistory();
}

///////////////////////////////////////////////////////////////
Model::~Model() {
  destroyed.emitSignal();
  VisusAssert(destroyed.empty());
}

///////////////////////////////////////////////////////////////
void Model::executeAction(StringTree redo)
{
  if (redo.name == "copy")
  {
    return copy(*this, redo);
  }

  if (redo.name == "transaction")
  {
    beginUpdate(redo, Transaction());
    {
      for (auto sub_action : redo.childs)
      {
        if (!sub_action->isHashNode())
          executeAction(*sub_action);
      }
    }
    endUpdate();
    return;
  }

  if (redo.name == "diff")
  {
    auto patch = redo.readText("patch");
    auto diff = Diff(StringUtils::getNonEmptyLines(patch));
    if (diff.empty())
      return;

    auto encoded = this->encode();

    std::vector<String> curr = StringUtils::getNonEmptyLines(encoded.toXmlString());
    std::vector<String> next = diff.applyDirect(curr);

    encoded = StringTree::fromString(StringUtils::join(next, "\r\n"));
    if (!encoded.valid())
    {
      String error_msg = StringUtils::format() << "Error ApplyPatch::applyPatch()\r\n"
        << "diff:\r\n" << "[[" << diff.toString() << "]]\r\n"
        << "curr:\r\n" << "[[" << StringUtils::join(curr, "\r\n") << "]]\r\n"
        << "next:\r\n" << "[[" << StringUtils::join(next, "\r\n") << "]]\r\n\r\n";

      ThrowException(error_msg);
    }

    beginUpdate(redo, StringTree("diff"));
    {
      readFrom(encoded);
    }
    endUpdate();
  }

  ThrowException("internal error, unknown action " + redo.name);
}


///////////////////////////////////////////////////////////////
void Model::enableLog(String filename)
{
  if (log.is_open())
    log.close();

  this->log_filename = filename;

  if (!filename.empty())
  {
    log.open(log_filename.c_str(), std::fstream::out);
    log.rdbuf()->pubsetbuf(0, 0);
    log.rdbuf()->pubsetbuf(0, 0);
  }
}

///////////////////////////////////////////////////////////////
void Model::clearHistory()
{
  this->history = StringTree();
  this->redos = std::stack<StringTree>();
  this->undos = std::stack<StringTree>();
  this->undo_redo.clear();
  this->n_undo_redo = 0;
  this->bUndoingRedoing = false;
  this->utc = Time::now().getUTCMilliseconds();
  enableLog(this->log_filename);
}

///////////////////////////////////////////////////////////////
void Model::beginUpdate(StringTree redo, StringTree undo)
{
  auto utc = Time::now().getUTCMilliseconds()-this->utc;
  redo.write("utc", utc);
  undo.write("utc", utc);

  undo.write("is_undo", true);

  //note only the root action is important
  if (!isUpdating() && (redo.name == "diff" || undo.name == "diff"))
    this->diff_begin = this->encode();

  //collect at the end
  if (!redos.empty() && topRedo().name == "transaction")
    topRedo().childs.insert(topRedo().childs.end(), std::make_shared<StringTree>(redo)); 

  //collect at the beginning
  if (!undos.empty() && topUndo().name == "transaction")
    topUndo().childs.insert(topUndo().childs.begin(), std::make_shared<StringTree>(undo));

  redos.push(redo);
  undos.push(undo);

  if (bool bBegin = (redos.size() == 1))
    begin_update.emitSignal();
}

///////////////////////////////////////////////////////////////
void Model::endUpdate()
{
  //final one?
  if (redos.size() == 1)
  {
    auto& redo = topRedo();
    auto& undo = topUndo();

    //special case for diff action
    if (redo.name == "diff" || undo.name=="diff")
    {
      auto A = diff_begin;
      auto B = this->encode();

      auto diff = Diff(
        StringUtils::getNonEmptyLines(A.toXmlString()),
        StringUtils::getNonEmptyLines(B.toXmlString()));

      if (redo.name == "diff")
        redo.writeCode("patch", diff.toString());

      if (undo.name == "diff")
        undo.writeCode("patch", diff.inverted().toString());
    }

    this->diff_begin = StringTree();

    this->history.addChild(redo);

    if (!bUndoingRedoing)
    {
      this->undo_redo.resize(n_undo_redo++);
      this->undo_redo.push_back(std::make_pair(redo, undo));
    }

    if (this->log.is_open())
      this->log << redo.toString() << std::endl;

    this->modelChanged();
    this->end_update.emitSignal();
  }

  this->redos.pop();
  this->undos.pop();
}

///////////////////////////////////////////////////////////////
bool Model::redo() {
  VisusAssert(VisusHasMessageLock());
  VisusAssert(!bUndoingRedoing);
  if (!canRedo())  return false;
  auto action = undo_redo[n_undo_redo++].first;
  bUndoingRedoing = true;
  executeAction(action);
  bUndoingRedoing = false;
  return true;
}

///////////////////////////////////////////////////////////////
bool Model::undo() {
  VisusAssert(VisusHasMessageLock());
  VisusAssert(!bUndoingRedoing);
  if (!canUndo()) return false;
  auto action = undo_redo[--n_undo_redo].second;
  bUndoingRedoing = true;
  executeAction(action);
  bUndoingRedoing = false;
  return true;
}

} //namespace Visus

