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
#include <LiquidCrystal_I2C.h>
#include "Botao.h"

const char *ssid = "SENAI EDUCACIONAL";
const char *password = "info@134";

const char *BROKER = "broker.emqx.io";
const int PORTA = 1883;

const char *TOPICO_SUB = "senai134/esp32/SmartPet/status";
const char *TOPICO_PUB = "senai134/esp32/SmartPet/comando";

const int PINO_LED = 5;
const int PINO_BOTAO = 4;

LiquidCrystal_I2C lcd(0x27, 20, 4);
Botao btn(PINO_BOTAO);

String id;

WiFiClient espClient;
PubSubClient mqtt(espClient);

bool ledState = false;
bool foodState = false;

String lastFeedingTime = "--:--";
String nextFeedingTime = "--:--";

void atualizarLCD()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("SMARTPET");

    lcd.setCursor(0, 1);
    lcd.print("LED: ");
    lcd.print(ledState ? "Ligado" : "Desligado");

    lcd.setCursor(0, 2);
    lcd.print("Ultima: ");
    lcd.print(lastFeedingTime);

    lcd.setCursor(0, 3);
    lcd.print("Proxima: ");
    lcd.print(nextFeedingTime);
}

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

    Serial.println(message);

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.print("Erro JSON: ");
        Serial.println(error.c_str());
        return;
    }

    ledState = doc["ledState"] | false;
    foodState = doc["foodState"] | false;

    const char *ultima = doc["lastFeedingTime"] | "--:--";
    const char *proxima = doc["nextFeedingTime"] | "--:--";

    lastFeedingTime = ultima;
    nextFeedingTime = proxima;

    digitalWrite(PINO_LED, ledState ? HIGH : LOW);

    atualizarLCD();

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

void enviarComandoAlimentacao()
{
    if (!mqtt.connected())
    {
        Serial.println("MQTT desconectado!");
        return;
    }

    JsonDocument doc;

    doc["ledState"] = true;
    doc["foodState"] = true;

    String message;

    serializeJson(doc, message);

    bool enviado = mqtt.publish(TOPICO_PUB, message.c_str());

    if (enviado)
    {
        Serial.println("Comando enviado!");
        Serial.println(message);

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("SMARTPET");

        lcd.setCursor(0, 1);
        lcd.print("Solicitando");

        lcd.setCursor(0, 2);
        lcd.print("alimentacao...");
    }
    else
    {
        Serial.println("Erro ao enviar comando MQTT.");
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(PINO_LED, OUTPUT);
    digitalWrite(PINO_LED, LOW);

    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("SMARTPET");
    lcd.setCursor(0, 1);
    lcd.print("Conectando WiFi...");

    btn.iniciar();
    btn.intervaloDebounce(20);

    WiFi.begin(ssid, password);

    Serial.println("Conectando ao WiFi...");

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("\nWiFi conectado!");

    lcd.clear();
    lcd.print("WiFi conectado!");

    mqtt.setServer(BROKER, PORTA);
    mqtt.setCallback(callback);

    id = "esp32-receptor-" +
         String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.print("ID do receptor: ");
    Serial.println(id);

    conectarMQTT();

    atualizarLCD();
}

void loop()
{
    btn.atualizar();

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.reconnect();
    }

    if (!mqtt.connected())
    {
        conectarMQTT();
    }

    mqtt.loop();

    if (btn.clicou())
    {
        Serial.println("Botao pressionado!");

        enviarComandoAlimentacao();
    }
}