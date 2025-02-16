#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet3.h>
#include <EthernetUdp3.h>

#include <SPI.h>



// Configuration réseau
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x26, 0x34 };
IPAddress ip(192, 168, 100, 2); // Adresse IP du récepteur
IPAddress multicastIP(239, 0, 0, 255); // Adresse multicast
unsigned int multicastPort = 5555;   // Port multicast

int glob;

EthernetUDP udp;
char packetBuffer[20]; // Tampon pour stocker les données UDP reçues

void initNetwork(){
  Ethernet.begin(mac, ip);
 
  udp.beginMulticast(multicastIP, multicastPort);
  Serial.println("Récepteur prêt.");
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
