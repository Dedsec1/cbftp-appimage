#pragma once

#include <list>
#include <utility>
#include <string>

#include "browsescreensub.h"

#include "../uiwindow.h"
#include "../uifilelist.h"
#include "../menuselectoption.h"
#include "../menuselectoptiontextfield.h"
#include "../../path.h"

class SiteLogic;
class Site;
class FileList;
class Ui;
class BrowseScreenAction;

class BrowseScreenSite : public BrowseScreenSub {
public:
  BrowseScreenSite(Ui *, const std::string &);
  ~BrowseScreenSite();
  BrowseScreenType type() const;
  void redraw(unsigned int, unsigned int, unsigned int);
  void update();
  void command(const std::string &, const std::string &);
  BrowseScreenAction keyPressed(unsigned int);
  std::string getLegendText() const;
  std::string getInfoLabel() const;
  std::string getInfoText() const;
  void setFocus(bool);
  void tick(int);
  std::string siteName() const;
  FileList * fileList() const;
  UIFile * selectedFile() const;
  static void addFileDetails(MenuSelectOption &, unsigned int, unsigned int, const std::string &);
  static void addFileDetails(MenuSelectOption &, unsigned int, unsigned int, const std::string &, const std::string &,
      const std::string &, const std::string &, const std::string &, bool, bool);
private:
  Ui * ui;
  unsigned int row;
  unsigned int col;
  unsigned int coloffset;
  MenuSelectOption table;
  unsigned int currentviewspan;
  bool virgin;
  bool resort;
  mutable int tickcount;
  bool wipe;
  bool wiperecursive;
  bool deleting;
  bool deletingrecursive;
  bool nuking;
  mutable bool changedsort;
  mutable bool cwdfailed;
  mutable bool wipesuccess;
  mutable bool wipefailed;
  mutable bool deletesuccess;
  mutable bool deletefailed;
  mutable bool nukesuccess;
  mutable bool nukefailed;
  bool gotomode;
  bool gotomodefirst;
  int gotomodeticker;
  bool filtermodeinput;
  std::string gotomodestring;
  Path nuketarget;
  Path wipetarget;
  Path wipepath;
  std::string wipefile;
  unsigned int sortmethod;
  Pointer<Site> site;
  UIFileList list;
  Pointer<SiteLogic> sitelogic;
  int requestid;
  Path requestedpath;
  mutable int spinnerpos;
  FileList * filelist;
  bool withinraceskiplistreach;
  Path closestracesectionpath;
  std::string separatortext;
  std::list<std::pair<Path, std::string> > selectionhistory;
  bool focus;
  MenuSelectOptionTextField filterfield;
  void sort();
  void refreshFilelist();
  void disableGotoMode();
};