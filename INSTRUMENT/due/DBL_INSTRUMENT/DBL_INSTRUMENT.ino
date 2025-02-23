#include <MIDI.h>
#include <MIDIUSB.h>
#include "RECEIVER.h"

#define DEBUG 1

#if DEBUG
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
  #define serialBegin() Serial.begin(115200)
#else
  #define debug(x) 
  #define debugln(x)
  #define serialBegin() 
#endif


#define LED 13


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


//MIDI_CREATE_DEFAULT_INSTANCE();
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1,  MIDIRX);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1,  MIDI);



//========================================================

int gamme = 2773;
int* gammeTab;
int accord = 2192;
int* accordTab;
int tonica;

bool modeChords = false;
int chordNoteNbr=1;

bool modeFree = false;
bool modeRandom = false;



/// dbl
unsigned long precMillis = 0; // 0 pour initialisation, la première fois qu'on recois un paquet precMillis = millis()
unsigned long timeMeasure = 0; // 0 pour initialisation, puis timeMeasure sera millis()- precMillis
unsigned long timeBit;
int lenDblMeasure;
String dblMeasure;
int indexDbl = 0;

float dblAttenuation = 0.7;

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
    NotePrec() {
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





//****************************************************************************************************
// midi standard

void noteOn(byte channel, byte pitch, byte velocity) {
  velocityAverage = (alpha * velocity) + ((1 - alpha) * velocityAverage);
  MIDI.sendNoteOn(pitch, velocity, channel);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOff(pitch, 0, channel);
}


// les notes en dbl ne sont pas utilisées pour calculer la velocity moyenne
void noteDblOn(byte channel, byte pitch, byte velocity) {
  MIDI.sendNoteOn(pitch, velocity, channel);
}


//####################################################################################################
// Fonction DBL DECODE
// Fonction générique pour obtenir une note relative, même si la note initiale n'est pas dans la gamme
// il faut associer une note midi à un symbole dbl
//####################################################################################################
int dblDecode(int dbl) {

  if (modeFree==false){
    
    
    int coeff = 1;
    if (modeRandom==true){
    coeff = random(2) * 2 - 1;
    }


    if (dbl==64){  //note a, /
      return coeff*1;
    }else if (dbl==65){ // a#, /
      return coeff*1;
    }else if (dbl==66){ //b, )
      return coeff*2;
    }else if (dbl==67){ //c, )
      return coeff*2;
    }else if (dbl==68){ //c#, ]
      return coeff*3;
    }else if (dbl==69){ //d, ]
      return coeff*3;
    }else if (dbl==70){ //d#, }
      return coeff*4;
    }
	
	
	else if (dbl==71){ // symbole O ->tonique haut
      return coeff*12;
    }	
    
    else if (dbl==62){ // g#, 
      return 0;
    }  
    
    else if (dbl==60){ // g , 
      return -1*coeff;
    }else if (dbl==59){ //f#, 
      return -1*coeff;
    }else if (dbl==58){ //f, (
      return -2*coeff;
    }else if (dbl==57){ //e, (
      return -2*coeff;
    }else if (dbl==56){ //d#, [
      return -3*coeff;
    }else if (dbl==55){ //d, [
      return -3*coeff;
    }else if (dbl==54){ //c#, {
      return -4*coeff;
    }
	
    else if (dbl==53){ // symbole o -> tonique bas
      return -12*coeff;
    } 	
    
    
    else if ((dbl==61)or(dbl==49)){ //p ou accord
      return -1*coeff;
    } else if ((dbl==63)or(dbl==51)){ // n ou accord
      return 1*coeff;
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
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush(); 


        } else {
          if ((pitchInt>=54) and (pitchInt<=70) and (pitchInt!=61) and (pitchInt!=63))  {  // si on a des symbols / ) [...
            nota = getMidiNote(nota, mouvement);
            notePrec.set(pitchInt, nota);
            byte notaByte = static_cast<byte>(nota);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();   
          
            if (modeChords == true){

                    if (chordNoteNbr==2){
                      int notaTemp = getMidiNote(nota, 2);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();    

                    } else if (chordNoteNbr==3){
                      int notaTemp = getMidiNote(nota, 2);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

                      notaTemp = getMidiNote(notaTemp, 2);
                      notePrec_5.set(pitchInt, notaTemp);
                      byte notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();
                  }

            }


          } else if (pitchInt==71){ // on monte vers la tonique

              int ottava = nota/12; // en int pas de reste
              int baseottavaMidi = ottava*12;//ça tombe sur le do de l'octave en cours

              if (baseottavaMidi+tonica>nota){

                  nota = baseottavaMidi+tonica; 
                  notePrec.set(pitchInt, nota);
                  byte notaByte = static_cast<byte>(nota);
                  noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                    
              } else {
                  nota = baseottavaMidi+tonica+12; 
                  notePrec.set(pitchInt, nota);
                  byte notaByte = static_cast<byte>(nota);
                  noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                                 
              }

          } else if (pitchInt==53){//on descent vers la tonique


              int ottava = nota/12; // en int pas de reste
              int baseottavaMidi = ottava*12;//ça tombe sur le do de l'octave en cours

              if (baseottavaMidi+tonica<nota){

                  nota = baseottavaMidi+tonica; 
                  notePrec.set(pitchInt, nota);
                  byte notaByte = static_cast<byte>(nota);
                  noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                    
              } else {
                  nota = baseottavaMidi+tonica-12; 
                  notePrec.set(pitchInt, nota);
                  byte notaByte = static_cast<byte>(nota);
                  noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                                 
              }          

          }  else if ((pitchInt==61) or (pitchInt==63)){ // si on a : n ou p
            
            nota = getMidiNoteChord(nota, mouvement);

            
            notePrec.set(pitchInt, nota);
            byte notaByte = static_cast<byte>(nota);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();   

            if (modeChords == true){

                      int notaTemp = getMidiNoteChord(nota, 1);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

                      notaTemp = getMidiNoteChord(notaTemp, 1);
                      notePrec_5.set(pitchInt, notaTemp);
                      byte notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

            }


          } else if (pitchInt>=49 and pitchInt<=51){


            notaChord = getMidiNoteChord(notaChord, mouvement);

            
            notePrecChord.set(pitchInt, notaChord);
            byte notaByte = static_cast<byte>(notaChord);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();

            int notaTemp = getMidiNoteChord(notaChord, 1);
            notePrecChord_3.set(pitchInt, notaTemp);
            notaByte = static_cast<byte>(notaTemp);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();

            notaTemp = getMidiNoteChord(notaTemp, 1);
            notePrecChord_5.set(pitchInt, notaTemp);
            notaByte = static_cast<byte>(notaTemp);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();



          
          }
        }
      
      }
}





//####### fonction d'exécution d'une note ou un accord 

void playDBL(String dblLine, int pos){//(int pitchInt, byte velocity){
  debugln("note");
    char dblChar = dblLine.charAt(pos);
    //debugln(dblChar);
  if (dblChar=='-'){

  } else if (dblChar=='_'){

        byte notaBytePrec = static_cast<byte>(notaDBL);
        noteOff(4, notaBytePrec, 0);  // Channel 0, middle C, normal velocity
        


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
        noteOff(4, notaBytePrec, 0);  // Channel 0, middle C, normal velocity
        


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
            byte velocityAverageOn = dblAttenuation*velocityAverage;
            //noteOnUSB(4, notaByte, velocityAverageOn); // comme ça le valome va diminuer petit à petit
            noteDblOn(4, notaByte, velocityAverageOn);

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
            byte velocityAverageOn = dblAttenuation*velocityAverage;
            noteDblOn(4, notaByte, velocityAverageOn);   // Channel 0, middle C, normal velocity

          } 
      }
    }
}


//-------------- EVENEMENTS NOTES ON ET OFF ----------------------------------------------------------------

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
    } else if (pitchInt==41){
      modeRandom = true;
      debugln("mode random dans handleNoteOn");
    }
    
    
    else {
        
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
    
    } else  if (pitchInt==41){

      modeRandom = false;
      debugln("mode random dans handleNoteOff");
    
    }    
    
    else if ((pitchInt>=49)and(pitchInt<=51)) { //accords

    byte notaByte = static_cast<byte>(notePrecChord.get(pitchInt));

      noteOff(3, notaByte, 0);  // Channel 0, middle C, normal velocity
      MidiUSB.flush();

      int noteprecTemp = notePrecChord_3.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOff(3, notaByte, 0);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();
        notePrecChord.set(pitchInt, 0);
      }

      noteprecTemp = notePrecChord_5.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOff(3, notaByte, 0);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();
        notePrecChord.set(pitchInt, 0);
      }




    }else{
    
      
      byte notaByte = static_cast<byte>(notePrec.get(pitchInt));

      noteOff(3, notaByte, 0);  // Channel 0, middle C, normal velocity
      MidiUSB.flush();

      int noteprecTemp = notePrec_3.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOff(3, notaByte, 0);  // Channel 0, middle C, normal velocity
        MidiUSB.flush();
        notePrec.set(pitchInt, 0);
      }

      noteprecTemp = notePrec_5.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOff(3, notaByte, 0);  // Channel 0, middle C, normal velocity
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

bool screenEnabled = false;

int ethernetHW = 0;
String ethernetStatus;

void setup()
{
    serialBegin();
    initNetwork();
    
    
    ethernetStatus = Ethernet.linkReport();
    debug("ethernetHW : ");debugln(ethernetStatus);

    if (ethernetStatus=="NO LINK"){
      debugln("problème hardware ethernet");
    } else {
      ethernetHW = 1;
      debugln("ethernet hardware OK");
    }


    pinMode(LED, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    debugln(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  } else {
  debugln(F("Display OK"));
  display.clearDisplay();
  screenEnabled = true;
  }

  debug("screenEnabled : ");debugln(screenEnabled);
  
    MIDI.setHandleNoteOn(handleNoteOn); 
    MIDI.setHandleNoteOff(handleNoteOff);    
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();
    

    
}

int packetSize =0;
void loop()
{
  if (ethernetHW == 1){
  int packetSize = udp.parsePacket();
  }

  if ((packetSize) and (ethernetHW == 1)) {
    
 
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
      infoDisplay(message, "->");


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










//************************************************************************
// display 
//************************************************************************

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
    //display.print(ligneActive+1);
    display.print("/");
    //display.print(nLignes);


    //display.setFont(&FreeMono9pt7b);
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.print(curs);
    display.setTextSize(3);
    display.print(getChordLabel(chInt));
    display.setTextSize(3);
    display.println("");

    display.setTextSize(1);
    display.println(getScaleLabel(scInt));
    display.setTextSize(1);
    display.println(dblSymbols);
    display.display();
  }
}

String getChordLabel(int accInt) {
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





