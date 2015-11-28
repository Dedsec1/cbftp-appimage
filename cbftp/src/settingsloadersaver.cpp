#include "settingsloadersaver.h"

#include "globalcontext.h"
#include "datafilehandler.h"
#include "eventlog.h"
#include "iomanager.h"
#include "remotecommandhandler.h"
#include "externalfileviewing.h"
#include "skiplist.h"
#include "skiplistitem.h"
#include "proxymanager.h"
#include "proxy.h"
#include "localstorage.h"
#include "uibase.h"
#include "sitemanager.h"
#include "site.h"
#include "util.h"
#include "tickpoke.h"

#define AUTO_SAVE_INTERVAL 600000 // 10 minutes

extern GlobalContext * global;

SettingsLoaderSaver::SettingsLoaderSaver() :
  dfh(makePointer<DataFileHandler>()){

}

bool SettingsLoaderSaver::enterKey(const std::string & key) {
  if (dfh->isInitialized()) {
    return false;
  }
  if (dataExists()) {
    bool result = dfh->readEncrypted(key);
    if (result) {
      global->getEventLog()->log("DataLoaderSaver", "Data decryption successful.");
      loadSettings();
      startAutoSaver();
      return true;
    }
    global->getEventLog()->log("DataLoaderSaver", "Data decryption failed.");
    return false;
  }
  else {
    dfh->newDataFile(key);
    startAutoSaver();
    return true;
  }
}

bool SettingsLoaderSaver::dataExists() const {
  return dfh->fileExists();
}

bool SettingsLoaderSaver::changeKey(const std::string & key, const std::string & newkey) {
  return dfh->changeKey(key, newkey);
}

void SettingsLoaderSaver::loadSettings() {
  std::vector<std::string> lines;
  dfh->getDataFor("IOManager", &lines);
  std::vector<std::string>::iterator it;
  std::string line;
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("defaultinterface")) {
      global->getIOManager()->setDefaultInterface(value);
    }
  }

  dfh->getDataFor("RemoteCommandHandler", &lines);
  bool enable = false;
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("enabled")) {
      if (!value.compare("true")) {
        enable = true;
      }
    }
    else if (!setting.compare("port")) {
      global->getRemoteCommandHandler()->setPort(util::str2Int(value));
    }
    else if (!setting.compare("password")) {
      global->getRemoteCommandHandler()->setPassword(value);
    }
  }
  if (enable) {
    global->getRemoteCommandHandler()->setEnabled(true);
  }

  dfh->getDataFor("ExternalFileViewing", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("video")) {
      global->getExternalFileViewing()->setVideoViewer(value);
    }
    else if (!setting.compare("audio")) {
      global->getExternalFileViewing()->setAudioViewer(value);
    }
    else if (!setting.compare("image")) {
      global->getExternalFileViewing()->setImageViewer(value);
    }
    else if (!setting.compare("pdf")) {
      global->getExternalFileViewing()->setPDFViewer(value);
    }
  }

  global->getSkipList()->clearEntries();
  dfh->getDataFor("SkipList", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("entry")) {
      size_t split = value.find('$');
      if (split != std::string::npos) {
        std::string pattern = value.substr(0, split);
        value = value.substr(split + 1);
        split = value.find('$');
        bool file = value.substr(0, split) == "true" ? true : false;
        value = value.substr(split + 1);
        split = value.find('$');
        bool dir = value.substr(0, split) == "true" ? true : false;
        value = value.substr(split + 1);
        split = value.find('$');
        bool allowed;
        int scope;
        if (split != std::string::npos) {
          scope = util::str2Int(value.substr(0, split));
          allowed = value.substr(split + 1) == "true" ? true : false;
        }
        else {
          scope = SCOPE_IN_RACE;
          allowed = value == "true" ? true : false;
        }
        global->getSkipList()->addEntry(pattern, file, dir, scope, allowed);
      }
      else { // backwards compatibility
        global->getSkipList()->addEntry(value, true, true, SCOPE_IN_RACE, false);
      }
    }
    if (!setting.compare("defaultallow")) {
      global->getSkipList()->setDefaultAllow(value.compare("true") == 0 ? true : false);
    }
  }

  dfh->getDataFor("ProxyManager", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok1 = line.find('$');
    size_t tok2 = line.find('=', tok1);
    std::string name = line.substr(0, tok1);
    std::string setting = line.substr(tok1 + 1, (tok2 - tok1 - 1));
    std::string value = line.substr(tok2 + 1);
    Proxy * proxy = global->getProxyManager()->getProxy(name);
    if (proxy == NULL) {
      proxy = new Proxy(name);
      global->getProxyManager()->addProxy(proxy);
    }
    if (!setting.compare("addr")) {
      proxy->setAddr(value);
    }
    else if (!setting.compare("port")) {
      proxy->setPort(value);
    }
    else if (!setting.compare("authmethod")) {
      proxy->setAuthMethod(util::str2Int(value));
    }
    else if (!setting.compare("user")) {
      proxy->setUser(value);
    }
    else if (!setting.compare("pass")) {
      proxy->setPass(value);
    }
  }
  lines.clear();
  dfh->getDataFor("ProxyManagerDefaults", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("useproxy")) {
      global->getProxyManager()->setDefaultProxy(value);
    }
  }
  global->getProxyManager()->sortProxys();

  dfh->getDataFor("LocalStorage", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("temppath")) {
      global->getLocalStorage()->setTempPath(value);
    }
    if (!setting.compare("downloadpath")) {
      global->getLocalStorage()->setDownloadPath(value);
    }
  }

  dfh->getDataFor("UI", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("legend")) {
      if (!value.compare("true")) {
        global->getUIBase()->showLegend(true);
      }
      else {
        global->getUIBase()->showLegend(false);
      }
    }
  }

  dfh->getDataFor("SiteManager", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok1 = line.find('$');
    size_t tok2 = line.find('=', tok1);
    std::string name = line.substr(0, tok1);
    std::string setting = line.substr(tok1 + 1, (tok2 - tok1 - 1));
    std::string value = line.substr(tok2 + 1);
    Site * site = global->getSiteManager()->getSite(name);
    if (site == NULL) {
      site = new Site(name);
      global->getSiteManager()->addSiteLoad(site);
    }
    if (!setting.compare("addr")) {
      site->setAddress(value);
    }
    else if (!setting.compare("port")) {
      site->setPort(value);
    }
    else if (!setting.compare("user")) {
      site->setUser(value);
    }
    else if (!setting.compare("pass")) {
      site->setPass(value);
    }
    else if (!setting.compare("basepath")) {
      site->setBasePath(value);
    }
    else if (!setting.compare("idletime")) {
      site->setMaxIdleTime(util::str2Int(value));
    }
    else if (!setting.compare("pret")) {
      if (!value.compare("true")) site->setPRET(true);
    }
    else if (!setting.compare("binary")) {
      if (!value.compare("true")) site->setForceBinaryMode(true);
    }
    else if (!setting.compare("sslconn")) {
      if (!value.compare("false")) site->setSSL(false);
    }
    else if (!setting.compare("sslfxpforced")) {
      if (!value.compare("true")) site->setSSLTransferPolicy(SITE_SSL_ALWAYS_ON);
    }
    else if (!setting.compare("ssltransfer")) {
      site->setSSLTransferPolicy(util::str2Int(value));
    }
    else if (!setting.compare("cpsv")) {
      if (!value.compare("false")) site->setSupportsCPSV(false);
    }
    else if (!setting.compare("listcommand")) {
      site->setListCommand(util::str2Int(value));
    }
    else if (!setting.compare("allowupload")) {
      if (!value.compare("false")) site->setAllowUpload(false);
    }
    else if (!setting.compare("allowdownload")) {
      if (!value.compare("false")) site->setAllowDownload(false);
    }
    else if (!setting.compare("brokenpasv")) {
      if (!value.compare("true")) site->setBrokenPASV(true);
    }
    else if (!setting.compare("logins")) {
      site->setMaxLogins(util::str2Int(value));
    }
    else if (!setting.compare("maxdn")) {
      site->setMaxDn(util::str2Int(value));
    }
    else if (!setting.compare("maxup")) {
      site->setMaxUp(util::str2Int(value));
    }
    else if (!setting.compare("section")) {
      size_t split = value.find('$');
      std::string sectionname = value.substr(0, split);
      std::string sectionpath = value.substr(split + 1);
      site->addSection(sectionname, sectionpath);
    }
    else if (!setting.compare("avgspeed")) {
      size_t split = value.find('$');
      std::string sitename = value.substr(0, split);
      int avgspeed = util::str2Int(value.substr(split + 1));
      site->setAverageSpeed(sitename, avgspeed);
    }
    else if (!setting.compare("affil")) {
      site->addAffil(value);
    }
    else if (!setting.compare("bannedgroup")) {
      site->addBannedGroup(value);
    }
    else if (!setting.compare("proxytype")) {
      site->setProxyType(util::str2Int(value));
    }
    else if (!setting.compare("proxyname")) {
      site->setProxy(value);
    }
    else if (!setting.compare("rank")) {
      site->setRank(util::str2Int(value));
    }
    else if (!setting.compare("ranktolerance")) {
      site->setRankTolerance(util::str2Int(value));
    }
  }
  dfh->getDataFor("SiteManagerDefaults", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("username")) {
      global->getSiteManager()->setDefaultUserName(value);
    }
    else if (!setting.compare("password")) {
      global->getSiteManager()->setDefaultPassword(value);
    }
    else if (!setting.compare("maxlogins")) {
      global->getSiteManager()->setDefaultMaxLogins(util::str2Int(value));
    }
    else if (!setting.compare("maxup")) {
      global->getSiteManager()->setDefaultMaxUp(util::str2Int(value));
    }
    else if (!setting.compare("maxdown")) {
      global->getSiteManager()->setDefaultMaxDown(util::str2Int(value));
    }
    else if (!setting.compare("sslconn")) {
      if (!value.compare("false")) {
        global->getSiteManager()->setDefaultSSL(false);
      }
    }
    else if (!setting.compare("sslfxpforced")) {
      if (!value.compare("true")) {
        global->getSiteManager()->setDefaultSSLTransferPolicy(SITE_SSL_ALWAYS_ON);
      }
    }
    else if (!setting.compare("ssltransfer")) {
      global->getSiteManager()->setDefaultSSLTransferPolicy(util::str2Int(value));
    }
    else if (!setting.compare("maxidletime")) {
      global->getSiteManager()->setDefaultMaxIdleTime(util::str2Int(value));
    }
    else if (!setting.compare("rank")) {
      global->getSiteManager()->setGlobalRank(util::str2Int(value));
    }
    else if (!setting.compare("ranktolerance")) {
      global->getSiteManager()->setGlobalRankTolerance(util::str2Int(value));
    }
  }
  lines.clear();
  dfh->getDataFor("SiteManagerRules", &lines);
  for (it = lines.begin(); it != lines.end(); it++) {
    line = *it;
    if (line.length() == 0 ||line[0] == '#') continue;
    size_t tok = line.find('=');
    std::string setting = line.substr(0, tok);
    std::string value = line.substr(tok + 1);
    if (!setting.compare("blockedpair")) {
      size_t split = value.find('$');
      std::string site1 = value.substr(0, split);
      std::string site2 = value.substr(split + 1);
      global->getSiteManager()->addBlockedPair(site1, site2);
    }
  }
  global->getSiteManager()->sortSites();
}

void SettingsLoaderSaver::saveSettings() {
  if (!dfh->isInitialized()) {
    return;
  }

  dfh->clearOutputData();

  if (global->getIOManager()->hasDefaultInterface()) {
    dfh->addOutputLine("IOManager", "defaultinterface=" + global->getIOManager()->getDefaultInterface());
  }

  if (global->getRemoteCommandHandler()->isEnabled()) dfh->addOutputLine("RemoteCommandHandler", "enabled=true");
  dfh->addOutputLine("RemoteCommandHandler", "port=" + util::int2Str(global->getRemoteCommandHandler()->getUDPPort()));
  dfh->addOutputLine("RemoteCommandHandler", "password=" + global->getRemoteCommandHandler()->getPassword());

  {
    std::string filetag = "ExternalFileViewing";
    dfh->addOutputLine(filetag, "video=" + global->getExternalFileViewing()->getVideoViewer());
    dfh->addOutputLine(filetag, "audio=" + global->getExternalFileViewing()->getAudioViewer());
    dfh->addOutputLine(filetag, "image=" + global->getExternalFileViewing()->getImageViewer());
    dfh->addOutputLine(filetag, "pdf=" + global->getExternalFileViewing()->getPDFViewer());
  }

  {
    std::list<SkiplistItem>::const_iterator it;
    for (it = global->getSkipList()->entriesBegin(); it != global->getSkipList()->entriesEnd(); it++) {
      std::string entryline = it->matchPattern() + "$" +
          (it->matchFile() ? "true" : "false") + "$" +
          (it->matchDir() ? "true" : "false") + "$" +
          util::int2Str(it->matchScope()) + "$" +
          (it->isAllowed() ? "true" : "false");
      dfh->addOutputLine("SkipList", "entry=" + entryline);
    }
    std::string defaultallowstr = global->getSkipList()->defaultAllow() ? "true" : "false";
    dfh->addOutputLine("SkipList", "defaultallow=" + defaultallowstr);
  }

  {
    std::vector<Proxy *>::const_iterator it;
    std::string filetag = "ProxyManager";
    std::string defaultstag = "ProxyManagerDefaults";
    for (it = global->getProxyManager()->begin(); it != global->getProxyManager()->end(); it++) {
      Proxy * proxy = *it;
      std::string name = proxy->getName();
      dfh->addOutputLine(filetag, name + "$addr=" + proxy->getAddr());
      dfh->addOutputLine(filetag, name + "$port=" + proxy->getPort());
      dfh->addOutputLine(filetag, name + "$user=" + proxy->getUser());
      dfh->addOutputLine(filetag, name + "$pass=" + proxy->getPass());
      dfh->addOutputLine(filetag, name + "$authmethod=" + util::int2Str(proxy->getAuthMethod()));
    }
    if (global->getProxyManager()->getDefaultProxy() != NULL) {
      dfh->addOutputLine(defaultstag, "useproxy=" + global->getProxyManager()->getDefaultProxy()->getName());
    }
  }

  {
    std::string filetag = "LocalStorage";
    dfh->addOutputLine(filetag, "temppath=" + global->getLocalStorage()->getTempPath());
    dfh->addOutputLine(filetag, "downloadpath=" + global->getLocalStorage()->getDownloadPath());
  }

  if (global->getUIBase()->legendEnabled()) {
    dfh->addOutputLine("UI", "legend=true");
  }
  else {
    dfh->addOutputLine("UI", "legend=false");
  }

  {
    std::vector<Site *>::const_iterator it;
    std::string filetag = "SiteManager";
    std::string defaultstag = "SiteManagerDefaults";
    std::string rulestag = "SiteManagerRules";
    for (it = global->getSiteManager()->begin(); it != global->getSiteManager()->end(); it++) {
      Site * site = *it;
      std::string name = site->getName();
      dfh->addOutputLine(filetag, name + "$addr=" + site->getAddress());
      dfh->addOutputLine(filetag, name + "$port=" + site->getPort());
      dfh->addOutputLine(filetag, name + "$user=" + site->getUser());
      dfh->addOutputLine(filetag, name + "$pass=" + site->getPass());
      std::string basepath = site->getBasePath();
      if (basepath != "" && basepath != "/") {
        dfh->addOutputLine(filetag, name + "$basepath=" + basepath);
      }
      dfh->addOutputLine(filetag, name + "$logins=" + util::int2Str(site->getInternMaxLogins()));
      dfh->addOutputLine(filetag, name + "$maxup=" + util::int2Str(site->getInternMaxUp()));
      dfh->addOutputLine(filetag, name + "$maxdn=" + util::int2Str(site->getInternMaxDown()));
      dfh->addOutputLine(filetag, name + "$idletime=" + util::int2Str(site->getMaxIdleTime()));
      dfh->addOutputLine(filetag, name + "$ssltransfer=" + util::int2Str(site->getSSLTransferPolicy()));
      if (!site->supportsCPSV()) dfh->addOutputLine(filetag, name + "$cpsv=false");
      dfh->addOutputLine(filetag, name + "$listcommand=" + util::int2Str(site->getListCommand()));
      if (site->needsPRET()) dfh->addOutputLine(filetag, name + "$pret=true");
      if (site->forceBinaryMode()) dfh->addOutputLine(filetag, name + "$binary=true");
      if (!site->SSL()) dfh->addOutputLine(filetag, name + "$sslconn=false");
      if (!site->getAllowUpload()) dfh->addOutputLine(filetag, name + "$allowupload=false");
      if (!site->getAllowDownload()) dfh->addOutputLine(filetag, name + "$allowdownload=false");
      if (site->hasBrokenPASV()) dfh->addOutputLine(filetag, name + "$brokenpasv=true");
      dfh->addOutputLine(filetag, name + "$rank=" + util::int2Str(site->getRank()));
      dfh->addOutputLine(filetag, name + "$ranktolerance=" + util::int2Str(site->getRankTolerance()));
      int proxytype = site->getProxyType();
      dfh->addOutputLine(filetag, name + "$proxytype=" + util::int2Str(proxytype));
      if (proxytype == SITE_PROXY_USE) {
        dfh->addOutputLine(filetag, name + "$proxyname=" + site->getProxy());
      }
      std::map<std::string, std::string>::const_iterator sit;
      for (sit = site->sectionsBegin(); sit != site->sectionsEnd(); sit++) {
        dfh->addOutputLine(filetag, name + "$section=" + sit->first + "$" + sit->second);
      }
      std::map<std::string, int>::const_iterator sit2;
      for (sit2 = site->avgspeedBegin(); sit2 != site->avgspeedEnd(); sit2++) {
        dfh->addOutputLine(filetag, name + "$avgspeed=" + sit2->first + "$" + util::int2Str(sit2->second));
      }
      std::map<std::string, bool>::const_iterator sit3;
      for (sit3 = site->affilsBegin(); sit3 != site->affilsEnd(); sit3++) {
        dfh->addOutputLine(filetag, name + "$affil=" + sit3->first);
      }
      for (sit3 = site->bannedGroupsBegin(); sit3 != site->bannedGroupsEnd(); sit3++) {
        dfh->addOutputLine(filetag, name + "$bannedgroup=" + sit3->first);
      }
    }
    dfh->addOutputLine(defaultstag, "username=" + global->getSiteManager()->getDefaultUserName());
    dfh->addOutputLine(defaultstag, "password=" + global->getSiteManager()->getDefaultPassword());
    dfh->addOutputLine(defaultstag, "maxlogins=" + util::int2Str(global->getSiteManager()->getDefaultMaxLogins()));
    dfh->addOutputLine(defaultstag, "maxup=" + util::int2Str(global->getSiteManager()->getDefaultMaxUp()));
    dfh->addOutputLine(defaultstag, "maxdown=" + util::int2Str(global->getSiteManager()->getDefaultMaxDown()));
    dfh->addOutputLine(defaultstag, "maxidletime=" + util::int2Str(global->getSiteManager()->getDefaultMaxIdleTime()));
    dfh->addOutputLine(defaultstag, "ssltransfer=" + util::int2Str(global->getSiteManager()->getDefaultSSLTransferPolicy()));
    dfh->addOutputLine(defaultstag, "rank=" + util::int2Str(global->getSiteManager()->getGlobalRank()));
    dfh->addOutputLine(defaultstag, "ranktolerance=" + util::int2Str(global->getSiteManager()->getGlobalRankTolerance()));
    if (!global->getSiteManager()->getDefaultSSL()) dfh->addOutputLine(defaultstag, "sslconn=false");
    std::map<Site *, std::map<Site *, bool> >::const_iterator it2;
    std::map<Site *, bool>::const_iterator it3;
    for (it2 = global->getSiteManager()->blockedPairsBegin(); it2 != global->getSiteManager()->blockedPairsEnd(); it2++) {
      for (it3 = it2->second.begin(); it3 != it2->second.end(); it3++) {
        dfh->addOutputLine(rulestag, "blockedpair=" + it2->first->getName() + "$" + it3->first->getName());
      }
    }
  }
  dfh->writeFile();
}

void SettingsLoaderSaver::tick(int) {
  saveSettings();
}

void SettingsLoaderSaver::startAutoSaver() {
  global->getTickPoke()->startPoke(this, "SettingsLoaderSaver", AUTO_SAVE_INTERVAL, 0);
}