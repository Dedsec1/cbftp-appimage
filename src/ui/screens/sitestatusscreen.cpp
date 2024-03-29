#include "sitestatusscreen.h"

#include "../../sitemanager.h"
#include "../../globalcontext.h"
#include "../../site.h"
#include "../../sitelogic.h"
#include "../../sitelogicmanager.h"
#include "../../ftpconn.h"
#include "../../connstatetracker.h"
#include "../../util.h"
#include "../../hourlyalltracking.h"

#include "../ui.h"

SiteStatusScreen::SiteStatusScreen(Ui * ui) {
  this->ui = ui;
}

void SiteStatusScreen::initialize(unsigned int row, unsigned int col, std::string sitename) {
  this->sitename = sitename;
  site = global->getSiteManager()->getSite(sitename);
  autoupdate = true;
  st = global->getSiteLogicManager()->getSiteLogic(site->getName());
  init(row, col);
}

void SiteStatusScreen::redraw() {
  ui->erase();
  ui->hideCursor();
  std::string loginslots = "Login slots:    " + std::to_string(st->getCurrLogins());
  if (!site->unlimitedLogins()) {
    loginslots += "/" + std::to_string(site->getMaxLogins());
  }
  std::string upslots = "Upload slots:   " + std::to_string(st->getCurrUp());
  if (!site->unlimitedUp()) {
    upslots += "/" + std::to_string(site->getMaxUp());
  }
  std::string downslots = "Download slots: " + std::to_string(st->getCurrDown());
  if (!site->unlimitedDown()) {
    downslots += "/" + std::to_string(site->getMaxDown());
  }
  ui->printStr(1, 1, loginslots);
  ui->printStr(2, 1, upslots);
  ui->printStr(3, 1, downslots);
  ui->printStr(5, 1, "Login threads:");
  int i = 8;
  for(unsigned int j = 0; j < st->getConns()->size(); j++) {
    std::string status = st->getStatus(j);
    std::string llstate = st->getConnStateTracker(j)->isListLocked()
        ? "Y" : "N";
    std::string hlstate = st->getConnStateTracker(j)->isHardLocked()
        ? "Y" : "N";
    std::string tstate = st->getConnStateTracker(j)->hasFileTransfer()
        ? "Y" : "N";
    ui->printStr(i++, 1, "#" + std::to_string((int)j) + " - LL:" + llstate +
        " - HL:" + hlstate + " - T:" + tstate + " - " + status);
  }
  ++i;
  unsigned long long int sizeupday = site->getSizeUp().getLast24Hours();
  unsigned int filesupday = site->getFilesUp().getLast24Hours();
  unsigned long long int sizeupall = site->getSizeUp().getAll();
  unsigned int filesupall = site->getFilesUp().getAll();
  unsigned long long int sizedownday = site->getSizeDown().getLast24Hours();
  unsigned int filesdownday = site->getFilesDown().getLast24Hours();
  unsigned long long int sizedownall = site->getSizeDown().getAll();
  unsigned int filesdownall = site->getFilesDown().getAll();
  ui->printStr(i++, 1, "Traffic measurements");
  ui->printStr(i++, 1, "Upload   last 24 hours: " + util::parseSize(sizeupday) + ", " +
                 std::to_string(filesupday) + " files - All time: " + util::parseSize(sizeupall) + ", " +
                 std::to_string(filesupall) + " files");
  ui->printStr(i++, 1, "Download last 24 hours: " + util::parseSize(sizedownday) + ", " +
                 std::to_string(filesdownday) + " files - All time: " + util::parseSize(sizedownall) + ", " +
                 std::to_string(filesdownall) + " files");
}

void SiteStatusScreen::update() {
  redraw();
}

bool SiteStatusScreen::keyPressed(unsigned int ch) {
  switch(ch) {
    case KEY_RIGHT:
      ui->goRawData(site->getName());
      return true;
    case 'E':
      ui->goEditSite(site->getName());
      return true;
    case 27: // esc
    case ' ':
    case 10:
      ui->returnToLast();
      return true;
    case 'b':
      ui->goBrowse(site->getName());
      return true;
    case 't':
      ui->goTransfersFilterSite(site->getName());
      return true;
    case 'w':
      ui->goRawCommand(site->getName());
      return true;
  }
  return false;
}

std::string SiteStatusScreen::getLegendText() const {
  return "[Right] Raw data screens - [Enter] Return - ra[w] command - [E]dit site - [t]ransfers";
}

std::string SiteStatusScreen::getInfoLabel() const {
  return "DETAILED STATUS: " + site->getName();
}
