#ifndef sdmanager_h
#define sdmanager_h

#ifdef SD_SDMMC_PINS
#include <SD_MMC.h>
#define SDMANAGER_BASE fs::SDMMCFS
#else
#include <SD.h>
#define SDMANAGER_BASE fs::SDFS
#endif

class SDManager : public SDMANAGER_BASE {
  public:
    bool ready;
  public:
    SDManager(FSImplPtr impl) : SDMANAGER_BASE(impl) {}
    bool start();
    void stop();
    bool cardPresent();
    void listSD(File &plSDfile, File &plSDindex, const char * dirname, uint8_t levels);
    void indexSDPlaylist();
  private:
    uint32_t _sdFCount;
  private:
    bool _checkNoMedia(const char* path);
    bool _endsWith (const char* base, const char* str);
};

extern SDManager sdman;
#if defined(SD_SPIPINS) || SD_HSPI
extern SPIClass  SDSPI;
#endif
#endif
