#include <Arduino.h>
#include <KnxTpUart.h>
#include "PubSubClient.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <cstring>
#include <String>

#include "config_page_html.h"
#include "success_page_html.h"

const char* config_file = "/config.txt";

#define BUTTON_PIN 21 // Broche GPIO21 connectée au bouton
#define LONG_PRESS_TIME  5000
// Variables qui vont changer :
int lastState = HIGH; // l'état précédent de la broche d'entrée
int currentState;         // la lecture actuelle de la broche d'entrée
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;


//Point d'acces
const char* access_ssid = "ESP_32_ACCESS";
const char* access_password = "ESP_32_ACCESS";

//adresses
const int maxTaille = 500;
const char* delimiteur = ",";
const char *config_address;

// Initialisation du tableau d'adresses KNX
char** tableauAdresses;
int tailleTableau = 0;

//MQTT
const char* mqtt_server = "***";
const int mqtt_port = ****;
const char* mqtt_user = "***";
const char* mqtt_password = "***";
const char* topic = "knx";

WiFiClient espClient;
PubSubClient client(espClient);
bool configSuccess = false;

WebServer Server(80);

// Taille du document JSON
const size_t capacity = 200;
DynamicJsonDocument jsonDocument(capacity);

//KNX
const char* knx_init = "1.1.99";
KnxTpUart knx(&Serial2, knx_init);
bool toSend = true; // To keep track of what is being sent


//Méthode pour Connexion à MQTT
void reconnect() {
  // Connexion au serveur MQTT
  client.setServer(mqtt_server, mqtt_port);
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
    Serial.println(".");
    delay(3000); //attente de connexion
    attempts++;

    if (attempts > 5) {
      Serial.println("Impossible de se connecter au réseau Wi-Fi !");
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connecté au réseau WiFi");
    reconnect();
  }
}

void handleRoot() {
  // Redirection HTTP vers la page HTML
  Server.sendHeader("Location", "/configure");
  Server.send(302);
  return;
}

void handleConfigure() {
  String ssid = Server.arg("ssid");
  String password = Server.arg("password");
  String address = Server.arg("address");
  
  // Écriture des informations de configuration dans SPIFFS
  File configFile = SPIFFS.open(config_file, "w");
  if (!configFile) {
    Serial.println("Erreur lors de l'ouverture du fichier de configuration !");
    return;
  }
  configFile.println(ssid);
  configFile.println(password);
  configFile.println(address);
  configFile.close();

  //Code pour enregistrer les informations de configuration
  Server.sendHeader("Location", "/success");
  Server.send(302);
}

void deleteConfig(){
  // Ouvrir le fichier de configuration en mode lecture
  File configFile = SPIFFS.open(config_file, "r");
  if (!configFile) {
    Serial.println("Erreur lors de l'ouverture du fichier de configuration en lecture !");
    return ;
  }
  // lecture du SSID, du mot de passe et de l'adresse depuis le fichier de configuration
  String ssid = configFile.readStringUntil('\n');
  String password = configFile.readStringUntil('\n');
  String address = configFile.readStringUntil('\n');
  configFile.close();
  // ouverture du fichier de configuration en mode écriture
  configFile = SPIFFS.open(config_file, "w");
  if (!configFile) {
    Serial.println("Erreur lors de l'ouverture du fichier de configuration en écriture !");
    return ;
  }
  //Suppression du SSID et du password dans le fichier de configuration
  configFile.println("");
  configFile.println("");
  configFile.println(address);

  configFile.close();
  Serial.println("Configuration supprimée avec succès");
  
  WiFi.disconnect(true);  // Déconnexion du réseau WiFi
  Serial.println("Wifi déconnecté");
  Serial.println("Création du point d'accès");
  
  WiFi.softAP(access_ssid, access_password);//Création du poit d'accès
  return ;
}

char** splitAddress(const char *address, const char *delimiteur, int& tailleTableau, const int maxTaille) {
  char **tableauAdresses = new char*[maxTaille];
  char *conv_address = strdup(address);
  if (conv_address == nullptr) {
      Serial.println("Erreur d'allocation de mémoire");
      return tableauAdresses;
  }
  char *adresse = strtok(conv_address, delimiteur);
  while (adresse != NULL && tailleTableau < maxTaille) {
      tableauAdresses[tailleTableau] = strdup(adresse);
      adresse = strtok(NULL, delimiteur);
      tailleTableau++;
  }
  free(conv_address);
  return tableauAdresses;
}

void setup() {
  
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configuration de la broche du bouton BOOT en entrée avec pull-up interne
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8E1);
  
  // Initialisation du système de fichiers SPIFFS
  if (!SPIFFS.begin(true)) {
      Serial.println("Erreur lors de l'initialisation de SPIFFS !");
      return;
  }

  String ssid;
  String password;
  String address;
  // Lecture des informations de configuration à partir du fichier
  File configFile = SPIFFS.open(config_file, "r");
  if (configFile) {
    ssid = configFile.readStringUntil('\n');
    password = configFile.readStringUntil('\n');
    address = configFile.readStringUntil('\n');
  }
  //Suppression des espaces blancs avant et après la chaine
  ssid.trim();
  password.trim();
  address.trim();
  configFile.close();

  //Conversion des String en const char*
  const char* config_ssid = ssid.c_str();
  const char* config_password = password.c_str();
  config_address = address.c_str();

  // Traitement des adresses KNX
  tableauAdresses = splitAddress(config_address, delimiteur, tailleTableau, maxTaille);
  // Utilisation du tableau d'adresses KNX
  for (int i = 0; i < tailleTableau; ++i) {
    String addressString = String(tableauAdresses[i]);
    addressString.trim();
    strcpy(tableauAdresses[i], addressString.c_str());
    
    knx.addListenGroupAddress(tableauAdresses[i]);
  }
  
  // Réinitialisation de la communication KNX
  knx.uartReset();

  if (WiFi.status() != WL_CONNECTED) {
    // Connexion au réseau Wi-Fi en utilisant les informations récupérées
    Serial.println("Tentative de connexion au réseau WiFi avec la configuration enregistrée...");
    Serial.print("Nom du WiFi : ");
    Serial.println(config_ssid);
    Serial.print("Mot de Passe : ");
    Serial.println(config_password);
    setup_wifi(config_ssid, config_password);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Création du point d'accès");
      
      WiFi.softAP(access_ssid, access_password);
    }
  }
  
  Server.on("/", HTTP_GET, handleRoot);
  Server.on("/configure", HTTP_GET, []() { Server.send(200, "text/html", config_page_html); });
  Server.on("/configure", HTTP_POST, handleConfigure);
  Server.on("/success", HTTP_GET, []() {
    configSuccess = true;
    Server.send(200, "text/html", success_page_html); 
  });
  Server.begin();
}
  
void callback(char *receivedTopic, byte *payload, unsigned int length)
{
  // Conversion du payload en chaîne de caractères
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  // Parse le JSON
  DynamicJsonDocument jsonDocument(200); // 200 est la taille du document JSON
  deserializeJson(jsonDocument, message);

  // Extraire les valeurs du JSON
  String ValeurString = jsonDocument["Valeur"];

  bool Valeur;
  if (ValeurString == "1") {
    Valeur = true;
  } else {
    Valeur = false;
  }

  String adressePhysique = jsonDocument["Adresse_physique"];
  String adresseDeGroupe = jsonDocument["Adresse_de_Groupe"];
  int priorite = jsonDocument["Priorite"];
  int taillePayload = jsonDocument["Taille_de_la_Payload"];

  // Afficher les valeurs reçues sur le moniteur série
  Serial.println("Données reçues depuis MQTT :");
  Serial.print("Valeur: ");
  Serial.println(Valeur);
  Serial.print("Adresse de groupe : ");
  Serial.println(adresseDeGroupe);

  // Envoyer les données à la maquette via la connexion UART
  // Vérification des événements KNX
  knx.groupWriteBool(adresseDeGroupe, Valeur);
  toSend = !toSend;
}

void loop() {

  Server.handleClient();

  if (configSuccess) {
    Serial.println("Configuration réussie");
    Serial.println("Redemmarage de l'ESP");
    delay(3000);
    ESP.restart();
  }
  

  while(WiFi.status() == WL_CONNECTED){
    
    client.loop();     

    // read the state of the switch/button:
    currentState = digitalRead(BUTTON_PIN);

    if(lastState == HIGH && currentState == LOW)        // button is pressed
        pressedTime = millis();
    else if(lastState == LOW && currentState == HIGH) { // button is released
        releasedTime = millis();
        long pressDuration = releasedTime - pressedTime;

        if( pressDuration > LONG_PRESS_TIME )
          Serial.println("A long press is detected");
          deleteConfig();
    }

  // save the the last state
  lastState = currentState;
      
    // Utilisation du tableau d'adresses KNX
    for (int i = 0; i < tailleTableau; ++i) {
      Serial.print("Adresse ");
      Serial.print(i + 1);
      Serial.print(" : ");
      Serial.println(tableauAdresses[i]);

      String addressString = String(tableauAdresses[i]);
      addressString.trim();
      strcpy(tableauAdresses[i], addressString.c_str());

      //Envoie à knx 
      knx.groupWriteBool(tableauAdresses[i], toSend);
      toSend = !toSend;
    }
    
    KnxTpUartSerialEventType eType = knx.serialEvent();

    if (eType == TPUART_RESET_INDICATION) {
      Serial.println("RESET KNX");
    }

    // Configuration de la fonction callback pour MQTT
    client.setCallback(callback);

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

      //Recupération de la payload
      bool payload = telegram->getBool();

      //Envoie des données en format ArduinoJson
      jsonDocument.clear();      //Réinitialiser le document JSON avant de l'utiliser à nouveau

      jsonDocument["Valeur"] = payload;
      jsonDocument["Adresse physique"] = sender;
      jsonDocument["Adresse de Groupe"] = target;
      jsonDocument["Priorité"] = priority;
      jsonDocument["Taille de la Payload"] = payloadLength;

      //Convertir l'objet JSON en chaîne JSON
      String jsonString;
      ArduinoJson::serializeJsonPretty(jsonDocument, jsonString); // Utilise serializeJsonPretty pour une sortie formatée

      if (client.connected()) {
        client.publish("knx_envoi", jsonString.c_str());
      }
    }

    if (eType == IRRELEVANT_KNX_TELEGRAM) {
      Serial.println("TELEGRAMME NON PERTINENT");
    }

    if (eType == UNKNOWN_EVENT) {
      Serial.println("EVENEMENT INCONNU");
    }
    delay(3000);
  }
}
