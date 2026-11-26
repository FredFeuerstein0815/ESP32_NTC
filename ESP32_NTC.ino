#define HEIZUNG_PIN      16
#define NTC_PIN          34
#define NTC_SCHWELLE     2450     // 1750-1800 = 1-2 °C, 3000-3100 = Handwarm
#define NTC_SAMPLES      5
#define NTC_INTERVALL_MS 60000    // Messung alle 60 Sekunden = 1 Minute

// Zustand zwischen Tasks teilen
volatile bool zustand = false;

void sensorTask(void *pvParameters) {
  (void)pvParameters;
  for (;;) {
    long summe = 0;
    for (int i = 0; i < NTC_SAMPLES; i++) {
      summe += analogRead(NTC_PIN);
      vTaskDelay(pdMS_TO_TICKS(2));
    }
    int mittelwert = summe / NTC_SAMPLES;
    float temperatur = map(mittelwert, 2250, 3461, 1, 23);  // grobe Schätzung der Temperatur
    zustand = (mittelwert < NTC_SCHWELLE);
    Serial.println(F("---------------------------"));
    Serial.print(F("NTC Mittelwert: "));
    Serial.println(mittelwert);
    Serial.print(temperatur);
    Serial.println(" °C");
    Serial.println(zustand ? F("Zu kalt") : F("Warm genug"));
    Serial.print(F("Heizung ist "));
    Serial.println(zustand ? F("an") : F("aus"));
    Serial.println(F("---------------------------"));
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
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
  
void setup() {
  Serial.begin(115200);
  Serial.println(F("Starte"));
  pinMode(NTC_PIN, INPUT);
  pinMode(HEIZUNG_PIN, OUTPUT);
  digitalWrite(HEIZUNG_PIN, LOW);
  xTaskCreatePinnedToCore(sensorTask,    "NTC",       2048, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(heizungTask,   "Heizung",   4096, nullptr, 2, nullptr, 1);
}
  
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
