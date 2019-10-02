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
}

///////////////////////////////////////////////////////////////
Model::~Model() {
  destroyed.emitSignal();
  VisusAssert(destroyed.empty());
}

///////////////////////////////////////////////////////////////
void Model::executeAction(StringTree action)
{
  if (action.name == "Assign")
  {
    readFrom(action);
    return;
  }

  if (action.name == "Transaction")
  {
    beginTransaction();
    {
      for (auto sub_action : action.childs)
      {
        if (!sub_action->isHashNode())
          executeAction(*sub_action);
      }
    }
    endTransaction();
    return;
  }

  if (action.name == "DiffAction")
  {
    auto patch = action.readText("patch");
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

    beginUpdate();
    readFrom(encoded);
    endUpdate();
  }

  ThrowException("internal error, unknown action " + action.name);
}


///////////////////////////////////////////////////////////////
bool Model::enableLog(String filename)
{
  if (log.is_open()) return true;
  log.open(filename.c_str(), std::fstream::out);
  log.rdbuf()->pubsetbuf(0, 0);
  log.rdbuf()->pubsetbuf(0, 0);
  return true;
}

///////////////////////////////////////////////////////////////
void Model::clearHistory()
{
  this->history = StringTree();
  this->log.close();
  this->redos = std::stack<StringTree>();
  this->undos = std::stack<StringTree>();
  this->undo_redo.clear();
  this->n_undo_redo = 0;
  this->bUndoingRedoing = false;
}

///////////////////////////////////////////////////////////////
void Model::beginUpdate(StringTree redo, StringTree undo)
{
  //collect actions...
  if (!redos.empty() && topRedo().name == "Transaction")
  {
    topRedo().childs.insert(topRedo().childs.end(), std::make_shared<StringTree>(redo)); //at the end 
    topUndo().childs.insert(topUndo().childs.begin(), std::make_shared<StringTree>(undo)); //at the beginning
  }

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
    if (redo.name == "DiffAction")
    {
      auto A = *redo.getFirstChild(); redo.childs.clear();
      auto B = this->encode();

      auto diff = Diff(
        StringUtils::getNonEmptyLines(A.toXmlString()),
        StringUtils::getNonEmptyLines(B.toXmlString()));

      redo.writeText("patch", diff.toString(),/*bCData*/true);
      undo.writeText("patch", diff.inverted().toString(),/*bCData*/true);
    }

    this->history.addChild(redo);

    if (!bUndoingRedoing)
    {
      this->undo_redo.resize(n_undo_redo++);
      this->undo_redo.push_back(std::make_pair(redo, undo));
    }

    if (this->log.is_open())
      this->log << redo.toString() << std::endl << std::endl;

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

