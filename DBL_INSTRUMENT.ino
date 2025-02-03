#include <MIDI.h>
#include <MIDIUSB.h>
#include "RECEIVER.h"

#define DEBUG 0

#if DEBUG
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debug(x) 
  #define debugln(x)
#endif


#define LED 13

MIDI_CREATE_DEFAULT_INSTANCE();

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//initiation
/** USAGE 
NotePrec notePrec;
    
    notePrec.set(51, 72); //quand on appuis sur la note 51 on obtient 

    // Lire la valeur de notePrec[51]
    int value = notePrec.get(51);
    Serial.println(value); // Affichera 72
**/
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int gamme = 2773;
int* gammeTab;
int accord = 2192;
int* accordTab;
int tonica;

bool modeChords = false;
int chordNoteNbr=1;

bool modeFree = false;


/// dbl
unsigned long precMillis = 0; // 0 pour initialisation, la première fois qu'on recois un paquet precMillis = millis()
unsigned long timeMeasure = 0; // 0 pour initialisation, puis timeMeasure sera millis()- precMillis
unsigned long timeBit;
int lenDblMeasure;
String dblMeasure;
int indexDbl = 0;

unsigned long precMillisNote;

// velocity

byte velocityAverage=80;
float alpha = 0.3; // Facteur d'atténuation (entre 0 et 1)


//********************************************************
// gestion des notes précédentes
//********************************************************

class NotePrec {
private:
    int notaPrec[23]; // Plage de 49 à 71 -> 23 valeurs (71 - 49 + 1)
public:
    // Constructeur pour initialiser les valeurs si nécessaire
    NoteManager() {
        for (int i = 0; i < 23; i++) {
            notaPrec[i] = 0; // Initialisation à 0
        }
    }

    // Méthode pour définir une valeur
    void set(int noteClavier, int noteTranspose) {
        
        // les notes jouées avec le clavier vont de 48 à 64
        if (noteClavier >= 49 && noteClavier <= 71) {
            notaPrec[noteClavier - 49] = noteTranspose; // Ajustement de l'index
        } 
    }

    // Méthode pour récupérer une valeur
    int get(int noteClavier) {
      // les notes jouées avec le clavier vont de 48 à 64
        if (noteClavier >= 49 && noteClavier <= 71) {
            return notaPrec[noteClavier - 49]; // Ajustement de l'index
        } 
    }
};


//********* initiation *********************
int nota = 59;
int pitchPrec = 60;
int notaChord = 47;
int notaDBL = 35;



NotePrec notePrec;
NotePrec notePrec_3;
NotePrec notePrec_5;

NotePrec notePrecChord;
NotePrec notePrecChord_3;
NotePrec notePrecChord_5;

NotePrec notePrecDBL;


//***************************************************************
//***************************************************************
//******************** scales ***********************************
//***************************************************************
//***************************************************************

/*
2773 --> C major / A minor
3434 --> C# major / A# minor
1717 --> D major / B minor
2906 --> D# major / C minor
1453 --> E major / C# minor
2774 --> F major / D minor
1387 --> F# major / D# minor
2741 --> G major / E minor
3418 --> G# major / F minor
1709 --> A major / F# minor
2902 --> Bb major / G minor
1451 --> B major / G# minor
2901 --> C melodic minor
3498 --> C# melodic minor
1749 --> D melodic minor
2922 --> D# melodic minor
1461 --> E melodic minor
2778 --> F melodic minor
1389 --> F# melodic minor
2742 --> G melodic minor
1371 --> G# melodic minor
2733 --> A melodic minor
3414 --> Bb melodic minor
1707 --> B melodic minor
2905 --> C harmonic minor
3500 --> C# harmonic minor
1750 --> D harmonic minor
875 --> D# harmonic minor
2485 --> E harmonic minor
3290 --> F harmonic minor
1645 --> F# harmonic minor
2870 --> G harmonic minor
1435 --> G# harmonic minor
2765 --> A harmonic minor
3430 --> Bb harmonic minor
1715 --> B harmonic minor
1718 --> D harmonic major
859 --> D# harmonic major
1459 --> B harmonic major
1643 --> F# harmonic major
1741 --> A harmonic major
2477 --> E harmonic major
2777 --> C harmonic major
2869 --> G harmonic major
2918 --> A# harmonic major
3286 --> F harmonic major
3436 --> C# harmonic major
3482 --> G# harmonic major
*/

///////////////////////////////////////////////////////////////////////////////
/// fonction qui récupère par exemple 2773 et qui renvoi un array des 7 notes
/// de la gamme de Do Majeur 
///////////////////////////////////////////////////////////////////////////////

int* gammeIntToArray(int number) {
  static int result[7]; // Tableau pour stocker les résultats
  int index = 0;

  // Parcours des 12 bits (comme le nombre est en binaire sur 12 bits)
  for (int i = 0; i < 12; i++) {
    if ((number >> (11 - i)) & 1) {  // Vérifie si le bit est à 1
      result[index] = i;  
      index++;
    }
  }
  
  return result;
}

//////////////////////////////////////////////////////////////////////
/// fonction qui récupère 2192 et qui renvoi un array des 3 notes
/// de l'accord de Do Majeur 
//////////////////////////////////////////////////////////////////////
int* accordIntToArray(int number) {
  static int result[3]; // Tableau pour stocker les résultats
  int index = 0;

  // Parcours des 12 bits (comme le nombre est en binaire sur 12 bits)
  for (int i = 0; i < 12; i++) {
    if ((number >> (11 - i)) & 1) {  // Vérifie si le bit est à 1
      result[index] = i;  // Ajouter la position sans la multiplication
      index++;
    }
  }
  
  return result;
}

//***************************************************************
//***************************************************************
//******************** chords ***********************************
//***************************************************************
//***************************************************************

/*
2192 --> C
2320 --> Cm
1096 --> C#
1160 --> C#m
548 --> D
580 --> Dm
274 --> D#
290 --> D#m
137 --> E
145 --> Em
2116 --> F
2120 --> Fm
1058 --> F#
1060 --> F#m
529 --> G
530 --> Gm
2312 --> G#
265 --> G#m
1156 --> A
2180 --> Am
578 --> A#
1090 --> A#m
289 --> B
545 --> Bm
2336 --> Cdim
1168 --> C#dim
584 --> Ddim
292 --> D#dim
146 --> Edim
73 --> Fdim
2084 --> F#dim
1042 --> Gdim
521 --> G#dim
2308 --> Adim
1154 --> A#dim
577 --> Bdim

*/


//****************************************************************************************************
// midi usb

void noteOnUSB(byte channel, byte pitch, byte velocity) {
  velocityAverage = (alpha * velocity) + ((1 - alpha) * velocityAverage);
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOffUSB(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}


// les notes en dbl ne sont pas utilisées pour calculer la velocity moyenne
void noteDblOnUSB(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}


//####################################################################################################
// Fonction DBL DECODE
// Fonction générique pour obtenir une note relative, même si la note initiale n'est pas dans la gamme
// il faut associer une note midi à un symbole dbl
//####################################################################################################
int dblDecode(int dbl) {

  if (modeFree==false){

    if (dbl==64){  //note a, /
      return 1;
    }else if (dbl==65){ // a#, /
      return 1;
    }else if (dbl==66){ //b, )
      return 2;
    }else if (dbl==67){ //c, )
      return 2;
    }else if (dbl==68){ //c#, ]
      return 3;
    }else if (dbl==69){ //d, ]
      return 3;
    }else if (dbl==70){ //d#, }
      return 4;
    }else if (dbl==71){ //e, }
      return 4;
    }
    
    else if (dbl==62){ // g#, 
      return 0;
    }  
    
    else if (dbl==60){ // g , 
      return -1;
    }else if (dbl==59){ //f#, 
      return -1;
    }else if (dbl==58){ //f, (
      return -2;
    }else if (dbl==57){ //e, (
      return -2;
    }else if (dbl==56){ //d#, [
      return -3;
    }else if (dbl==55){ //d, [
      return -3;
    }else if (dbl==54){ //c#, {
      return -4;
    }else if (dbl==53){ //c, {
      return -4;
    } 
    
    
    else if ((dbl==61)or(dbl==49)){ //p ou accord
      return -1;
    } else if ((dbl==63)or(dbl==51)){ // n ou accord
      return 1;
    } else if (dbl==50){ // le même accord
      return 0;
    }

    else {
    return -99;
    }



  } else if (modeFree==true){
    debugln("mode free dans dblDecode");

    debugln("nota : ");
    debugln(nota);

    float dblFloat = dbl;
    float pitchPrecFloat = pitchPrec;
    

    float ret = (dblFloat-pitchPrecFloat)/2;

    ret = (ret > 0) ? ceil(ret) : floor(ret);

    debugln("dbl : ");
    debugln(dbl);
    
    debugln("mouvement : ");
    debugln(ret);
    
    
    
    return ret;

  }
 
  

}



//####################################################################################################
// Fonction GAMMES
// Fonction générique pour obtenir une note relative, même si la note initiale n'est pas dans la gamme
//####################################################################################################

int getMidiNote(int currentNote, int mouv) {

int currentNoteBase = currentNote - currentNote%12;
currentNote= currentNote%12;

    gammeTab = gammeIntToArray(gamme);
    
      int size = 7;
      int startIndex = -1;

      // Rechercher l'index exact ou trouver le voisin le plus proche
      for (int j = 0; j < size; j++) {
        if (gammeTab[j] == currentNote) {
          startIndex = j; // La note est dans la gamme
          break;
        } else if (gammeTab[j] > currentNote) {
          startIndex = (mouv >= 0) ? j-1 : j ; // Note supérieure ou inférieure selon la direction
          break;
        }
      }

      // Si aucune note supérieure n'est trouvée, boucle en partant de la fin
      if (startIndex == -1) {
        startIndex = (mouv >= 0) ? 0 : size - 1;
      }

      // Calcul de l'index cible avec gestion des boucles
      int targetIndex = (startIndex + mouv) % size;
      if (targetIndex < 0) {
        targetIndex += size; // Gestion des indices négatifs
      }

      // correction éventuelle de l'octave
      if((currentNote>gammeTab[targetIndex])&&(mouv>0)){
        currentNoteBase+=12;
      } else if((currentNote<gammeTab[targetIndex])&&(mouv<0)){
        currentNoteBase-=12;
      }

      return currentNoteBase + gammeTab[targetIndex]; // Retourne la note cible


  //return -1; // Clé introuvable
}




//####################################################################################################
// Fonction ACCORDS
// Fonction générique pour obtenir une note relative, même si la note initiale n'est pas dans la gamme
//####################################################################################################

int getMidiNoteChord(int currentNote, int mouv) {

int currentNoteBase = currentNote - currentNote%12;
currentNote= currentNote%12;

    accordTab = accordIntToArray(accord);
      int size = 3;
      //debugln(size);
      int startIndex = -1;

      // Rechercher l'index exact ou trouver le voisin le plus proche
      for (int j = 0; j < size; j++) {
        if (accordTab[j] == currentNote) {
          startIndex = j; // La note est dans la gamme
          break;
        } else if (accordTab[j] > currentNote) {
          startIndex = (mouv >= 0) ? j-1 : j ; // Note supérieure ou inférieure selon la direction
          break;
        }
      }

      // Si aucune note supérieure n'est trouvée, boucle en partant de la fin
      if (startIndex == -1) {
        startIndex = (mouv >= 0) ? 0 : size - 1;
      }

      // Calcul de l'index cible avec gestion des boucles
      int targetIndex = (startIndex + mouv) % size;
      if (targetIndex < 0) {
        targetIndex += size; // Gestion des indices négatifs
      }

      // correction éventuelle de l'octave
      if((currentNote>accordTab[targetIndex])&&(mouv>0)){
        currentNoteBase+=12;
      } else if((currentNote<accordTab[targetIndex])&&(mouv<0)){
        currentNoteBase-=12;
      }

      return currentNoteBase + accordTab[targetIndex]; // Retourne la note cible


  return -1; // Clé introuvable
}




//####### fonction d'exécution d'une note ou un accord 

void play(int pitchInt, byte velocity){

     int mouvement = dblDecode(pitchInt);

      if (mouvement!=-99){
        if (modeFree==true){


            nota = getMidiNote(nota, mouvement);
            notePrec.set(pitchInt, nota);
            pitchPrec = pitchInt;
            byte notaByte = static_cast<byte>(nota);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush(); 


        } else {
          if ((pitchInt>=54) and (pitchInt<=71) and (pitchInt!=61) and (pitchInt!=63))  {  // si on a des symbols / ) [...
            nota = getMidiNote(nota, mouvement);
            notePrec.set(pitchInt, nota);
            byte notaByte = static_cast<byte>(nota);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();   
          
            if (modeChords == true){

                    if (chordNoteNbr==2){
                      int notaTemp = getMidiNote(nota, 2);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();    

                    } else if (chordNoteNbr==3){
                      int notaTemp = getMidiNote(nota, 2);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

                      notaTemp = getMidiNote(notaTemp, 2);
                      notePrec_5.set(pitchInt, notaTemp);
                      byte notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();
                  }

            }


          } else if ((pitchInt==61) or (pitchInt==63)){ // si on a : n ou p
            
            nota = getMidiNoteChord(nota, mouvement);

            
            notePrec.set(pitchInt, nota);
            byte notaByte = static_cast<byte>(nota);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();   

            if (modeChords == true){

                      int notaTemp = getMidiNoteChord(nota, 1);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

                      notaTemp = getMidiNoteChord(notaTemp, 1);
                      notePrec_5.set(pitchInt, notaTemp);
                      byte notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

            }


          } else if (pitchInt>=49 and pitchInt<=51){


            notaChord = getMidiNoteChord(notaChord, mouvement);

            
            notePrecChord.set(pitchInt, notaChord);
            byte notaByte = static_cast<byte>(notaChord);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();

            int notaTemp = getMidiNoteChord(notaChord, 1);
            notePrecChord_3.set(pitchInt, notaTemp);
            notaByte = static_cast<byte>(notaTemp);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();

            notaTemp = getMidiNoteChord(notaTemp, 1);
            notePrecChord_5.set(pitchInt, notaTemp);
            notaByte = static_cast<byte>(notaTemp);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();



          
          }
        }
      
      }
}





//####### fonction d'exécution d'une note ou un accord 

void playDBL(String dblLine, int pos){//(int pitchInt, byte velocity){

    char dblChar = dblLine.charAt(pos);
    //debugln(dblChar);
  if (dblChar=="-"){

  } else if (dblChar=="_"){

        byte notaBytePrec = static_cast<byte>(notaDBL);
        noteOffUSB(4, notaBytePrec, 0);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();


  } else {
    int mouvement;
    bool mouvementChord = false;
    if (dblChar=='n'){
      mouvement = 1;
      mouvementChord = true;
     } else if (dblChar=='p'){
      mouvement = -1;
      mouvementChord = true;      
     } else if (dblChar=='/'){
      mouvement = 1;
      mouvementChord = false;      
     } else if (dblChar==')'){
      mouvement = 2;
      mouvementChord = false;      
     } else if (dblChar==']'){
      mouvement = 3;
      mouvementChord = false;      
     } else if (dblChar=='}'){
      mouvement = 4;
      mouvementChord = false;      
     } else if (dblChar=='\\'){
      mouvement = -1;
      mouvementChord = false;      
     } else if (dblChar=='())'){
      mouvement = -2;
      mouvementChord = false;      
     } else if (dblChar=='['){
      mouvement = -3;
      mouvementChord = false;      
     } else if (dblChar=='{'){
      mouvement = -4;
      mouvementChord = false;      
     } else if (dblChar=='o'){
     } else if (dblChar=='O'){
     }
      else {
     mouvement=-99;
     }

    //debugln(mouvement);
      if (mouvement!=-99){

        byte notaBytePrec = static_cast<byte>(notaDBL);
        noteOffUSB(4, notaBytePrec, 0);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();


          if (mouvementChord == false)  {  // si on a des symbols / ) [...

          //debugln("no chord");

            if (notaDBL<30){
            notaDBL = notaDBL+12;
            }

            if (notaDBL>48){
            notaDBL = notaDBL-12;
            }


            
            if (dblChar=='o'){
             notaDBL = tonica+36;
             //debugln(notaDBL);
            } else if (dblChar=='O'){
             notaDBL = tonica+48;
             //debugln(notaDBL);
            }else{
             notaDBL = getMidiNote(notaDBL, mouvement);
             //debugln("altri caratteri");
            }

            byte notaByte = static_cast<byte>(notaDBL);
            byte velocityAverageOn = 0.8*velocityAverage;
            //noteOnUSB(4, notaByte, velocityAverageOn); // comme ça le valome va diminuer petit à petit
            noteDblOnUSB(4, notaByte, velocityAverageOn);
            MidiUSB.flush();   

          } else { // si on a : n ou p

          //debugln("chord");

            if (notaDBL<36){
            notaDBL = notaDBL+12;
            }

            if (notaDBL>60){
            notaDBL = notaDBL-12;
            }


            notaDBL = getMidiNoteChord(notaDBL, mouvement);
            byte notaByte = static_cast<byte>(notaDBL);
            byte velocityAverageOn = 0.8*velocityAverage;
            noteDblOnUSB(4, notaByte, velocityAverageOn);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();   

          } 
      }
    }
}
















//------------------------------------------------------------------------------

void handleNoteOn(byte channel, byte pitch, byte velocity)
{

		digitalWrite(LED, HIGH);
    int pitchInt = static_cast<int>(pitch);

    // si on jour le Do 36 on active le mode accord 
    if (pitchInt==36){

        modeChords = true;
        chordNoteNbr = 3;

    } else if (pitchInt==38){

        modeChords = true;
        chordNoteNbr = 2;

    } else if (pitchInt==40){
      modeFree = true;
      debugln("mode free dans handleNoteOn");
    }else {
        
         play(pitchInt, velocity);

    }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
		digitalWrite(LED, LOW);
		
    int pitchInt = static_cast<int>(pitch);

        // si on relache le Do 36 on désactive le mode accord 
    if ((pitchInt==36)or(pitchInt==38)){

        modeChords = false;
        chordNoteNbr = 1;

    } else  if (pitchInt==40){

      modeFree = false;
      debugln("mode free dans handleNoteOff");
    
    } else if ((pitchInt>=49)and(pitchInt<=51)) { //accords

    byte notaByte = static_cast<byte>(notePrecChord.get(pitchInt));

      noteOffUSB(3, notaByte, velocity);  // Channel 0, middle C, normal velocity
      MidiUSB.flush();

      int noteprecTemp = notePrecChord_3.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOffUSB(3, notaByte, velocity);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();
        notePrecChord.set(pitchInt, 0);
      }

      noteprecTemp = notePrecChord_5.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOffUSB(3, notaByte, velocity);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();
        notePrecChord.set(pitchInt, 0);
      }




    }else{
    
      
      byte notaByte = static_cast<byte>(notePrec.get(pitchInt));

      noteOffUSB(3, notaByte, velocity);  // Channel 0, middle C, normal velocity
      MidiUSB.flush();

      int noteprecTemp = notePrec_3.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOffUSB(3, notaByte, velocity);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();
        notePrec.set(pitchInt, 0);
      }

      noteprecTemp = notePrec_5.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOffUSB(3, notaByte, velocity);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();
        notePrec.set(pitchInt, 0);
      }



    }
}

// -----------------------------------------------------------------------------


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

void setup()
{
    initNetwork();
    pinMode(LED, OUTPUT);
	
    MIDI.setHandleNoteOn(handleNoteOn); 
    MIDI.setHandleNoteOff(handleNoteOff);    
    MIDI.begin(MIDI_CHANNEL_OMNI);

    
}

void loop()
{
  int packetSize = udp.parsePacket();
  if (packetSize) {
 
    //debugln(velocityAverage);
    
    /// calcul du temps de la mesure 
    unsigned long currentMillis = millis();
    if (precMillis!=0){
    timeMeasure = currentMillis - precMillis; 
    } 
    precMillis = currentMillis;    
    
    // Lire le message UDP
      int len = udp.read(packetBuffer, 20);
      if (len > 0) {
        packetBuffer[len] = '\0'; // Null-terminate la chaîne
      }

      String message = String(packetBuffer);
      int separatorIndex = message.indexOf(';');

      if (separatorIndex != -1) {

        gamme = getScale(message);
        gammeTab = gammeIntToArray(gamme);
        accord = getChord(message);
        accordTab = accordIntToArray(accord);

        debug("gamme : ");
        debugln(gamme);
        debug("accord : ");      
        debugln(accord);        

        int firstNoteChord = accordTab[0];
        int secondNoteChord = accordTab[1];
        int thirdNoteChord = accordTab[2];

        int firstChordDiff = 12+firstNoteChord - thirdNoteChord;
        int secondChordDiff = secondNoteChord - firstNoteChord;
        int thirdChordDiff = thirdNoteChord - secondNoteChord;


        int maxVal = max(firstChordDiff, max(secondChordDiff, thirdChordDiff));

        if (maxVal==firstChordDiff){
          tonica = firstNoteChord; 
        } else if (maxVal==secondChordDiff){
          tonica = secondNoteChord;
        } else if (maxVal==thirdChordDiff){
          tonica = thirdNoteChord;
        }


      dblMeasure = getDBL(message);
      dblMeasure.trim();
      lenDblMeasure = dblMeasure.length();

      timeBit = timeMeasure/lenDblMeasure;   


      indexDbl=0;
      playDBL(dblMeasure, indexDbl);
      precMillisNote = millis();
      

    }
    }else {
      MIDI.read();
      unsigned long currentMillis = millis();

      if  (
            (currentMillis - precMillisNote >= timeBit) and
            (indexDbl<lenDblMeasure-1)
          )
        {
          indexDbl++;
          playDBL(dblMeasure, indexDbl);
          precMillisNote = millis();
        } 

      

    }

}
