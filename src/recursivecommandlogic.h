#pragma once

#include <string>
#include <map>
#include <list>
#include <vector>
#include <utility>

#include "path.h"

class FileList;

enum RecursiveCommandType {
  RCL_DELETE,
  RCL_DELETEOWN,
  RCL_TRANSFER
};

enum RecursiveCommandLogicAction {
  RCL_ACTION_LIST,
  RCL_ACTION_CWD,
  RCL_ACTION_DELETE,
  RCL_ACTION_DELDIR,
  RCL_ACTION_NOOP,
};

class RecursiveCommandLogic {
private:
  RecursiveCommandType mode;
  bool active;
  Path basepath;
  Path target;
  std::list<Path> wantedlists;
  std::vector<std::pair<Path, bool> > deletefiles;
  bool listtarget;
  Path listtargetpath;
public:
  RecursiveCommandLogic();
  void initialize(RecursiveCommandType, const Path &);
  bool isActive() const;
  int getAction(const Path &, Path &);
  void addFileList(FileList *);
  void failedCwd();
};
