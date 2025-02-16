#include <SPI.h>
#include <SD.h>
#include <STRING.h>
#include <Ethernet3.h>
#include <EthernetUdp3.h>

// Configuration réseau
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x26, 0x35 };
IPAddress ip(192, 168, 100, 10); // Adresse IP du récepteur
IPAddress multicastIP(239, 0, 0, 255); // Adresse multicast
unsigned int multicastPort = 5555;   // Port multicast

EthernetUDP udp;


const int chipSelect = 4;
File root;

EthernetServer server(80);

int scale;
int chord;
String dblFileName;
int bpmTime = 1000;

String linesDBL[100];
int nLignes;
int ligneActive=0;

bool initialisation = true;
/////////////////////////////////////////////////////////////////////////
///////// RECUPERATION DE SCALE ET CHORD EN INT (PAS FONCTIONNELLE)
/////////////////////////////////////////////////////////////////////////

void getLigneInt(String fileName, int nLigne) {
  File file = SD.open(fileName); // Ouvre le fichier en lecture
  if (!file) {
    Serial.println("Erreur d'ouverture du fichier !");
    return;
  }

  int currentLine = 0; // Compteur de lignes
  String lineContent;

  while (file.available()) {
    lineContent = file.readStringUntil('\n'); // Lire jusqu'à la fin de la ligne

    if (currentLine == nLigne) { 
      // Extraire les données de la ligne
      int separatorIndex = lineContent.indexOf(';');
      if (separatorIndex != -1) {
        scale = lineContent.substring(0, separatorIndex).toInt(); // Convertir en entier
        chord = lineContent.substring(separatorIndex + 1).toInt(); // Enlever les espaces
        Serial.println("dans la fonction");
        Serial.println(scale);
        Serial.println(chord);
      } else {
        Serial.println("Erreur de format dans la ligne.");
      }
      break;
    }
    currentLine++;
  }

  if (currentLine != nLigne) {
    Serial.println("Ligne demandée non trouvée !");
  }

  file.close(); // Fermer le fichier
}




/////////////////////////////////////////////////////////////////////////
///////// RECUPERATION nombre de lignes
/////////////////////////////////////////////////////////////////////////



int getLignesNumber(String fileName) {
  File file = SD.open(fileName); // Ouvre le fichier en lecture
  if (!file) {
    Serial.println("Erreur d'ouverture du fichier !");
    return;
  }

  int currentLine = 0; // Compteur de lignes
  String lineContent;

  while (file.available()) {
    lineContent = file.readStringUntil('\n');
    currentLine++;

  }
return currentLine;
}

/////////////////////////////////////////////////////////////////////////
///////// RECUPERATION D'UNE LIGNE ENTIERE
/////////////////////////////////////////////////////////////////////////

String getLigneString(String fileName, int nLigne) {
  if (nLigne < 0) {
    Serial.println("Numéro de ligne invalide !");
    return "";
  }

  File file = SD.open(fileName); // Ouvre le fichier en lecture
  if (!file) {
    Serial.println("Erreur d'ouverture du fichier !");
    return "";
  }

  int currentLine = 0; // Compteur de lignes
  String lineContent = "";

  while (file.available()) {
    lineContent = file.readStringUntil('\n'); // Lire jusqu'à la fin de la ligne
    if (currentLine == nLigne) { 
      file.close();
      return lineContent; // Retourne directement si la ligne est trouvée
    }
    currentLine++;
  }

  // Si la ligne demandée n'est pas trouvée
  Serial.println("Ligne demandée non trouvée !");
  file.close(); // Fermer le fichier
  return "";
}


///// decoder la donnée de la variable POST
String urlDecode(String input) {
    String decoded = "";
    char temp[3] = {0}; // Tampon pour stocker les caractères hexadécimaux
    
    for (unsigned int i = 0; i < input.length(); i++) {
        if (input[i] == '+') {
            decoded += ' '; // Les espaces sont encodés en +
        } else if (input[i] == '%' && i + 2 < input.length()) {
            temp[0] = input[i + 1];
            temp[1] = input[i + 2];
            decoded += (char) strtol(temp, NULL, 16); // Convertit %XX en caractère réel
            i += 2;
        } else {
            decoded += input[i];
        }
    }
    return decoded;
}



void getFileDbl(String filedbl){

nLignes = getLignesNumber(filedbl);
for(int i=0;i<nLignes;i++){
  
  String linetemp = getLigneString(filedbl, i);
  linetemp.trim();

  if (i==0){ ///ligne bpm => bpm=60
      int pos = linetemp.indexOf('=');

      if (pos != -1) { // Vérifier si "=" existe
          String valueStr = linetemp.substring(pos + 1); 
          bpmTime = valueStr.toInt(); // Convertir en entier
          bpmTime = 120000/bpmTime;
          Serial.println(bpmTime);
      } else {
          bpmTime = 1000;
      }


  } else { // lignes harmonie et dbl
  linesDBL[i-1] = linetemp;
  }

  

}

nLignes--;// on doit compter les lignes d'harmonie et exclure la première ligne tempo ('bpm=120')


}


void setup() {
  pinMode(8, INPUT);// PIN 8 -> 1-> webserver mode 0-> multicast mode 
 
  Serial.begin(9600);
  while (!Serial);

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
    while (true);
  }

  Serial.println("initialization done.");

  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println("done!");

  dblFileName = "dbl.txt";
  File file = SD.open(dblFileName);

  if (!file) {
    Serial.println("Erreur d'ouverture du fichier !");
    return;
  } else {
    Serial.println("fichier ouvert !");
  }




  getFileDbl(dblFileName);

  Ethernet.begin(mac, ip);
  server.begin();//server web

  /*
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Shield Ethernet non détecté");
    while (true);
  }
  
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Pas de connexion Ethernet détectée");
  }
  */

  // Initialisation UDP
  udp.beginMulticast(multicastIP, multicastPort);
  Serial.println("Émetteur prêt.");


  
}

void loop() {


  // si le piN 8 est débranché on gère l'envoi du flux multicast 
  if (digitalRead(8)==0){

  if (initialisation==false){
  Serial.println(digitalRead(8));
  //String payload = getLigneString(dblFileName, ligneActive);//linesDBL[ligneActive];

  String payload = linesDBL[ligneActive];
  //Serial.println(payload);
  udp.beginPacket(multicastIP, multicastPort);

  //Serial.println(payload.length());
  udp.write(payload.c_str(), payload.length());//print(payload);
  udp.endPacket();

  delay(bpmTime);
  ligneActive++;

  if (ligneActive>=nLignes){
    ligneActive =0;
  }

  } else {

  // envoi de 4 paquets d'initialisation  
  udp.beginPacket(multicastIP, multicastPort);
  udp.write("init", 4);//print(payload);
  udp.endPacket();
  delay(bpmTime);

  udp.beginPacket(multicastIP, multicastPort);
  udp.write("init", 4);//print(payload);
  udp.endPacket();
  delay(bpmTime);

  udp.beginPacket(multicastIP, multicastPort);
  udp.write("init", 4);//print(payload);
  udp.endPacket();
  delay(bpmTime);

  udp.beginPacket(multicastIP, multicastPort);
  udp.write("init", 4);//print(payload);
  udp.endPacket();
  delay(bpmTime);


  initialisation=false;
  }



} else {
// si le piN 8 est branché on gère le serveur web 
//======================================================
/// utilisatyion server web 
//======================================================
    initialisation=true; // à la reconnexion on enverra 4 paquets vides 
    ligneActive = 0;
    EthernetClient client = server.available();
    
    if (client) {
        Serial.println("Client connecté.");
        String request = "";
        bool isPost = false;
        String postData = "";

        while (client.available()) {
            char c = client.read();
            request += c;

            // Détection de la méthode HTTP
            if (request.indexOf("POST /write.htm") != -1) {
                isPost = true;
            }

            // Fin des headers HTTP
            if (request.endsWith("\r\n\r\n")) {
                break;
            }
        }

        // Gestion du POST : récupération des données
        if (isPost) {
            while (client.available()) {
                char c = client.read();
                postData += c;
            }
            
            Serial.print("Données POST reçues : ");
            Serial.println(postData);

            // Extraction de la variable "Harmony"
            String harmonyValue = "";
            int index = postData.indexOf("Harmony=");
            if (index != -1) {
                harmonyValue = postData.substring(index + 8);
                harmonyValue.trim();
                harmonyValue.replace("+", " ");  // Remplace les "+" par des espaces
                Serial.print("Valeur de Harmony : ");
                Serial.println(urlDecode(harmonyValue));

                if (SD.exists("/dbl.txt")) {
                    SD.remove("/dbl.txt");
                }

                // Écriture dans le fichier dbl.txt
                File file = SD.open("/dbl.txt", FILE_WRITE);
                if (file) {
                    file.print(urlDecode(harmonyValue));
                    file.close();
                    Serial.println("Fichier dbl.txt mis à jour !");
                    getFileDbl("dbl.txt");
                } else {
                    Serial.println("Erreur lors de l'ouverture du fichier dbl.txt !");
                }
            }

            // Réponse HTTP au client
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("Données reçues !");
        }
        // Gestion des requêtes GET pour servir les fichiers depuis la carte SD
        else {
            String filename = "/index.htm"; // Fichier par défaut
            if (request.indexOf("GET / ") != -1) {
                filename = "/index.htm";
            } else if (request.indexOf("GET /") != -1) {
                int start = request.indexOf("GET /") + 5;
                int end = request.indexOf(" ", start);
                filename = "/" + request.substring(start, end);
            }

            Serial.print("Demande du fichier : ");
            Serial.println(filename);

            if (SD.exists(filename)) {
                File file = SD.open(filename);
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();
                
                while (file.available()) {
                    client.write(file.read());
                }
                file.close();
            } else {
                client.println("HTTP/1.1 404 Not Found");
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();
                client.println("<h1>404 - Fichier non trouvé</h1>");
            }
        }

        delay(10);
        client.stop();
        Serial.println("Client déconnecté.");
    }
}





}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
