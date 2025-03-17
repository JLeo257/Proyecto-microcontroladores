#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <math.h>

// Definir el pin analógico conectado al MAX4466
const int micPin = 34;  // En ESP32, usar GPIO 32-39 para lecturas ADC

// Parámetros de calibración
const float referenceVoltage = 3.3;   // Voltaje de referencia del ESP32
const float dBReference = 94.0;       // Nivel de referencia para 1 Pa (94 dB SPL)
const int sampleWindow = 50;          // Tiempo de muestreo en ms

// Configuración de WiFi
const char* ssid = "Mega_2.4G_FBE8";
const char* password = "26cKT2SZ";

// URL del servidor
const char* serverUrl = "http://192.168.1.13:3000/data"; // Cambia por la URL de tu servidor local

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando a WiFi...");
    }

    Serial.println("Conectado a WiFi");
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        float soundLevel = leerSensorDeSonido();

        // Crear el JSON para enviar los datos
        String jsonData = "{\"zona\": 1, \"decibelios\": " + String(soundLevel) + "}";

        WiFiClient client;
        HTTPClient http;

        http.begin(client, serverUrl);
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(jsonData);

        if (httpResponseCode > 0) {
            Serial.print("Datos enviados. Código de respuesta: ");
            Serial.println(httpResponseCode);
            Serial.print("Decibelios enviados: ");
            Serial.println(soundLevel);
        } else {
            Serial.print("Error al enviar los datos. Código de respuesta: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    }

    delay(500); // Enviar datos al servidor cada 0.5 segundos
}

float leerSensorDeSonido() {
    unsigned long startMillis = millis(); // Tiempo inicial
    int signalMax = 0;
    int signalMin = 4095; // Rango ADC del ESP32 (0-4095)

    // Medir la señal durante sampleWindow ms
    while (millis() - startMillis < sampleWindow) {
        int sample = analogRead(micPin); // Leer el pin analógico
        if (sample > signalMax) signalMax = sample;
        if (sample < signalMin) signalMin = sample;
    }

    // Calcular voltaje pico a pico
    float peakToPeak = (signalMax - signalMin) * (referenceVoltage / 4095.0);

    // Evitar valores negativos o cercanos a 0 en log10()
    if (peakToPeak <= 0) {
        peakToPeak = 0.0001; // Valor mínimo para evitar errores matemáticos
    }

    // Calcular nivel de ruido en dB SPL
    float dB = 20 * log10(peakToPeak) + dBReference;

    // Mostrar resultado en el Monitor Serie
    Serial.print("Nivel de ruido: ");
    Serial.print(dB - 18); // Ajuste de calibración
    Serial.println(" dB");

    return dB - 18;
}
