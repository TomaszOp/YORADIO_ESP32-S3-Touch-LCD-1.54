/*  Waveshare ESP32-S3-Touch-LCD-1.54 (SKU 33868)
    Inicjalizacja specyficzna dla płytki, wywoływana z setup() przez yoradio_on_setup():
    - podtrzymanie zasilania z akumulatora (GPIO2),
    - magistrala I2C (kodek ES8311, dotyk CST816, QMI8658),
    - SPI wyświetlacza na pinach 38/39,
    - kodek audio ES8311 (MCLK z ESP32, I2S slave).
    Piny wg https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54 */
#include "../../core/options.h"
#ifdef BOARD_WS_S3_TOUCH_LCD_154
#    include <Arduino.h>
#    include <SPI.h>
#    include <Wire.h>
#    include "es8311.h"

#    ifndef WS154_PWR_HOLD_PIN
#        define WS154_PWR_HOLD_PIN 2
#    endif
#    ifndef WS154_I2C_SDA
#        define WS154_I2C_SDA 42
#    endif
#    ifndef WS154_I2C_SCL
#        define WS154_I2C_SCL 41
#    endif
#    ifndef WS154_CODEC_VOLUME
#        define WS154_CODEC_VOLUME 85 // 0..100, głośność sprzętowa kodeka; głośność radia reguluje yoRadio
#    endif

static void ws154_codecInit() {
    es8311_handle_t es = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
    if (!es) {
        Serial.println("[WS154] ES8311 create failed");
        return;
    }
    // Audio.cpp generuje MCLK = 256 * fs, więc wystarczy ustawić stosunek 256x
    const es8311_clock_config_t clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = 16000 * 256,
        .sample_frequency = 16000,
    };
    esp_err_t err = es8311_init(es, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    if (err == ESP_OK) err = es8311_voice_volume_set(es, WS154_CODEC_VOLUME, NULL);
    if (err == ESP_OK) err = es8311_microphone_config(es, false);
    Serial.printf("[WS154] ES8311 init %s\n", err == ESP_OK ? "OK" : "FAILED");
}

void yoradio_on_setup() {
    // Bez tego płytka zasilana z akumulatora wyłącza się po puszczeniu przycisku PWR
    pinMode(WS154_PWR_HOLD_PIN, OUTPUT);
    digitalWrite(WS154_PWR_HOLD_PIN, HIGH);

    Wire.begin(WS154_I2C_SDA, WS154_I2C_SCL);
    Wire.setClock(400000);

    // Adafruit_SPITFT woła SPI.begin() bez pinów; pierwsze wywołanie z pinami płytki wygrywa
    SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);

    ws154_codecInit();
}

#endif // BOARD_WS_S3_TOUCH_LCD_154
