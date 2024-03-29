#include "browsescreenselector.h"

#include <memory>

#include "../../sitemanager.h"
#include "../../site.h"
#include "../../globalcontext.h"
#include "../../localstorage.h"

#include "../termint.h"
#include "../ui.h"
#include "../menuselectoptiontextbutton.h"
#include "../resizableelement.h"
#include "../misc.h"

#include "browsescreenaction.h"

BrowseScreenSelector::BrowseScreenSelector(Ui * ui) :
  ui(ui),
  focus(true),
  pointer(0),
  currentviewspan(0),
  gotomode(false) {
  SiteManager * sm = global->getSiteManager();
  std::vector<std::shared_ptr<Site> >::const_iterator it;
  entries.push_back(std::pair<std::string, std::string>(BROWSESCREENSELECTOR_HOME,
                                                        global->getLocalStorage()->getDownloadPath().toString()));
  entries.push_back(std::pair<std::string, std::string>("", ""));
  for (it = sm->begin(); it != sm->end(); it++) {
    entries.push_back(std::pair<std::string, std::string>((*it)->getName(), (*it)->getName()));
  }
}

BrowseScreenSelector::~BrowseScreenSelector() {

}

BrowseScreenType BrowseScreenSelector::type() const {
  return BrowseScreenType::SELECTOR;
}

void BrowseScreenSelector::redraw(unsigned int row, unsigned int col, unsigned int coloffset) {
  this->row = row;
  this->col = col;
  this->coloffset = coloffset;
  table.clear();
  adaptViewSpan(currentviewspan, row, pointer, entries.size());

  int y = 0;
  for (unsigned int i = currentviewspan; i < currentviewspan + row && i < entries.size(); i++) {
    std::shared_ptr<MenuSelectOptionTextButton> msotb =
        table.addTextButtonNoContent(y++, coloffset + 1, entries[i].first, entries[i].second);
    if (entries[i].first == "") {
      msotb->setSelectable(false);
    }
    if (i == pointer) {
      table.setPointer(msotb);
    }
  }
  table.checkPointer();
  for (unsigned int i = 0; i < table.size(); i++) {
    std::shared_ptr<MenuSelectOptionTextButton> msotb = std::static_pointer_cast<MenuSelectOptionTextButton>(table.getElement(i));
    bool highlight = false;
    if (table.getSelectionPointer() == i) {
      highlight = true;
    }
    ui->printStr(msotb->getRow(), msotb->getCol(), msotb->getLabelText(), highlight && focus);
  }
  printSlider(ui, row, coloffset + col - 1, entries.size(), currentviewspan);
}

void BrowseScreenSelector::update() {
  if (table.size()) {
    if (pointer < currentviewspan || pointer >= currentviewspan + row) {
      ui->redraw();
      return;
    }
    std::shared_ptr<ResizableElement> re = std::static_pointer_cast<ResizableElement>(table.getElement(table.getLastSelectionPointer()));
    ui->printStr(re->getRow(), re->getCol(), re->getLabelText());
    re = std::static_pointer_cast<ResizableElement>(table.getElement(table.getSelectionPointer()));
    ui->printStr(re->getRow(), re->getCol(), re->getLabelText(), focus);
  }
}

BrowseScreenAction BrowseScreenSelector::keyPressed(unsigned int ch) {
  unsigned int pagerows = (unsigned int) row * 0.6;
  if (gotomode) {
    if (ch >= 32 && ch <= 126) {
      for (unsigned int i = 0; i < entries.size(); i++) {
        const std::string & label = entries[i].second;
        if (!label.empty() && toupper(ch) == toupper(label[0])) {
          pointer = i;
          ui->redraw();
          break;
        }
      }
    }
    gotomode = false;
    ui->update();
    ui->setLegend();
    return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
  }
  switch (ch) {
    case 27: // esc
      ui->returnToLast();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    case 'c':
    case KEY_LEFT:
    case KEY_BACKSPACE:
      return BrowseScreenAction(BROWSESCREENACTION_CLOSE);
    case KEY_UP:
      if (table.goUp()) {
        pointer = table.getElement(table.getSelectionPointer())->getRow() + currentviewspan;
        ui->update();
      }
      else if (pointer > 0) {
        pointer--;
        ui->redraw();
      }
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    case KEY_DOWN:
      if (table.goDown()) {
        pointer = table.getElement(table.getSelectionPointer())->getRow() + currentviewspan;
        ui->update();
      }
      else if (pointer < entries.size()) {
        pointer++;
        ui->redraw();
      }
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    case KEY_HOME:
      pointer = 0;
      ui->redraw();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    case KEY_END:
      pointer = entries.size() - 1;
      ui->redraw();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    case KEY_NPAGE:
      if (pagerows < entries.size() && pointer < entries.size() - 1 - pagerows) {
        pointer += pagerows;
      }
      else {
        pointer = entries.size() - 1;
      }
      ui->redraw();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    case KEY_PPAGE:
      if (pointer > pagerows) {
        pointer -= pagerows;
      }
      else {
        pointer = 0;
      }
      ui->redraw();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    case KEY_RIGHT:
    case 10:
    case 'b':
    {
      std::shared_ptr<MenuSelectOptionTextButton> msotb = std::static_pointer_cast<MenuSelectOptionTextButton>(table.getElement(table.getSelectionPointer()));
      if (msotb->getIdentifier() == BROWSESCREENSELECTOR_HOME) {
        return BrowseScreenAction(BROWSESCREENACTION_HOME);
      }
      else {
        return BrowseScreenAction(BROWSESCREENACTION_SITE, msotb->getIdentifier());
      }
      break;
    }
    case 'q':
      gotomode = true;
      ui->update();
      ui->setLegend();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
  }
  return BrowseScreenAction();
}

std::string BrowseScreenSelector::getLegendText() const {
  if (gotomode) {
    return "[Any] Go to matching first letter in site list - [Esc] Cancel";
  }
  return "[Up/Down] Navigate - [Enter/Right/b] Browse - [q]uick jump - [Esc] Cancel - [c]lose";
}

std::string BrowseScreenSelector::getInfoLabel() const {
  return "SELECT SITE";
}

std::string BrowseScreenSelector::getInfoText() const {
  return "select site or local";
}

void BrowseScreenSelector::setFocus(bool focus) {
  this->focus = focus;
}
