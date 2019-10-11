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
String Model::popTargetId(StringTree& action) {
  auto v = StringUtils::split(action.readString("target_id"), "/");
  if (v.empty()) return "";
  auto left = v[0];
  auto right = StringUtils::join(std::vector<String>(v.begin() + 1, v.end()), "/");
  action.removeAttribute("target_id");  //i want the target_id at the beginning of attributes
  action.attributes.insert(action.attributes.begin(), std::make_pair("target_id", right));
  return left;
}

///////////////////////////////////////////////////////////////
void Model::pushTargetId(StringTree& action, String target_id) {
  auto right = action.readString("target_id");
  if (!right.empty()) target_id = target_id + "/" + right;
  action.removeAttribute("target_id"); //i want the target_id at the beginning of attributes
  action.attributes.insert(action.attributes.begin(), std::make_pair("target_id", target_id));
}

///////////////////////////////////////////////////////////////
StringTree Model::createPassThroughAction(StringTree action, String target_id) {
  auto ret = action;
  pushTargetId(ret, target_id);
  return ret;
}

///////////////////////////////////////////////////////////////
bool Model::getPassThroughAction(StringTree& action, String match) {
  auto v = StringUtils::split(action.readString("target_id"), "/");
  if (v.empty() || v[0] != match) return false;
  popTargetId(action);
  return true;
}

///////////////////////////////////////////////////////////////
void Model::copy(Model& dst, StringTree encoded)
{
  //before updating I need a backup
  StringTree undo("copy");
  dst.write(undo);

  dst.beginUpdate(encoded, undo);
  dst.read(encoded);
  dst.endUpdate();
}

///////////////////////////////////////////////////////////////
void Model::copy(Model& dst, const Model& src) {
  StringTree encoded("copy");
  src.write(encoded);
  return copy(dst, encoded);
}

///////////////////////////////////////////////////////////////
void Model::execute(Archive& ar)
{
  if (ar.name == "copy")
  {
    return copy(*this, ar);
  }

  if (isTransaction(ar))
  {
    beginUpdate(Transaction(), Transaction());
    {
      for (auto sub_action : ar.childs)
      {
        if (!sub_action->isHash())
          execute(*sub_action);
      }
    }
    endUpdate();
    return;
  }

  if (isDiff(ar))
  {
    String patch; ar.readText("patch", patch);
    auto diff = Visus::Diff(StringUtils::getNonEmptyLines(patch));
    if (diff.empty())
      return;

    StringTree encoded(this->getTypeName());
    this->write(encoded);

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

    beginUpdate(ar, Diff());
    {
      read(encoded);
    }
    endUpdate();
  }

  ThrowException("internal error, unknown action " + ar.name);
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
  VisusAssert(!isUpdating());
  VisusAssert(!bUndoing);
  VisusAssert(!bRedoing);
  this->history = StringTree();
  this->redo_stack = std::stack<StringTree>();
  this->undo_stack = std::stack<StringTree>();
  this->undo_redo.clear();
  this->cursor_undo_redo = 0;
  this->bUndoing = false;
  this->bRedoing = false;
  this->utc = Time::now().getUTCMilliseconds();
  enableLog(this->log_filename);
}

///////////////////////////////////////////////////////////////
void Model::beginUpdate(StringTree redo, StringTree undo)
{
  bool bTopLevel = (redo_stack.size() == 0);

  //only top level (note: it's important to overwrite the utc in case of undo/redo)
  if (bTopLevel)
  {
    auto utc = Time::now().getUTCMilliseconds() - this->utc;
    redo.write("utc", utc);
    undo.write("utc", utc);
  }

  //note only the root action is important
  if (bTopLevel && (isDiff(redo) || isDiff(undo)))
  {
    StringTree encoded(this->getTypeName());
    this->write(encoded);
    this->diff_begin = encoded;
  }

  this->redo_stack.push(redo);
  this->undo_stack.push(undo);

  //emit signal
  if (bTopLevel)
    begin_update.emitSignal();
}

///////////////////////////////////////////////////////////////
void Model::endUpdate()
{
  bool bTopLevel = (redo_stack.size() == 1);

  if (bTopLevel)
  {
    //the redo/undo I've stored in my undo_redo vector be more sintetic than the new one
    if (bRedoing)
    {
      topRedo() = undo_redo[cursor_undo_redo].first;
      topUndo() = undo_redo[cursor_undo_redo].second;
      cursor_undo_redo += 1;
    }
    else if (bUndoing)
    {
      cursor_undo_redo -= 1;
      topRedo() = undo_redo[cursor_undo_redo].second;
      topUndo() = undo_redo[cursor_undo_redo].first;
    }
    else
    {
      //special case for diff action, need to compute the diff
      if (isDiff(topRedo()) || isDiff(topUndo()))
      {
        StringTree A = diff_begin;
        StringTree B(this->getTypeName());
        this->write(B);

        auto diff = Visus::Diff(
          StringUtils::getNonEmptyLines(A.toXmlString()),
          StringUtils::getNonEmptyLines(B.toXmlString()));

        if (isDiff(topRedo()))
          topRedo().writeText("patch", diff.toString(), /*cdata*/true);

        if (isDiff(topUndo()))
          topUndo().writeText("patch", diff.inverted().toString(), /*cdata*/true);
      }

      //do not touch the undo/redo history if in the middle of an undo/redo
      this->undo_redo.resize(cursor_undo_redo);
      this->cursor_undo_redo += 1;
      this->undo_redo.push_back(std::make_pair(topRedo(), topUndo()));
    }

    this->diff_begin = StringTree();

    this->history.addChild(topRedo());

    //write the action
    if (this->log.is_open())
    {
      this->log << std::endl;
      //this->log << "<!--REDO-->" << std::endl;
      this->log << topRedo().toString() << std::endl;
      //this->log << "<!--UNDO-->" << std::endl;
      //this->log << topUndo().toString() << std::endl;
    }

    //emit signals (before the top() in case someone whants to use topUndo()/topRedo()
    this->modelChanged();
    this->end_update.emitSignal();
  }

  auto redo = this->topRedo(); this->redo_stack.pop();
  auto undo = this->topUndo(); this->undo_stack.pop();

  //collect for transaction
  if (!bTopLevel && isTransaction(topRedo()))
  {
    VisusAssert(redo.name != "begin_update");
    Utils::push_back(topRedo().childs, std::make_shared<StringTree>(redo));
  }

  //collect redo_stack at the begin (since they need to be executed in reverse order)
  if (!bTopLevel && isTransaction(topUndo()))
  {
    VisusAssert(undo.name != "begin_update");
    Utils::push_front(topUndo().childs, std::make_shared<StringTree>(undo));
  }

}

///////////////////////////////////////////////////////////////
bool Model::redo() {
  VisusAssert(VisusHasMessageLock());
  VisusAssert(!isUpdating());
  VisusAssert(!bUndoing && !bRedoing);
  if (!canRedo())  return false;
  int num_actions = history.getNumberOfChilds();
  auto action = undo_redo[cursor_undo_redo].first;
  bRedoing = true;
  execute(action);
  bRedoing = false;
  VisusReleaseAssert(history.getNumberOfChilds()==num_actions+1);
  return true;
}

///////////////////////////////////////////////////////////////
bool Model::undo() {
  VisusAssert(VisusHasMessageLock());
  VisusAssert(!isUpdating());
  VisusAssert(!bUndoing && !bRedoing);
  if (!canUndo()) return false;
  int num_actions = history.getNumberOfChilds();
  auto action = undo_redo[cursor_undo_redo - 1].second;
  bUndoing = true;
  execute(action);
  bUndoing = false;
  VisusReleaseAssert(history.getNumberOfChilds() == num_actions + 1);
  return true;
}

} //namespace Visus

