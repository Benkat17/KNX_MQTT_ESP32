//#include <Arduino.h>
#include <KnxTpUart.h>
#include "PubSubClient.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

#include "config_page_html.h"
#include "success_page_html.h"

const char* config_file = "/config.txt";

//WiFi
const char* wifi_ssid = ".4G";
const char* wifi_password = "  ";

//Point d'acces
const char* access_ssid = "ESP_32_ACCESS";
const char* access_password = "ESP_32_ACCESS";

//MQTT
const char* mqtt_server = "";
const int mqtt_port = ;
const char* mqtt_user = "";
const char* mqtt_password = "";
const char* topic = "knx";

WiFiClient espClient;
PubSubClient client(espClient);
bool configSuccess = false;

WebServer Server(80);

// Taille du document JSON
const size_t capacity = 200;
DynamicJsonDocument jsonDocument(capacity);

//KNX
const char* knx_address = "0/0/1";
const char* knx_init = "1.1.99";
KnxTpUart knx(&Serial2, knx_init);
bool toSend = true; // To keep track of what is being sent

//Méthode pour Connexion à MQTT
void reconnect() {
  // Connexion au serveur MQTT
  client.setServer(mqtt_server, mqtt_port);
  //client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Connected to MQTT");
      client.subscribe(topic);
    } 
    else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

//Méthode pour connection au WiFi
void setup_wifi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);  
  int attempts = 0; //Nombre d'essais

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    Serial.println("");
    delay(3000); //attente de connexion
    attempts++;

    if (attempts > 5) {
      Serial.println("Impossible de se connecter au réseau Wi-Fi !");
      //WiFi.softAP(access_ssid, access_password); // Création du point d'accès en cas d'échec de la connexion
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connecté au réseau WiFi");
    reconnect();
  }
}

void handleRoot() {
  // Redirection HTTP vers votre page HTML
  Server.sendHeader("Location", "/configure");
  Server.send(302);
  return;
}

void handleConfigure() {
  String ssid = Server.arg("ssid");
  String password = Server.arg("password");

  // Écriture des informations de configuration dans SPIFFS
  File configFile = SPIFFS.open(config_file, "w");
  if (!configFile) {
    Serial.println("Erreur lors de l'ouverture du fichier de configuration !");
    return;
  }
  configFile.println(ssid);
  configFile.println(password);
  configFile.close();

  // Code pour enregistrer les informations de configuration
  Server.sendHeader("Location", "/success");
  Server.send(302);

}


void setup() {

  //Initialisation de la communication série pour le débogage
  Serial.begin(115200);

  Serial.println("Tentative de connexion au réseau WiFi...");
  setup_wifi(wifi_ssid, wifi_password );

  if (WiFi.status() != WL_CONNECTED) {
  // Initialisation du système de fichiers SPIFFS
    if (!SPIFFS.begin(true)) {
      Serial.println("Erreur lors de l'initialisation de SPIFFS !");
      return;
    }

    // Lecture des informations de configuration à partir du fichier
    File configFile = SPIFFS.open(config_file, "r");
    if (configFile) {
      String ssid = configFile.readStringUntil('\n');
      String password = configFile.readStringUntil('\n');

      ssid.trim();
      password.trim();
      configFile.close();
    
      //Conversion des String en char*
      const char* config_ssid = ssid.c_str();
      const char* config_password = password.c_str();

      Serial.println("Nom du WiFi :");
      Serial.print(config_ssid);

      Serial.println("Mot de Passe :");
      Serial.print(config_password);

      Serial.println("");

      // Connexion au réseau Wi-Fi en utilisant les informations récupérées
      Serial.println("Tentative de connexion au réseau WiFi avec la configuration enregistrée...");
      setup_wifi(config_ssid, config_password);
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Création du point d'accès");
        WiFi.softAP(access_ssid, access_password);
      }
    } 
  }

  // Configuration du Point d'accès Wi-Fi
  Server.on("/", HTTP_GET, handleRoot);
  Server.on("/configure", HTTP_GET, []() { Server.send(200, "text/html", config_page_html); });
  Server.on("/configure", HTTP_POST, handleConfigure);
  Server.on("/success", HTTP_GET, []() {
    configSuccess = true; 
    Server.send(200, "text/html", success_page_html); 
  });
  Server.begin();

  // Initialisation de la communication série pour KNX
  Serial2.begin(19200, SERIAL_8E1);
  // Réinitialisation de la communication KNX
  knx.uartReset();
  knx.addListenGroupAddress("0/0/1");
  knx.addListenGroupAddress("0/2/0");
  //knx.groupRead(knx_address);*/
}

void callback(char* receivedTopic, byte* payload, unsigned int length) {
  // Traitement du message reçu (si nécessaire)
}

void loop() {

  Server.handleClient();

  if (configSuccess) {
    Serial.println("Configuration réussie");
    Serial.println("Redemmarage de l'ESP");
    delay(3000);
    ESP.restart();
  }
   
  while(WiFi.status() == WL_CONNECTED)
  {
    client.loop();
    //Vérification des événements KNX
    knx.groupWriteBool(knx_address, toSend);
    //Serial.println(toSend);
    toSend = !toSend;

    KnxTpUartSerialEventType eType = knx.serialEvent();

    if (eType == TPUART_RESET_INDICATION) {
      Serial.println("RESET KNX");
    }

    //Evaluation of the received telegram -> only KNX telegrams are accepted
    if (eType == KNX_TELEGRAM) {
      KnxTelegram* telegram = knx.getReceivedTelegram();

      //Recupération de l'adresse physique
      String sender =
        String(0 + telegram->getSourceArea())   + "." +
        String(0 + telegram->getSourceLine())   + "." +
        String(0 + telegram->getSourceMember());

      //Recupération de l'adresse de Groupe
      String target =
        String(0 + telegram->getTargetMainGroup())   + "/" +
        String(0 + telegram->getTargetMiddleGroup()) + "/" +
        String(0 + telegram->getTargetSubGroup());

      //Recupération de la Priorité
      int priority = telegram->getPriority();

      //Recupération de la Longueur de la charge utile
      int payloadLength = telegram->getPayloadLength();

      //Envoie des données en format ArduinoJson
      // Réinitialiser le document JSON avant de l'utiliser à nouveau
      jsonDocument.clear();
      jsonDocument["eType"] = eType;
      jsonDocument["Adresse physique"] = sender;
      jsonDocument["Adresse de Groupe"] = target;
      jsonDocument["Priorité"] = priority;
      jsonDocument["Taille de la Payload"] = payloadLength;

      // Convertir l'objet JSON en chaîne JSON
      String jsonString;
      serializeJsonPretty(jsonDocument, jsonString); // Utilise serializeJsonPretty pour une sortie formatée

      if (client.connected()) {
        client.publish(topic, jsonString.c_str());
      }
    }

    if (eType == IRRELEVANT_KNX_TELEGRAM) {
      Serial.println("TELEGRAMME NON PERTINENT");
    }

    if (eType == UNKNOWN_EVENT) {
      Serial.println("EVENEMENT INCONNU");
    }
    
    else{
      setup_wifi(wifi_ssid, wifi_password);
    }
    
    delay(5000);
  }
  
}
