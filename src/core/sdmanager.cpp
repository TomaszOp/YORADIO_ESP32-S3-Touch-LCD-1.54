//v0.9.686
#include "options.h"
#if SDC_CS!=255
#include <Arduino.h>
#include <SPI.h>
#ifdef SD_SDMMC_PINS
#include <SD_MMC.h>
#else
#include <SD.h>
#include "sd_diskio.h"
#endif
#include "vfs_api.h"
#include <vector>
#include <algorithm>
//#define USE_SD
#include "config.h"
#include "sdmanager.h"
#include "display.h"
#include "player.h"

#if defined(SD_SPIPINS) || SD_HSPI
SPIClass  SDSPI(HSPI);
#define SDREALSPI SDSPI
#else
  #define SDREALSPI SPI
#endif

#ifndef SDSPISPEED
  #define SDSPISPEED 20000000
#endif

SDManager sdman(FSImplPtr(new VFSImpl()));

#ifdef SD_SDMMC_PINS
// Natywny SDMMC (np. Waveshare ESP32-S3-Touch-LCD-1.54): 4 linie danych, a gdy karta nie ruszy, 1 linia
bool SDManager::start(){
  if(_card) return ready = true;
  ready = setPins(SD_SDMMC_PINS) && begin("/sdcard", false, false, SDMMC_FREQ_DEFAULT);
  if(!ready){
    vTaskDelay(50);
    ready = begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT);
  }
  if(!ready) Serial.println("##[ERROR]#\tSD_MMC mount failed");
  return ready;
}
#else
bool SDManager::start(){
  ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(10);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(20);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(50);
  if(!ready) ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  return ready;
}
#endif

void SDManager::stop(){
  end();
  ready = false;
}
#include "diskio_impl.h"
bool SDManager::cardPresent() {

  if(!ready) return false;
  if(sectorSize()<1) {
    return false;
  }
  uint8_t buff[sectorSize()] = { 0 };
  bool bread = readRAW(buff, 1);
  if(sectorSize()>0 && !bread) return false;
  return bread;
}

bool SDManager::_checkNoMedia(const char* path){
  if (path[strlen(path) - 1] == '/')
    snprintf(config.tmpBuf, sizeof(config.tmpBuf), "%s%s", path, ".nomedia");
  else
    snprintf(config.tmpBuf, sizeof(config.tmpBuf), "%s/%s", path, ".nomedia");
  bool nm = exists(config.tmpBuf);
  return nm;
}

bool SDManager::_endsWith (const char* base, const char* str) {
  int slen = strlen(str) - 1;
  const char *p = base + strlen(base) - 1;
  while(p > base && isspace(*p)) p--;
  p -= slen;
  if (p < base) return false;
  return (strncmp(p, str, slen) == 0);
}

// Lista jednego folderu (bez wchodzenia w podfoldery): "..", podfoldery jako "[nazwa]" z "/" na końcu ścieżki,
// potem pliki audio. Obie grupy alfabetycznie. config.sdFirstFile = numer pierwszego pliku na liście.
void SDManager::listSD(File &plSDfile, File &plSDindex, const char* dirname, uint8_t levels) {
    File root = sdman.open(dirname);
    if (!root) {
        Serial.println("##[ERROR]#\tFailed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("##[ERROR]#\tNot a directory");
        root.close();
        return;
    }
    std::vector<String> dirs, files;
    while (true) {
        bool isDir;
        String path = root.getNextFileName(&isDir);
        if (path.isEmpty()) break;
        int slash = path.lastIndexOf('/');
        String name = path.substring(slash + 1);
        if (name.length() == 0 || name[0] == '.') continue;
        if (isDir) {
            if (name == "System Volume Information" || path == "/data") continue; // /data: lista radia na karcie
            if (_checkNoMedia(path.c_str())) continue;
            dirs.push_back(path);
        } else {
            String low = name;
            low.toLowerCase();
            if (low.endsWith(".mp3") || low.endsWith(".m4a") || low.endsWith(".aac") || low.endsWith(".wav") || low.endsWith(".flac")) {
                files.push_back(path);
            }
        }
        if ((dirs.size() + files.size()) % 32 == 0) vTaskDelay(1);
    }
    root.close();
    auto byName = [](const String& x, const String& y) {
        return strcasecmp(x.c_str() + x.lastIndexOf('/') + 1, y.c_str() + y.lastIndexOf('/') + 1) < 0;
    };
    std::sort(dirs.begin(), dirs.end(), byName);
    std::sort(files.begin(), files.end(), byName);

    uint16_t n = 0;
    auto add = [&](const char* name, const char* url) {
        uint32_t pos = plSDfile.position();
        plSDfile.printf("%s\t%s\t0\n", name, url);
        plSDindex.write((uint8_t*)&pos, 4);
        n++;
    };
    if (strcmp(dirname, config.sdRootDir()) != 0) {
        String parent = dirname;
        parent = parent.substring(0, parent.lastIndexOf('/'));
        if (parent.length() == 0) parent = "/";
        if (!parent.endsWith("/")) parent += "/";
        add("..", parent.c_str());
    }
    for (auto& d : dirs) {
        String name = "[" + d.substring(d.lastIndexOf('/') + 1) + "]";
        add(name.c_str(), (d + "/").c_str());
    }
    config.sdFirstFile = n + 1;
    for (auto& f : files) {
        add(f.c_str() + f.lastIndexOf('/') + 1, f.c_str());
        _sdFCount++;
        if (display.mode() == SDCHANGE) display.putRequest(SDFILEINDEX, _sdFCount);
    }
    Serial.printf("##[SD]#\t%s: %u folders, %u files\n", dirname, (unsigned)dirs.size(), (unsigned)files.size());
}

void SDManager::indexSDPlaylist() {
  _sdFCount = 0;
  if(exists(PLAYLIST_SD_PATH)) remove(PLAYLIST_SD_PATH);
  if(exists(INDEX_SD_PATH)) remove(INDEX_SD_PATH);
  File playlist = open(PLAYLIST_SD_PATH, "w", true);
  if (!playlist) {
    return;
  }
  File index = open(INDEX_SD_PATH, "w", true);
  // Czytany jest tylko bieżący folder (config.sdDir), więc nawet na dużej karcie trwa to chwilę.
  if (config.sdDir[0] == '\0' || !exists(config.sdDir)) strlcpy(config.sdDir, config.sdRootDir(), sizeof(config.sdDir));
  listSD(playlist, index, config.sdDir, 0);
  index.flush();
  index.close();
  playlist.flush();
  playlist.close();
}
#endif

