# yoRadio na Waveshare ESP32-S3-Touch-LCD-1.54

Ten katalog to fork [VaraiTamas/yoRadio](https://github.com/VaraiTamas/yoRadio) (wersja 0.8.8)
przystosowany do płytki **Waveshare ESP32-S3-Touch-LCD-1.54** (SKU 33868).
Piny i sterowniki płytki pochodzą z repozytorium producenta:
[waveshareteam/ESP32-S3-Touch-LCD-1.54](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54).

Kopia projektu sprzed zmian: `../YORADIO_1.54_kopia_20261119.tar.gz`.

## Sprzęt

| Element | Układ | Piny |
|---|---|---|
| Wyświetlacz 1,54" 240x240 | ST7789 (SPI) | SCK 38, MOSI 39, DC 45, CS 21, RST 40, podświetlenie 46 |
| Dotyk | CST816 (I2C 0x15) | SDA 42, SCL 41, INT 48, RST 47 |
| Kodek audio | ES8311 (I2C 0x18) | MCLK 8, BCLK 9, LRC 10, DOUT 12 |
| Wzmacniacz | PA_CTRL | GPIO 7 |
| Podtrzymanie zasilania z akumulatora | | GPIO 2 (HIGH) |
| Przyciski na obudowie | | lewy GPIO 0 (BOOT), środkowy GPIO 5 (PWR), prawy GPIO 4 |

Moduł: ESP32-S3, 16 MB flash, 8 MB PSRAM (OPI). Pasuje środowisko `esp32-s3-devkitc1-n16r8`
z `platformio.ini`.

Na płytce są też czujnik QMI8658, karta SD w trybie SDMMC (16/15/17/18/13/14) oraz pomiar baterii
(ADC GPIO 1, ładowanie GPIO 3). yoRadio ich nie używa.

## Co zmieniono

Wszystkie zmiany związane z płytką są włączane jednym przełącznikiem w `myoptions.h`:

```c
#define BOARD_WS_S3_TOUCH_LCD_154
```

### `myoptions.h`
- Ustawiono płytkę, wyświetlacz `DSP_ST7789_240` (zamiast OLED `DSP_SSD1322`) oraz piny ekranu, dotyku i I2S
  z tabeli powyżej.
- Dodano `I2S_MCLK 8`, bo kodek ES8311 potrzebuje zegara MCLK z ESP32.
- `MUTE_PIN 7` / `MUTE_VAL LOW` włącza wzmacniacz podczas odtwarzania i wyłącza go po zatrzymaniu.
- Przyciski ułożone zgodnie z obudową: `BTN_LEFT 0`, `BTN_CENTER 5`, `BTN_RIGHT 4`.
- Wyłączono oba enkodery, bo ich piny kolidowały z płytką.
- Zakomentowano `ENCODERS_INDEPENDENT`. Ta opcja blokowała przełączanie listy stacji przytrzymaniem przycisku.
- Język interfejsu, imieniny i zapowiedzi czasu zmieniono z HU na PL.
- Włączono kartę microSD: `SDC_CS 14` i `SD_SDMMC_PINS 16, 15, 17, 18, 13, 14` (CLK, CMD, D0, D1, D2, D3, według
  przykładu `07_sd_card_test` Waveshare). Slot jest podłączony pod natywny interfejs SDMMC, więc karta działa przez
  bibliotekę SD_MMC w trybie 4-bitowym. Jeśli w nim nie ruszy, program próbuje trybu 1-bitowego. `SDC_CS` służy tu tylko do
  włączenia obsługi karty. Pierwsza wersja używała trybu SPI (`SD_SPIPINS`), ale karta inicjowała się z błędami
  (`APP_OP_COND failed`, `f_mount failed`), a radio wieszało się po przełączeniu na kartę. Ponieważ radio pamięta
  wybrany tryb, wieszało się potem przy każdym starcie, dopóki nie wyjęto karty.
- `src/core/sdmanager.h`, `src/core/sdmanager.cpp`: gdy zdefiniowane jest `SD_SDMMC_PINS`, `SDManager` dziedziczy po
  `fs::SDMMCFS` (SD_MMC) zamiast `fs::SDFS` (SD przez SPI), a `start()` ustawia piny i montuje kartę przez SD_MMC
  (20 MHz). Bez tej opcji działa po staremu przez SPI.

### Inicjalizacja płytki: `src/boards/ws_s3_lcd154/` (nowy katalog)
- `ws_s3_lcd154.cpp` implementuje `yoradio_on_setup()`, wywoływane na początku `setup()`. Funkcja:
  - ustawia GPIO 2 w stan wysoki (podtrzymanie zasilania z akumulatora; przy zasilaniu z USB nie ma to wpływu),
  - uruchamia I2C na pinach 42/41,
  - uruchamia SPI ekranu na pinach 38/39, zanim zrobi to biblioteka Adafruit,
  - inicjalizuje kodek ES8311: MCLK z pinu w stosunku 256x, I2S 16 bit, głośność sprzętowa 85 %
    (`WS154_CODEC_VOLUME`), mikrofon wyłączony.
- `es8311.cpp/.h`, `es8311_reg.h` to sterownik ES8311 firmy Espressif (Apache-2.0), skopiowany z przykładów Waveshare.
  W nagłówku zamieniono `driver/i2c.h` na `hal/i2c_types.h`.

### Wyświetlacz ST7789 240x240
- `src/displays/dspcore.h` dostał nową gałąź dla `DSP_ST7789_240`. Ten fork obsługiwał wcześniej tylko ST7789 320x240.
- `src/displays/displayST7789.h` dołącza konfigurację 240x240.
- `src/displays/conf/displayST7789_240conf.h` (nowy plik) to układ elementów ekranu 240x240. Powstał na bazie
  `displayST7789conf.h` z forka i `displayST7789_240conf.h` z [e2002/yoradio](https://github.com/e2002/yoradio).
- `src/displays/displayST7789.cpp`:
  - na tej płytce kolory są domyślnie odwrócone, tak jak w `DSP_ST7789_170`, żeby barwy były poprawne;
  - ekran jest domyślnie obrócony o 180°, zgodnie z ułożeniem w obudowie. Opcja „flip screen” w ustawieniach WWW
    przywraca orientację 0°.
  - Przesunięcie obrazu w pamięci ST7789 zostaje domyślne z biblioteki Adafruit. Inne ustawienie przesuwało obraz
    o 80 px.
- `platformio.ini`: dodano bibliotekę `adafruit/Adafruit ST7735 and ST7789 Library`.
- Polskie znaki na ekranie: `copy_glcdfont.py` (w `platformio.ini` jako `extra_scripts = pre:...`) przed każdą
  kompilacją kopiuje czcionkę 5x7 z `locale/glcdfont/PL, SK, DE, RO, S, A/glcdfont.c` do biblioteki Adafruit GFX
  w `.pio/libdeps`. `utf8Pol.cpp` zamienia ą, ę, ł itd. na znaki z tej czcionki, a zwykła czcionka GFX pokazywała
  w ich miejscu inne symbole. Oryginalny yoRadio każe skopiować ten plik ręcznie, a kopia znika przy ponownej
  instalacji bibliotek.
- Nazwa stacji bez ogonków podczas grania: wiele stacji wysyła w metadanych strumienia (`icy-name`, `StreamTitle`)
  nazwę bez polskich liter, np. „RADIO PLONSK”, choć w playliście jest „RADIO PŁOŃSK”. Po STOP ekran wracał do nazwy
  z playlisty, dlatego wtedy ogonki były widoczne. `Config::setTitle()` i `Config::setStation()` w
  `src/core/config.cpp` porównują teraz oba napisy bez ogonków i bez rozróżniania wielkości liter. Gdy to ta sama
  nazwa, na ekranie i na stronie WWW zostaje wersja z playlisty.

### Dotyk CST816
- `src/core/options.h`: nowa stała `TS_MODEL_CST816 4`.
- `src/core/touchscreen.cpp` ma nowy sterownik. Po resecie (pin 47) wyłącza autousypianie układu i co 20 ms
  odczytuje rejestry 0x02–0x06 (liczba palców, X, Y). Współrzędne obracają się razem z ekranem, więc dotyk też jest
  domyślnie obrócony o 180°. Opcja „flip touch” go przywraca.
- `src/core/config.cpp`: dla CST816 inicjalizowane jest I2C i skan magistrali. W logu startowym widać znalezione
  adresy, np. 0x15 i 0x18.

### Audio
- `src/core/options.h`: nowa opcja `I2S_MCLK` (domyślnie -1, nieużywana).
- `src/core/player.cpp` przekazuje `I2S_MCLK` do `setPinout()`. Biblioteka audio generuje MCLK = 256 x fs.

### Strona WWW
- `data/www/style.css.gz`: na telefonie (szerokość do 480 px) lista stacji w odtwarzaczu ma stałą wysokość
  (60 % ekranu, min. 260 px), a strona przewija się w całości. Wcześniej lista dostawała tylko resztę ekranu
  i kurczyła się, nawet do jednej pozycji, gdy tytuł utworu zawijał się do kolejnej linii.
- `data/www/player.html.gz`: odstęp pod polem wyszukiwania stacji zmniejszony z 90 px do 8 px. Rozwijana lista
  gatunków wyświetla się nad listą stacji, więc nie potrzebuje zarezerwowanego miejsca.
- `data/www/script.js.gz`: gdy żadna stacja w `playlist.csv` nie ma gatunku (4. kolumna), rozwijana lista gatunków
  się nie pokazuje, a opis i podpowiedź pola mówią tylko o wyszukiwaniu po nazwie. Po dopisaniu gatunków filtr
  wraca automatycznie.

- `data/www/style.css.gz`: długie nazwy bez spacji (np. adres strumienia zamiast nazwy stacji) łamią się do kolejnej
  linii. Wcześniej rozpychały stronę na telefonie do ok. 800 px szerokości, przez co cała strona się
  pomniejszała, a ikony zmieniały rozmiar. Na ekranach dotykowych przyciski nie zostają podświetlone po stuknięciu.
- `data/www/style.css.gz`: na telefonie (do 480 px) boczne marginesy odtwarzacza zmniejszone z 40 px do 16 px
  z każdej strony, więc przyciski i lista stacji są szersze. Minimalna szerokość strony zmniejszona z 333 px do
  300 px, przez co na ekranach 320 px strona nie przewija się już w bok. Pole wyszukiwania ma czcionkę 16 px, a
  rozwijana lista gatunków jest wyższa (do połowy ekranu zamiast 80 px, czyli ok. dwóch pozycji) i ma większe
  pozycje do stukania.
- `data/www/style.css.gz`: na telefonie (do 600 px) edytor playlisty otwiera się na cały ekran, a każda stacja jest
  kartą: numer, zaznaczenie, nazwa i odsłuch w pierwszym wierszu, adres w drugim, głośność i gatunek w trzecim.
  Wcześniej edytor był tabelą o szerokości 880 px, przewijaną w bok, z ukrytym przyciskiem zamknięcia. Przyciski
  Import/Export/Add/Remove/Save zawijają się do kolejnego wiersza.
- `data/www/player.html.gz`, `data/www/script.js.gz`: krótsza podpowiedź w polu wyszukiwania („Station name or
  genre”), bo dłuższa była obcięta na telefonie. Po wybraniu gatunku z listy jego nazwa pojawia się w polu jako
  podpowiedź, więc widać, który filtr jest włączony.
- `data/www/script.js.gz`: przełączanie radio/karta SD działa bez odświeżania strony. Po kliknięciu ikony karty
  od razu pojawia się ikona globusa (powrót do radia), a wcześniej pokazywała się z powrotem ta sama ikona. Gdy
  połączenie ze stroną zerwie się w trakcie zmiany trybu, strona po ponownym połączeniu sama pobiera aktualny stan
  i listę.
- `data/www/script.js.gz`: lista stacji nie znika przy zmianie stacji. Gdy radio jest zajęte (łączy się ze stacją
  albo czyta kartę) i pobranie listy się nie uda, strona zostawia poprzednią listę i ponawia próbę co 2 s (do 5 razy).
  Wcześniej lista zostawała pusta do odświeżenia strony. Lista jest pobierana ponownie tylko po zmianie trybu
  radio/SD, a nie przy każdym ponownym połączeniu.

### Odtwarzacz
- `src/core/player.cpp`: przy szybkim przełączaniu kilku stacji odtwarzacz łączy się tylko z ostatnią wybraną.
  Wcześniej każde polecenie zmiany stacji czekało w kolejce i radio łączyło się po kolei z każdą stacją
  (do ok. 6 s na każdą), co dawało nawet minutę ciszy.
- `src/audioI2S/audiolib_structs.hpp`: bufor DMA I2S zwiększony z 16x192 do 16x512 ramek (~185 ms zamiast ~70 ms
  dźwięku). Ma to ograniczyć przerwy w dźwięku przy otwieraniu stron WWW (np. opcji), gdy odczyt plików z SPIFFS
  chwilowo wstrzymuje pętlę dekodera.
- `src/core/config.cpp` (`initPlaylistMode`): gdy radio startuje w trybie karty SD, a karty nie ma, wraca do radia
  internetowego i normalnie wczytuje listę stacji. Wcześniej funkcja kończyła się przed ustawieniem `_bootDone`,
  więc do następnego restartu każde przełączenie na kartę tylko zmieniało tryb. Karta nie była wtedy indeksowana
  (pusta lista, ekran „indeksowanie SD”), a strona nie dostawała nowego stanu.
- Karta SD jest przeglądana folderami (`src/core/sdmanager.cpp`, `config.cpp/.h`, `player.cpp`, `controls.cpp`,
  `options.h`). Lista zawiera tylko bieżący folder: na górze „..” (powrót wyżej, nie ma go w folderze głównym), potem
  podfoldery jako `[nazwa]` i pliki audio, każda grupa alfabetycznie. Program czyta tylko nazwy w tym jednym
  folderze, więc lista wczytuje się od razu, nawet na dużej karcie. Wcześniej przeglądał całą kartę do 3 poziomów
  w głąb, co przy wielu plikach trwało około minuty i blokowało radio oraz stronę.
  - Wybranie folderu (na stronie albo przyciskiem na radiu) wczytuje jego listę i niczego nie odtwarza. Na ekranie
    radia lista zostaje otwarta, z kursorem na pierwszym pliku.
  - Następny, poprzedni, automatyczne przejście po końcu utworu i losowanie działają tylko na plikach bieżącego
    folderu. Po ostatnim pliku odtwarzanie zaczyna się od pierwszego. Jeśli folder zmieniono w trakcie grania, po
    bieżącym utworze gra pierwszy plik nowego folderu.
  - Po wejściu w tryb karty (także po restarcie) zawsze startuje folder główny: `/music` (opcja `SD_SCAN_DIR`), a gdy
    go nie ma, główny folder karty. Pomijane są foldery ukryte, `System Volume Information`, `/data` (tu radio
    zapisuje swoją listę) i foldery z plikiem `.nomedia`.
  - Ścieżka pliku może mieć najwyżej 169 znaków (`BUFLEN`), dłuższej radio nie otworzy.
- `src/displays/widgets/widgets.cpp/.h`, `src/core/display.cpp`: lista na ekranie radia jest czytana od nowa przy
  każdym otwarciu. Przy `PLAYLIST_SCROLL_MOVING_CURSOR` widżet zapamiętywał stronę listy i czytał ją ponownie tylko
  przy zmianie strony, więc po przełączeniu radio/SD albo folderu karty pokazywał stacje radiowe lub poprzedni folder.
- `data/www/script.js.gz`, `data/www/style.css.gz`: na liście karty SD foldery mają ikonę folderu i pogrubioną nazwę,
  a „..” ikonę strzałki. Po zmianie folderu strona sama wczytuje nową listę.

### Wgrywanie przez WiFi (OTA)
Program i system plików można wgrywać bez kabla, gdy radio działa w sieci (`USE_OTA true` w `myoptions.h`):
- **Z PlatformIO:** w `platformio.ini` jest środowisko `ota` (`upload_protocol = espota`). Wpisz w nim IP radia
  w `upload_port` i uruchom `pio run -e ota -t upload` (program) albo `pio run -e ota -t uploadfs` (system plików).
  W VS Code wybierz środowisko `env:ota` na dolnym pasku i użyj zwykłych przycisków Upload / Upload Filesystem Image.
  Domyślne środowisko (`default_envs`) dalej wgrywa przez USB. Jeśli w `myoptions.h` ustawisz `OTA_PASS`,
  odkomentuj `upload_flags = --auth=...`.
- **Z przeglądarki:** strona aktualizacji radia (formularz z wyborem firmware / SPIFFS) przyjmuje
  `.pio/build/esp32-s3-devkitc1-n16r8/firmware.bin` albo `spiffs.bin`.
- `src/main.cpp`: przed wgraniem systemu plików przez ArduinoOTA SPIFFS jest odmontowywany.

### Wgrywanie
- `platformio.ini`: `upload_speed = 460800`. Przy wyższej prędkości wgrywanie przez natywne USB zrywało się.
- Jeśli wgrywanie się nie udaje, wymuś tryb pobierania: odłącz USB, przytrzymaj lewy przycisk (BOOT), podłącz USB
  i puść przycisk.
- System plików WWW (`data/`) można też wgrać przez przeglądarkę, gdy radio działa w sieci.

> **Uwaga:** na posiadanym egzemplarzu ESP32-S3-Touch-LCD-1.54 wgrywanie systemu plików z poziomu PlatformIO
> („Upload Filesystem Image” / `pio run -t uploadfs`) nie działało: przerywało się błędem „The chip stopped responding”.
> Pliki strony WWW wgrano przez przeglądarkę: po wgraniu samego firmware wejdź na `http://<IP-radia>/`
> i wskaż pliki z `data/www/` oraz `data/data/playlist.csv`.

## Obsługa przycisków

| Przycisk | Klik | Przytrzymanie | Podwójny klik |
|---|---|---|---|
| Lewy (GPIO 0) | ciszej / w górę listy | ciszej ciągle / przewijanie | poprzednia stacja |
| Środkowy (GPIO 5) | start/stop, zatwierdzenie stacji | otwiera i zamyka listę stacji | – |
| Prawy (GPIO 4) | głośniej / w dół listy | głośniej ciągle / przewijanie | następna stacja |

Gdy na ekranie jest lista stacji albo plików i folderów karty SD, lewy i prawy przycisk przesuwają kursor w górę i w dół,
przytrzymanie przewija listę, a środkowy wybiera pozycję. Podwójne kliknięcie lewym lub prawym przesuwa wtedy kursor
o dwie pozycje, a na ekranie głośności zmienia ją o dwa kroki. Wcześniej (`src/core/controls.cpp`, `onBtnDoubleClick`)
było poza ekranem odtwarzacza ignorowane, więc przy szybkim naciskaniu kursor gubił kroki.

Dotyk: przesunięcie w lewo/prawo zmienia głośność, przesunięcie w górę/dół otwiera i przewija listę stacji,
stuknięcie to start/stop lub zatwierdzenie.

GPIO 5 to jednocześnie sprzętowy przycisk zasilania. Przy zasilaniu z akumulatora długie przytrzymanie może
wyłączyć płytkę.

## Kompilacja

```bash
pio run
```

Firmware: `.pio/build/esp32-s3-devkitc1-n16r8/firmware.bin`. Obraz WWW (`pio run -t buildfs`):
`.pio/build/esp32-s3-devkitc1-n16r8/spiffs.bin`.
