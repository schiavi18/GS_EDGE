/* ---------------------------------------------------------
   InclusiveWork Station – Estação de Acessibilidade
    Desenvolvido por: Julia Schiavi | Josue Faria
   Objetivo:
   Criar uma solução de monitoramento inclusiva que coleta 
   dados ambientais e envia para plataformas externas usando 
   MQTT e HTTP. O sistema auxilia PCDs garantindo 
   conforto, segurança e acessibilidade no ambiente de trabalho.
----------------------------------------------------------*/

#include <WiFi.h>              
#include "DHT.h"               
#include <HTTPClient.h>        
#include <PubSubClient.h>      

//  Configurações do Sensor DHT22 
#define DHTPIN 15              
#define DHTTYPE DHT22         
DHT dht(DHTPIN, DHTTYPE);      

//  Configuração do LDR 
#define LDR_PIN 34             

//  Credenciais WiFi
const char* ssid = "Wokwi-GUEST";  
const char* password = "";         

//  Configurações MQTT 
const char* mqttServer = "broker.hivemq.com"; // Servidor MQTT público
int mqttPort = 1883;                          // Porta padrão MQTT

WiFiClient espClient;           // Cliente WiFi
PubSubClient client(espClient); // Cliente MQTT

unsigned long lastRead = 0;     // Controle de intervalo entre leituras

/* 
   Função: reconnectMQTT()
   Objetivo:
   - Garante que o ESP32 sempre esteja conectado ao broker MQTT
   - Reinscreve no tópico de comandos
*/
void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("InclusiveWorkESP32")) {
      client.subscribe("inclusivework/comandos");  // Inscrição para comandos recebidos
    } else {
      delay(1000); 
    }
  }
}

/* 
   Função: setup()
   Executa uma única vez quando o ESP32 inicia
*/
void setup() {
  Serial.begin(115200);   
  dht.begin();            
  pinMode(LDR_PIN, INPUT); 

  // ---- Conexão WiFi ----
  WiFi.begin(ssid, password);
  Serial.println("Conectando WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }
  Serial.println("WiFi conectado!");

  //  Configuração do servidor MQTT 
  client.setServer(mqttServer, mqttPort);
}

/* 
   Função: loop()
   - Lê sensores
   - Envia dados para MQTT
   - Exibe saída simulando uma API HTTP
   - Emite alertas inteligentes
*/
void loop() {
  if (!client.connected()) reconnectMQTT();  // Mantém conexão MQTT
  client.loop();                             

  unsigned long now = millis();
  

  if (now - lastRead >= 3000) {
    lastRead = now;

    // ---- Lendo sensores ----
    float temp = dht.readTemperature();  
    float hum = dht.readHumidity();      
    int ldr = analogRead(LDR_PIN);      

    // Verifica se o DHT22 retornou valores válidos
    if (isnan(temp) || isnan(hum)) {
      Serial.println("Erro ao ler DHT!");
      return;
    }

    // ---- Classificação da luminosidade ----
    String lightStatus;
    if (ldr < 1000)       lightStatus = "AMBIENTE_ESCURO";
    else if (ldr < 2500)  lightStatus = "ILUMINACAO_MEDIA";
    else                  lightStatus = "AMBIENTE_CLARO";

    // JSON 
    String payload = "{";
    payload += "\"temperature\":" + String(temp,1) + ",";
    payload += "\"humidity\":" + String(hum,1) + ",";
    payload += "\"light\":" + String(ldr) + ",";
    payload += "\"light_status\":\"" + lightStatus + "\"";
    payload += "}";

    //  Envio MQTT 
    client.publish("inclusivework/dados", payload.c_str());

    // ---- Simulação HTTP (para exibir no monitor serial) ----
    Serial.println("\n[HTTP] POST /dados");
    Serial.println(payload);
    Serial.println("[200] OK");

    // ---- Alertas inteligentes ----
    if (temp >= 30)
      Serial.println("⚠ ALERTA: Calor excessivo – risco para foco e bem-estar.");

    if (ldr < 1000)
      Serial.println("⚠ ALERTA: Ambiente escuro – não recomendado para videoconferências.");
  }
}
