//*============================================|
//* Projeto: projeto mqtt - em dupla           |
//* Descrição: Alimentação de pet              |
//* Autor: Gabriel Pedrão e Henrique Jordão    |
//* Data: 04/11/2026                           |
//* Componentes:                               |
//* - ESP 32                                   |
//* - LED                                      |
//* - LCD 20x4                                 |
//* - Botão                                    |
//*============================================|

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char *ssid = "SENAI EDUCACIONAL";
const char *password = "info@134";

const char *BROKER = "broker.emqx.io";
const int PORTA = 1883;

const char *TOPICO_SUB = "senai134/esp32/SmartPet/status";

String id;

WiFiClient espClient;
PubSubClient mqtt(espClient);

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.println("\nMensagem recebida!");
    Serial.print("Topico: ");
    Serial.println(topic);

    String message;

    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("JSON recebido: ");
    Serial.println(message);

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.print("Erro ao interpretar JSON: ");
        Serial.println(error.c_str());
        return;
    }

    bool ledState = doc["ledState"].as<bool>();
    bool foodState = doc["foodState"].as<bool>();

    const char *lastFeedingTime = doc["lastFeedingTime"];
    const char *nextFeedingTime = doc["nextFeedingTime"];

    Serial.println("----------------------------");
    Serial.println("       STATUS SMARTPET      ");
    Serial.println("----------------------------");

    Serial.print("LED: ");
    Serial.println(ledState ? "Ligado" : "Desligado");

    Serial.print("Alimentacao: ");
    Serial.println(foodState ? "Ativa" : "Inativa");

    Serial.print("Ultima alimentacao: ");
    Serial.println(lastFeedingTime);

    Serial.print("Proxima alimentacao: ");
    Serial.println(nextFeedingTime);

    Serial.println("----------------------------");
}

void conectarMQTT()
{
    while (!mqtt.connected())
    {
        Serial.println("Conectando ao broker MQTT...");

        if (mqtt.connect(id.c_str()))
        {
            Serial.println("Conectado ao broker!");

            mqtt.subscribe(TOPICO_SUB);

            Serial.print("Inscrito no topico: ");
            Serial.println(TOPICO_SUB);
        }
        else
        {
            Serial.print("Falha MQTT: ");
            Serial.println(mqtt.state());

            delay(2000);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    WiFi.begin(ssid, password);

    Serial.println("Conectando ao WiFi...");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("\nWiFi conectado!");

    mqtt.setServer(BROKER, PORTA);
    mqtt.setCallback(callback);

    id = "esp32-receptor-" +
         String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.print("ID do receptor: ");
    Serial.println(id);

    conectarMQTT();
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.reconnect();
    }

    if (!mqtt.connected())
    {
        conectarMQTT();
    }

    mqtt.loop();
}