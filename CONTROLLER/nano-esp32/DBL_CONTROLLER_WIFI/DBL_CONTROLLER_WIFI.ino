#include <STRING.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include "FS.h"
#include <FFat.h>

//*************** display ********************************
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//activation du display (si display présent --> true sinon false)
bool displayOn = true;
//**********************************************************

#define DEBUG 0

#if DEBUG
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
  #define serialBegin(x) Serial.begin(115200)
#else
  #define debug(x) 
  #define debugln(x)
  #define serialBegin(x) 
#endif



// Configuration réseau
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x26, 0x35 };
IPAddress ip(192, 168, 100, 10); // Adresse IP du récepteur
IPAddress multicastIP(239, 0, 0, 255); // Adresse multicast
unsigned int multicastPort = 5555;   // Port multicast

WiFiUDP udp;

const char *ssid = "DBL";
const char *password = "DBLINSTRUMENT";
WebServer server(80);

const int chipSelect = 4;
File root;

int scale;
int chord;
String dblFileName;
int bpmTime = 1000;

String linesDBL[100];
int nLignes;
int ligneActive=0;

bool initialisation = true;


/////////////////////////////////////////////////////////////////////////
///////// Opération sur les fichiers
/////////////////////////////////////////////////////////////////////////

String readFile(const char *path) {
    Serial.printf("Reading file: %s\r\n", path);
    String content = "";

    File file = FFat.open(path);
    if (!file || file.isDirectory()) {
        debugln("- failed to open file for reading");
        return "";  // Retourne une chaîne vide en cas d'erreur
    }

    debugln("- read from file:");
    while (file.available()) {
        content += (char)file.read();
    }

    file.close();
    return content;
}


int getLignesNumber(String fileName) {
    if (!FFat.begin(true)) {  // Monte FFat avec formatage automatique si nécessaire
        Serial.println("Échec du montage de FFat");
        return -1;
    }

    File file = FFat.open(fileName, "r");
    if (!file) {
        Serial.println("Échec de l'ouverture du fichier");
        return -1;
    }

    int lineCount = 0;
    while (file.available()) {
        if (file.read() == '\n') {
            lineCount++;
        }
    }

    file.close();
    return lineCount;
}

/////////////////////////////////////////////////////////////////////////
///////// RECUPERATION D'UNE LIGNE ENTIERE
/////////////////////////////////////////////////////////////////////////

String getLigneString(String fileName, int nLigne) {
  if (nLigne < 0) {
    debugln("Numéro de ligne invalide !");
    return "";
  }

  File file = FFat.open(fileName, "r"); // Ouvre le fichier en lecture
  if (!file) {
    debugln("Erreur d'ouverture du fichier !");
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
  debugln("Ligne demandée non trouvée !");
  file.close(); // Fermer le fichier
  return "";
}


// récupère le fichier DBL et place les lignes dans linesDBL[i-1]
// récupère auddi le bpm

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
          debug("bpmTime : ");debugln(bpmTime);
      } else {
          bpmTime = 1000;
      }


  } else { // lignes harmonie et dbl
  linesDBL[i-1] = linetemp;
  }

  

}

nLignes--;// on doit compter les lignes d'harmonie et exclure la première ligne tempo ('bpm=120')


}



//*******************************************************************
//****** Fonction wifi pour la gestion du serveur web 
//*******************************************************************

void handleRoot()
{
    if (server.hasArg("file") ) {
      debugln(server.arg("file"));
    }
    
    String page = readFile("/index.html");

    server.setContentLength(page.length());
    server.send(200, "text/html", page);
}

void handlejQuery(){

    String page = readFile("/zepto.min.js");

    server.setContentLength(page.length());
    server.send(200, "text/javascript", page);  

}

void handleDBL(){
    String path = server.uri();
    
     if ((path.endsWith(".dbl")) or (path.startsWith("/DBL"))) {
    
    String page = readFile(path.c_str());
    //String page = readFile("/DBL.TXT");

    server.setContentLength(page.length());
    server.send(200, "text/plain", page);  
     }

}



void handleSubmit() {
    if (server.hasArg("Harmony") ) {
        String Harmony = server.arg("Harmony");


  //****************************************************  
    String filename = "/DBL.TXT";    
    File file = FFat.open(filename, "w");
    if (!file) {
        debugln("Erreur d'ouverture du fichier dans handleSubmit.");
        return;
    }
    file.print(Harmony);  
    file.close();

    debugln("Fichier DBL.TXT écrit avec succès !");
    getFileDbl("/DBL.TXT");
  //****************************************************  

        String response = "<h2>OK</h2>";
        response += "<a href='/'>Retour</a>";
        
        server.send(200, "text/html", response);
    } else {
        server.send(400, "text/plain", "Erreur : données manquantes.");
    }
}




bool screenEnabled = false;


void setup() {
  pinMode(7, INPUT_PULLUP);// PIN 7 -> 1-> webserver mode 0-> multicast mode 
 
  serialBegin(115200);
  delay(1000);

//-------init display 
  if (displayOn){
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      debugln(F("SSD1306 allocation failed"));
      //for(;;); // Don't proceed, loop forever
    } else {
    debugln(F("Display OK"));
    display.clearDisplay();
    screenEnabled = true;
    }
  }

//----- init PSIFFS

  if (!FFat.begin(true)) {
    debugln("Erreur FFat");
    return;
  }



  dblFileName = "/DBL.TXT";
  getFileDbl(dblFileName);

//----- init HOTSPOT

  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1);
  }

//---init multicast 

  IPAddress myIP = WiFi.softAPIP();
  debug("AP IP address: ");
  debugln(myIP);
  udp.beginMulticast(multicastIP, multicastPort);


//------ init server web ------------------

  server.on("/", handleRoot);
  server.on("/zepto.min.js", handlejQuery);
  server.on("/write.htm", HTTP_POST, handleSubmit);
  server.onNotFound(handleDBL);

  server.begin();

  
}


unsigned long mesureTimeSync = millis();
float mesureTimePercent;
float mesureTimeStatePercent = 0.0;


void loop() {


  // si le piN 8 est débranché on gère l'envoi du flux multicast 
  if (digitalRead(7)){
  
  mesureTimePercent = 100*(millis()-mesureTimeSync)/bpmTime;

  if (initialisation==false){
    String payload = linesDBL[ligneActive];

    if (millis()-mesureTimeSync>bpmTime){  
      udp.beginPacket(multicastIP, multicastPort);
      udp.write((const uint8_t*)payload.c_str(), payload.length());
      udp.endPacket();
      // envoyer le paquet immédiatement
      udp.flush();

      debug("/");
      infoDisplay(payload, " /");

      ligneActive++;

      if (ligneActive>=nLignes){
        ligneActive =0;
        debugln("");
      }
      mesureTimeSync = millis();
      mesureTimeStatePercent=0.0;

    } else if ((mesureTimePercent>50.0) and (mesureTimeStatePercent!=50.0)){

      // on définit des actions tout au long de la mesure
      // par exemple >50 c'est au milieu de la mesure 
      
      debug("\\");
      infoDisplay(payload, "\\ ");
      mesureTimeStatePercent=50.0;

      String bpmInit = String(bpmTime); // Conversion en String
      udp.beginPacket(multicastIP, multicastPort);
      udp.write((const uint8_t*)bpmInit.c_str(), bpmInit.length()); // Utilisation correcte
      udp.endPacket();
      udp.flush();


    } 

  } else {

      // envoi de 4 paquets d'initialisation  
      // ils contiennent le bpm en format string

      String bpmInit = String(bpmTime); // Conversion en String
      udp.beginPacket(multicastIP, multicastPort);
      udp.write((const uint8_t*)bpmInit.c_str(), bpmInit.length()); // Utilisation correcte
      udp.endPacket();
      udp.flush();

      debugln("init");

      preCountDisplay("/ ", 4);
      delay(bpmTime);
    

      udp.beginPacket(multicastIP, multicastPort);
      udp.write((const uint8_t*)bpmInit.c_str(), bpmInit.length()); // Utilisation correcte
      udp.endPacket();
      udp.flush();

      debugln("init");

      preCountDisplay("- ", 3);
      delay(bpmTime);

      udp.beginPacket(multicastIP, multicastPort);
      udp.write((const uint8_t*)bpmInit.c_str(), bpmInit.length()); // Utilisation correcte
      udp.endPacket();
      udp.flush();
      
      debugln("init");

      preCountDisplay("\\ ", 2);
      delay(bpmTime);


      udp.beginPacket(multicastIP, multicastPort);
      udp.write((const uint8_t*)bpmInit.c_str(), bpmInit.length()); // Utilisation correcte
      udp.endPacket();
      udp.flush();

      debugln("init");

      preCountDisplay("|", 1);
      delay(bpmTime);


      initialisation=false;
      mesureTimeSync = millis();

  }



} else {
// si le piN 8 est branché on gère le serveur web 
//======================================================
/// utilisatyion server web 
//======================================================

    displayWeb();

    initialisation=true; // à la reconnexion on enverra 4 paquets vides 
    ligneActive = 0;
    server.handleClient();
}





}



//************************************************************************
// display 
//************************************************************************

void displayWeb(){

  if ((screenEnabled==true)and (displayOn)){
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


void preCountDisplay(String curs, int mesureCount){

  if ((screenEnabled==true)and (displayOn)){
 

    display.clearDisplay();

    display.drawLine(0,15,SCREEN_WIDTH,15, WHITE);
    display.drawLine(0,SCREEN_HEIGHT-2,SCREEN_WIDTH,SCREEN_HEIGHT-2, WHITE);
    display.setTextColor(WHITE);

    display.setCursor(SCREEN_WIDTH-30, 6);

    display.setTextSize(1);

    //display.setFont(&FreeMono9pt7b);
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.print(curs);
    display.setTextSize(3);
    display.print(mesureCount);
    display.setTextSize(3);
    display.println("");

    display.setTextSize(1);
    display.println("Starting Song");
    display.setTextSize(1);
    display.println("");
    display.display();
  }
}


void infoDisplay(String dblLine, String curs){

  if ((screenEnabled==true)and (displayOn)){
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