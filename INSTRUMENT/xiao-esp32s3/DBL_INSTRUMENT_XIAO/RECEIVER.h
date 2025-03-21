#include <WiFi.h>
#include <WiFiUdp.h>

// Configuration réseau
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x26, 0x34 };
IPAddress ip(192, 168, 100, 2); // Adresse IP du récepteur
IPAddress multicastIP(239, 0, 0, 255); // Adresse multicast
unsigned int multicastPort = 5555;   // Port multicast

int glob;

WiFiUDP udp;
char packetBuffer[20]; // Tampon pour stocker les données UDP reçues

const char* ssid = "DBL";
const char* password = "DBLINSTRUMENT";


void initNetwork(){

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
 
    // Démarrer UDP et rejoindre le groupe multicast
    if (udp.beginMulticast(multicastIP, multicastPort)) {
        Serial.println("Inscription au groupe multicast réussie !");
    } else {
        Serial.println("Erreur lors de l'inscription au groupe multicast.");
    }
}

void receiveHarmony(){

  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Lire le message UDP
    int len = udp.read(packetBuffer, 20);
    if (len > 0) {
      packetBuffer[len] = '\0'; // Null-terminate la chaîne
    }

    String message = String(packetBuffer);
    int separatorIndex = message.indexOf(';');

    if (separatorIndex != -1) {
      int number = message.substring(0, separatorIndex).toInt();
      glob = number;
      String text = message.substring(separatorIndex + 1);
      /*
      Serial.print("Nombre : ");
      Serial.println(number);
      Serial.print("Texte : ");
      Serial.println(text);
      */
    }

  }

}
