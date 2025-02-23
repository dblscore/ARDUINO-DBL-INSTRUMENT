#include <SPI.h>
#include <SD.h>
#include <STRING.h>
#include <Ethernet3.h>
#include <EthernetUdp3.h>

//*************** display ********************************
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//**********************************************************

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
    return -1;
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

bool screenEnabled = false;
void setup() {
  pinMode(8, INPUT);// PIN 8 -> 1-> webserver mode 0-> multicast mode 
 
  Serial.begin(115200);
  while (!Serial);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  } else {
  Serial.println(F("Display OK"));
  display.clearDisplay();
  screenEnabled = true;
  }



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

  infoDisplay(payload, " /");

  delay(bpmTime/2);

  infoDisplay(payload, "\\ ");

  delay(bpmTime/2);

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

    displayWeb();

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


//************************************************************************
// display 
//************************************************************************

void displayWeb(){

  if (screenEnabled==true){
    display.clearDisplay();

    display.setTextColor(WHITE);
    //display.setFont(&FreeMono9pt7b);
    display.setCursor(0, 10);
    display.setTextSize(3);
    display.println("Setting");
    display.setTextSize(2);
    display.println("Web Server");
    
    display.setTextSize(1);
    display.println("port 80");
    display.display();
  }
}


void infoDisplay(String dblLine, String curs){

  if (screenEnabled==true){
    int chInt,scInt;
    String dblSymbols;

    chInt = getChord(dblLine);
    scInt = getScale(dblLine);
    dblSymbols = getDBL(dblLine);
    

    display.clearDisplay();

    display.drawLine(0,15,SCREEN_WIDTH,15, WHITE);
    display.drawLine(0,SCREEN_HEIGHT-2,SCREEN_WIDTH,SCREEN_HEIGHT-2, WHITE);
    display.setTextColor(WHITE);

    display.setCursor(SCREEN_WIDTH-30, 6);

    display.setTextSize(1);
    display.print(ligneActive+1);
    display.print("/");
    display.print(nLignes);


    //display.setFont(&FreeMono9pt7b);
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.print(curs);
    display.setTextSize(3);
    display.print(getChord(chInt));
    display.setTextSize(3);
    display.println("");

    display.setTextSize(1);
    display.println(getScaleLabel(scInt));
    display.setTextSize(1);
    display.println(dblSymbols);
    display.display();
  }
}

String getChord(int accInt) {
    switch (accInt) {
        case 2192: return "C";
        case 2320: return "Cm";
        case 1096: return "C#";
        case 1160: return "C#m";
        case 548:  return "D";
        case 580:  return "Dm";
        case 274:  return "D#";
        case 290:  return "D#m";
        case 137:  return "E";
        case 145:  return "Em";
        case 2116: return "F";
        case 2120: return "Fm";
        case 1058: return "F#";
        case 1060: return "F#m";
        case 529:  return "G";
        case 530:  return "Gm";
        case 2312: return "G#";
        case 265:  return "G#m";
        case 1156: return "A";
        case 2180: return "Am";
        case 578:  return "A#";
        case 1090: return "A#m";
        case 289:  return "B";
        case 545:  return "Bm";
        case 2336: return "Cdim";
        case 1168: return "C#dim";
        case 584:  return "Ddim";
        case 292:  return "D#dim";
        case 146:  return "Edim";
        case 73:   return "Fdim";
        case 2084: return "F#dim";
        case 1042: return "Gdim";
        case 521:  return "G#dim";
        case 2308: return "Adim";
        case 1154: return "A#dim";
        case 577:  return "Bdim";
        default:   return "Unknown";
    }
}




String getScaleLabel(int scaleInt) {
    switch (scaleInt) {
		case 2773 : return "C major / A minor";
		case 3434 : return "C# major / A# minor";
		case 1717 : return "D major / B minor";
		case 2906 : return "D# major / C minor";
		case 1453 : return "E major / C# minor";
		case 2774 : return "F major / D minor";
		case 1387 : return "F# major / D# minor";
		case 2741 : return "G major / E minor";
		case 3418 : return "G# major / F minor";
		case 1709 : return "A major / F# minor";
		case 2902 : return "Bb major / G minor";
		case 1451 : return "B major / G# minor";
		case 2901 : return "C melodic minor";
		case 3498 : return "C# melodic minor";
		case 1749 : return "D melodic minor";
		case 2922 : return "D# melodic minor";
		case 1461 : return "E melodic minor";
		case 2778 : return "F melodic minor";
		case 1389 : return "F# melodic minor";
		case 2742 : return "G melodic minor";
		case 1371 : return "G# melodic minor";
		case 2733 : return "A melodic minor";
		case 3414 : return "Bb melodic minor";
		case 1707 : return "B melodic minor";
		case 2905 : return "C harmonic minor";
		case 3500 : return "C# harmonic minor";
		case 1750 : return "D harmonic minor";
		case 875  : return "D# harmonic minor";
		case 2485 : return "E harmonic minor";
		case 3290 : return "F harmonic minor";
		case 1645 : return "F# harmonic minor";
		case 2870 : return "G harmonic minor";
		case 1435 : return "G# harmonic minor";
		case 2765 : return "A harmonic minor";
		case 3430 : return "Bb harmonic minor";
		case 1715 : return "B harmonic minor";
		case 1718 : return "D harmonic major";
		case 859  : return "D# harmonic major";
		case 1459 : return "B harmonic major";
		case 1643 : return "F# harmonic major";
		case 1741 : return "A harmonic major";
		case 2477 : return "E harmonic major";
		case 2777 : return "C harmonic major";
		case 2869 : return "G harmonic major";
		case 2918 : return "A# harmonic major";
		case 3286 : return "F harmonic major";
		case 3436 : return "C# harmonic major";
		case 3482 : return "G# harmonic major";
    default:   return "Unknown";
    }
}



int getScale(String input) {
    int firstSeparator = input.indexOf(';'); // Trouver le premier ';'
    if (firstSeparator == -1) {
        return 0; // Retourner la chaîne entière si aucun séparateur trouvé
    }

    return  input.substring(0, firstSeparator).toInt();// Extraire la partie avant le premier ';'
}

int getChord(String input) {
    int firstSeparator = input.indexOf(';');  // Trouver le premier ';'
    if (firstSeparator == -1) {
        return 0; // Pas de deuxième élément
    }
    int secondSeparator = input.indexOf(';', firstSeparator + 1); // Trouver le second ';'
    if (secondSeparator == -1) {
        return input.substring(firstSeparator + 1).toInt(); // Retourner le reste s'il n'y a que deux éléments
    }

    return input.substring(firstSeparator + 1, secondSeparator).toInt(); // Extraire le deuxième élément
}

String getDBL(String input) {
    int lastSeparator = input.lastIndexOf(';'); // Trouver la dernière occurrence de ';'
    if (lastSeparator == -1) {
        return input; // Retourner la chaîne entière si aucun séparateur trouvé
    }
    return input.substring(lastSeparator + 1); // Extraire la sous-chaîne après le dernier ';'
}