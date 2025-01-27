#include <MIDI.h>
#include <MIDIUSB.h>
#include "RECEIVER.h"


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

bool modeChords = false;
int chordNoteNbr=1;

//********************************************************
// gestion des notes précédentes
//********************************************************

class NotePrec {
private:
    int notaPrec[23]; // Plage de 48 à 64 -> 17 valeurs (64 - 48 + 1)
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
int notaChord = 47;

NotePrec notePrec;
NotePrec notePrec_3;
NotePrec notePrec_5;

NotePrec notePrecChord;
NotePrec notePrecChord_3;
NotePrec notePrecChord_5;





//int notaPrec[127];


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

/// fonction qui récupère par exemple 2773 et qui renvoi un array de 7 éléments 
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
/// fonction qui récupère 2773 et qui renvoi un array de 7 éléments 
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
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOffUSB(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}


//####################################################################################################
// Fonction DBL DECODE
// Fonction générique pour obtenir une note relative, même si la note initiale n'est pas dans la gamme
// il faut associer une note midi à un symbole dbl
//####################################################################################################
int dblDecode(int dbl) {

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
  
  
 else if ((dbl==61)or(dbl==49)){ //c, {
    return -1;
  } else if ((dbl==63)or(dbl==51)){ //c, {
    return 1;
  }



 
  
  else {
    return -99;
  }

}



//####################################################################################################
// Fonction GAMMES
// Fonction générique pour obtenir une note relative, même si la note initiale n'est pas dans la gamme
//####################################################################################################

int getRelativeNote(int currentNote, int steps) {

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
          startIndex = (steps >= 0) ? j-1 : j ; // Note supérieure ou inférieure selon la direction
          break;
        }
      }

      // Si aucune note supérieure n'est trouvée, boucle en partant de la fin
      if (startIndex == -1) {
        startIndex = (steps >= 0) ? 0 : size - 1;
      }

      // Calcul de l'index cible avec gestion des boucles
      int targetIndex = (startIndex + steps) % size;
      if (targetIndex < 0) {
        targetIndex += size; // Gestion des indices négatifs
      }

      // correction éventuelle de l'octave
      if((currentNote>gammeTab[targetIndex])&&(steps>0)){
        currentNoteBase+=12;
      } else if((currentNote<gammeTab[targetIndex])&&(steps<0)){
        currentNoteBase-=12;
      }

      return currentNoteBase + gammeTab[targetIndex]; // Retourne la note cible


  return -1; // Clé introuvable
}




//####################################################################################################
// Fonction ACCORDS
// Fonction générique pour obtenir une note relative, même si la note initiale n'est pas dans la gamme
//####################################################################################################

int getRelativeNoteChord(int currentNote, int steps) {

int currentNoteBase = currentNote - currentNote%12;
currentNote= currentNote%12;

    accordTab = accordIntToArray(accord);
      int size = 3;
      //Serial.println(size);
      int startIndex = -1;

      // Rechercher l'index exact ou trouver le voisin le plus proche
      for (int j = 0; j < size; j++) {
        if (accordTab[j] == currentNote) {
          startIndex = j; // La note est dans la gamme
          break;
        } else if (accordTab[j] > currentNote) {
          startIndex = (steps >= 0) ? j-1 : j ; // Note supérieure ou inférieure selon la direction
          break;
        }
      }

      // Si aucune note supérieure n'est trouvée, boucle en partant de la fin
      if (startIndex == -1) {
        startIndex = (steps >= 0) ? 0 : size - 1;
      }

      // Calcul de l'index cible avec gestion des boucles
      int targetIndex = (startIndex + steps) % size;
      if (targetIndex < 0) {
        targetIndex += size; // Gestion des indices négatifs
      }

      // correction éventuelle de l'octave
      if((currentNote>accordTab[targetIndex])&&(steps>0)){
        currentNoteBase+=12;
      } else if((currentNote<accordTab[targetIndex])&&(steps<0)){
        currentNoteBase-=12;
      }

      return currentNoteBase + accordTab[targetIndex]; // Retourne la note cible


  return -1; // Clé introuvable
}




//####### fonction d'exécution d'une note ou un accord 

void play(int pitchInt, byte velocity){

     int mouvement = dblDecode(pitchInt);

      if (mouvement!=-99){
          if ((pitchInt>=54) and (pitchInt<=71) and (pitchInt!=61) and (pitchInt!=63))  {  // si on a des symbols / ) [...
            nota = getRelativeNote(nota, mouvement);
            notePrec.set(pitchInt, nota);
            byte notaByte = static_cast<byte>(nota);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();   
          
            if (modeChords == true){

                    if (chordNoteNbr==2){
                      int notaTemp = getRelativeNote(nota, 2);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();    

                    } else if (chordNoteNbr==3){
                      int notaTemp = getRelativeNote(nota, 2);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

                      notaTemp = getRelativeNote(notaTemp, 2);
                      notePrec_5.set(pitchInt, notaTemp);
                      byte notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();
                  }

            }


          } else if ((pitchInt==61) or (pitchInt==63)){ // si on a : n ou p
            
            nota = getRelativeNoteChord(nota, mouvement);

            
            notePrec.set(pitchInt, nota);
            byte notaByte = static_cast<byte>(nota);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();   

            if (modeChords == true){

                      int notaTemp = getRelativeNoteChord(nota, 1);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

                      notaTemp = getRelativeNoteChord(notaTemp, 1);
                      notePrec_5.set(pitchInt, notaTemp);
                      byte notaByte = static_cast<byte>(notaTemp);
                      noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      MidiUSB.flush();

            }


          } else if ((pitchInt==49) or (pitchInt==51)){


            notaChord = getRelativeNoteChord(notaChord, mouvement);

            
            notePrecChord.set(pitchInt, notaChord);
            byte notaByte = static_cast<byte>(notaChord);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();

            int notaTemp = getRelativeNoteChord(notaChord, 1);
            notePrecChord_3.set(pitchInt, notaTemp);
            notaByte = static_cast<byte>(notaTemp);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();

            notaTemp = getRelativeNoteChord(notaTemp, 1);
            notePrecChord_5.set(pitchInt, notaTemp);
            notaByte = static_cast<byte>(notaTemp);
            noteOnUSB(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            MidiUSB.flush();



          
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

    }else {
        
         play(pitchInt, velocity);

    }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
		digitalWrite(LED, LOW);
		/*
		int pitchInt = static_cast<int>(pitch);
		int mouvement = dblDecode(pitchInt);
		int notaTemp = getRelativeNote(gamme, nota, mouvement);
    //Serial.println(pitchInt);
    */
		
    int pitchInt = static_cast<int>(pitch);

        // si on relache le Do 36 on désactive le mode accord 
    if ((pitchInt==36)or(pitchInt==38)){

        modeChords = false;
        chordNoteNbr = 1;

    } else if ((pitchInt==49)or(pitchInt==51)) { //accords

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

void setup()
{
    initNetwork();
    Serial.begin(31250);//begin(115200);
    pinMode(LED, OUTPUT);
	
	  MIDI.setHandleNoteOn(handleNoteOn); 
    MIDI.setHandleNoteOff(handleNoteOff);    
    MIDI.begin(MIDI_CHANNEL_OMNI);

    
}

void loop()
{
  int packetSize = udp.parsePacket();
  if (packetSize) {
    Serial.println(getRelativeNote(60, 1));
    Serial.println("message ");
    // Lire le message UDP
      int len = udp.read(packetBuffer, 20);
      if (len > 0) {
        packetBuffer[len] = '\0'; // Null-terminate la chaîne
      }

      String message = String(packetBuffer);
      int separatorIndex = message.indexOf(';');

      if (separatorIndex != -1) {
        gamme = message.substring(0, separatorIndex).toInt();

        gammeTab = gammeIntToArray(gamme);
        Serial.print("note : ");
        Serial.print(gammeTab[4]);
        accord = message.substring(separatorIndex + 1).toInt();
        
        //Serial.print("gamme : ");
        //Serial.println(gamme);
        //Serial.println(accord);
        
        /*
        Serial.print("Texte : ");
        Serial.println(text);
        */
        }
    }else {
      MIDI.read();
    }

}
