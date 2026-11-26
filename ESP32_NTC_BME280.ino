#include <Adafruit_BME280.h>

#define HEIZUNG_PIN      16
#define NTC_PIN          34
#define NTC_SCHWELLE     2000     // 1750-1800 = 1-2 °C, 3000-3100 = Handwarm
#define NTC_SAMPLES      5
#define NTC_INTERVALL_MS 60000    // Messung alle 60 Sekunden
#define BME_INTERVALL_MS 60000     // Messung alle 60 Sekunden

Adafruit_BME280 bme;
bool bmeAvailable = false;
volatile bool zustand = false; 
float global_temp = 33.33;
float global_druck = 999.99;
float global_feuchte = 99.9;

void bmeTask(void *pvParameters) {
    const TickType_t delay = pdMS_TO_TICKS(BME_INTERVALL_MS);
    for (;;) {
        float t, p, h;
        if (bmeAvailable) {
            t = bme.readTemperature();
            p = bme.readPressure() / 100.0F;
            h = bme.readHumidity();
            if (isnan(t) || isnan(p) || isnan(h)) {
                Serial.println("Falsche BME280-Daten!");
            } else {
                global_temp = t;
                global_druck = p;
                global_feuchte = h;
            }
        } else {
            Serial.println("BME280 nicht verfügbar");
        }
        Serial.printf("\n---------- BME280 -------------\nTemperatur: %.2f °C\nLuftdruck: %.2f hPa\nLuftfeuchtigkeit: %.2f %%\n",
                      global_temp, global_druck, global_feuchte);
        Serial.println("-------------------------------");
        Serial.println("\n");
        vTaskDelay(delay);
    }
}

void ntcTask(void *pvParameters) {
  (void)pvParameters;
  for (;;) {
    long summe = 0;
    for (int i = 0; i < NTC_SAMPLES; i++) {
      summe += analogRead(NTC_PIN);
      vTaskDelay(pdMS_TO_TICKS(2));
    }
    int mittelwert = summe / NTC_SAMPLES;
    float temperatur = map(mittelwert, 1875, 2650, 1.6, 19.56);
    zustand = (mittelwert < NTC_SCHWELLE);
    Serial.println(F("-------------------------------"));
    Serial.print(F("NTC Mittelwert: "));
    Serial.println(mittelwert);
    Serial.print(F("Geschätzte Temperatur: "));
    Serial.print(temperatur);
    Serial.println(" °C");
    Serial.println(zustand ? F("Zu kalt") : F("Warm genug"));
    Serial.print(F("Heizung ist "));
    Serial.println(zustand ? F("an") : F("aus"));
    Serial.println(F("-------------------------------"));
    vTaskDelay(pdMS_TO_TICKS(NTC_INTERVALL_MS));
  }
}

void heizungTask(void *pvParameters) {
    (void)pvParameters;
    bool letzter_zustand = false;
    for (;;) {
        if (zustand != letzter_zustand) {
            letzter_zustand = zustand;
            if (zustand) {
                digitalWrite(HEIZUNG_PIN, HIGH);
            } else {
                digitalWrite(HEIZUNG_PIN, LOW);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Starte ..."));

    pinMode(NTC_PIN, INPUT);
    pinMode(HEIZUNG_PIN, OUTPUT);
    digitalWrite(HEIZUNG_PIN, LOW);

    if (bme.begin(0x76)) {
        Serial.println("BME280 gefunden.");
        bmeAvailable = true;
    } else {
        Serial.println("Kein BME280 gefunden! Nutze Standard-Werte.");
        bmeAvailable = false;
    }
    xTaskCreatePinnedToCore(ntcTask,     "NTC",        2048, nullptr, 1, nullptr, 0);
    xTaskCreatePinnedToCore(heizungTask, "Heizung",    2048, nullptr, 2, nullptr, 1);
    xTaskCreatePinnedToCore(bmeTask,     "BME280",     4096, nullptr, 1, nullptr, 1);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
