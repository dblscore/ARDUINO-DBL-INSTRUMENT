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


/// ***** dbl *********************
// 0 pour initialisation, 
//la première fois qu'on recois un paquet precMillis = millis()
//puis timeMeasure sera millis()- precMillis
unsigned long precMillis = 0; 
unsigned long timeMeasure = 0; 
unsigned long timeBit;
int lenDblMeasure;
String dblMeasure;
int indexDbl = 0;

float dblAttenuation = 0.7;

unsigned long precMillisNote;

// ******** velocity *************
// velocityAverage est une velocity lissée ajoustée à chaque noteOn
// utilisé pour la velocity des notes en DBL envoyées par le contrôleur
byte velocityAverage=80;
float alpha = 0.3; // Facteur d'atténuation (pour ajustement entre 0 et 1)


//********************************************************
// gestion des notes précédentes
/*
Il s'agit d'une classe qui permet d'avoir le souvenir de la note précédente 
Utilisé pour attribuer la note selon le symbole DBL joué dans l'instrument 
On considère 23 possibles touches avec les différents symboles DBL
**/
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

//\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
//********* initiation *********************
/*
Initialement pas de note précédente donc on paramètre ici les différents noteprec
*/
//\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
// note précédente pour les gammes 
int nota = 59;
int pitchPrec = 60;
// note précédente pour les accords 
int notaChord = 47;
// note précédente pour la mélodie DBL (note plutôt basse, mais modifiable) 
int notaDBL = 35;

//*******************************************
// instanciation des objects de classe 
//*******************************************

NotePrec notePrec;   // note précédente pour l'instrument 
NotePrec notePrec_3; // note précédente pour la triade (utilisé si mode accord activé) 
NotePrec notePrec_5; // note précédente pour la triade supérieure (utilisé si mode accord activé)

NotePrec notePrecChord; // note précédente pour la tonique de l'accord (si on utilise les touches accords) 
NotePrec notePrecChord_3; // note précédente pour la tierce de l'accord (si on utilise les touches accords)
NotePrec notePrecChord_5; // note précédente pour la quinte de l'accord (si on utilise les touches accords)

NotePrec notePrecDBL; // note précédente pour la mélodie DBL 


//***************************************************************
//******************** scales ***********************************
//***************************************************************
/*
Les gammes ne sont pas enregistré dans le programme
Elles sont décodées à partir du numéro reçu par le controlleur 
Ici les associations implicites entre numéro de la gamme et nom de la gamme : 


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
/// fonction qui récupère le numéro de la gamme (par exemple 2773, gamme de do majeur)
/// et qui renvoi un array des 7 notes (par exemple {0,2,4,5,7,9,11})
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
/// fonction qui récupère le numéro de l'accord (par exemple 2192, do majeur)
/// et qui renvoi un array des 3 notes (par exemple {0,4,7})
//////////////////////////////////////////////////////////////////////
int* accordIntToArray(int number) {
  static int result[3]; // Tableau pour stocker les résultats
  int index = 0;

  // Parcourir 12 bits 
  for (int i = 0; i < 12; i++) {
    if ((number >> (11 - i)) & 1) {  // Vérifie si le bit est à 1
      result[index] = i;  // Ajouter la position sans la multiplication
      index++;
    }
  }
  
  return result;
}

//***************************************************************
//******************** chords ***********************************
//***************************************************************

/*

Les accords ne sont pas enregistré dans le programme
Ils sont décodés à partir du numéro reçu par le controlleur 
Ici les associations implicites entre numéro de l'accord et nom de l'accord : 

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
//***************************************************************************************************

// fonction utilisé pour la sortie MIDI en USB
void noteOnUSB(byte channel, byte pitch, byte velocity) {
  // on lisse la valeur de la velocity pour l'appliquer aux notes de la mélodie en DBL
  velocityAverage = (alpha * velocity) + ((1 - alpha) * velocityAverage);
  
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

// fonction utilisé pour la sortie MIDI en USB
void noteOffUSB(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}


// fonction utilisé pour la sortie MIDI en USB
void noteDblOnUSB(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}


//****************************************************************************************************
// midi 
/* Dans le code on utilise les fonction générique noteOn, noteOff.. 
   Eventuellement si on veut une sortie MIDI sur connecteur à 5 pin on modifie ces fonctions 
   en activanet le commentaire (doucle sortie MIDI et standard)
*/
//****************************************************************************************************

void noteOn(byte channel, byte pitch, byte velocity) {

  noteOnUSB(channel,pitch, velocity);
  /*
  velocityAverage = (alpha * velocity) + ((1 - alpha) * velocityAverage);
  MIDI.sendNoteOn(pitch, velocity, channel);
  **/
}

void noteOff(byte channel, byte pitch, byte velocity) {

  noteOffUSB(channel, pitch, velocity);

  //MIDI.sendNoteOff(pitch, 0, channel);
}


// les notes en dbl ne sont pas utilisées pour calculer la velocity moyenne
void noteDblOn(byte channel, byte pitch, byte velocity) {
  noteDblOnUSB(channel, pitch, velocity);
  
  //MIDI.sendNoteOn(pitch, velocity, channel);
}


//####################################################################################################
// Fonction DBL DECODE
/* Récupère une des touches jouées et restitue la valeur 0, +1, -1, +2... selon la touche jouée
*/
//####################################################################################################
int dblDecode(int dbl) {
  
  //-----------------------------------------------------------------
  // si le mode free n'est pas activé 
  // (mode DBL où chaque touche correspond à une variation de dedré)
  //-----------------------------------------------------------------
  if (modeFree==false){

    // si le mode random est activé un coefficient random +1 ou -1 est appliqué 
    int coeff = 1;
    if (modeRandom==true){
    coeff = random(2) * 2 - 1;
    }

    //******************************
    // symboles relatifs à la gamme 
    //******************************
    if (dbl==64){  // symbole  /
      return 1*coeff;
    }else if (dbl==65){ // symbole  /
      return 1*coeff;
    }else if (dbl==66){ //symbole  )
      return 2*coeff;
    }else if (dbl==67){ //symbole  )
      return 2*coeff;
    }else if (dbl==68){ //symbole  ]
      return 3*coeff;
    }else if (dbl==69){ //symbole  ]
      return 3*coeff;
    }else if (dbl==70){ //symbole  }
      return 4*coeff;
    }
    
    else if (dbl==62){ // symbole  = 
      return 0;
    }  
    
    else if (dbl==60){ // symbole  \ 
      return -1*coeff;
    }else if (dbl==59){ //symbole  \ 
      return -1*coeff;
    }else if (dbl==58){ //symbole  (
      return -2*coeff;
    }else if (dbl==57){ //symbole  (
      return -2*coeff;
    }else if (dbl==56){ //symbole  [
      return -3*coeff;
    }else if (dbl==55){ //symbole  [
      return -3*coeff;
    }else if (dbl==54){ //symbole  {
      return -4*coeff;
    }

    //******************************
    // symboles relatifs à l'accord
    //******************************

    else if (dbl==71){ // symbole O ->tonique haut
      return 12*coeff;
    }

    else if (dbl==53){ // symbole o -> tonique bas
      return -12*coeff;
    } 
       
    else if ((dbl==61)or(dbl==49)){ // symbole p, note précédente de l'accord
      return -1*coeff;
    } else if ((dbl==63)or(dbl==51)){ // symbole n, note suivante de l'accord
      return 1*coeff;

    } else if (dbl==50){ // on joue le même accord
      return 0;
    }

    else {
    return -99; // symbol inexistant
    }



  } else if (modeFree==true){

  //-----------------------------------------------------------------
  // mode free activé 
  // On joue d'une façon classique, mais les notes sont adaptées à la gamme et l'accord en cours
  //-----------------------------------------------------------------

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
// Fonction pour les GAMMES
// On récupère la note précédente et le mouvement (+1, +2, -1...)
// elle restitue la note MIDI, en fonction de la gamme en cours
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

}


//####################################################################################################
// Fonction pour l'accord
// On récupère la note précédente et le mouvement (0, +1, ou -1)
// elle restitue la note MIDI, en fonction de l'accord en cours
//####################################################################################################

int getMidiNoteChord(int currentNote, int mouv) {

int currentNoteBase = currentNote - currentNote%12;
currentNote= currentNote%12;

    accordTab = accordIntToArray(accord);
      int size = 3;
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

}


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//>>>>>> fonction principale pour exécuter une note de la gamme ou de l'accord 
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************



void play(int pitchInt, byte velocity){

     int mouvement = dblDecode(pitchInt);

      if (mouvement!=-99){
        if (modeFree==true){

            nota = getMidiNote(nota, mouvement);
            notePrec.set(pitchInt, nota);
            pitchPrec = pitchInt;
            byte notaByte = static_cast<byte>(nota);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity

        } else {
          if ((pitchInt>=54) and (pitchInt<=70) and (pitchInt!=61) and (pitchInt!=63))  {  // si on a des symbols / ) [...
            nota = getMidiNote(nota, mouvement);
            notePrec.set(pitchInt, nota);
            byte notaByte = static_cast<byte>(nota);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
               
          
            if (modeChords == true){

                    if (chordNoteNbr==2){
                      int notaTemp = getMidiNote(nota, 2);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                          

                    } else if (chordNoteNbr==3){
                      int notaTemp = getMidiNote(nota, 2);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      

                      notaTemp = getMidiNote(notaTemp, 2);
                      notePrec_5.set(pitchInt, notaTemp);
                      byte notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      
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

          } else if ((pitchInt==61) or (pitchInt==63)){ // si on a : n ou p
            
            nota = getMidiNoteChord(nota, mouvement);
            notePrec.set(pitchInt, nota);
            byte notaByte = static_cast<byte>(nota);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
               

            if (modeChords == true){

                      int notaTemp = getMidiNoteChord(nota, 1);
                      notePrec_3.set(pitchInt, notaTemp);
                      notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      

                      notaTemp = getMidiNoteChord(notaTemp, 1);
                      notePrec_5.set(pitchInt, notaTemp);
                      byte notaByte = static_cast<byte>(notaTemp);
                      noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
                      

            }


          } else if (pitchInt>=49 and pitchInt<=51){


            notaChord = getMidiNoteChord(notaChord, mouvement);

            
            notePrecChord.set(pitchInt, notaChord);
            byte notaByte = static_cast<byte>(notaChord);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            

            int notaTemp = getMidiNoteChord(notaChord, 1);
            notePrecChord_3.set(pitchInt, notaTemp);
            notaByte = static_cast<byte>(notaTemp);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            

            notaTemp = getMidiNoteChord(notaTemp, 1);
            notePrecChord_5.set(pitchInt, notaTemp);
            notaByte = static_cast<byte>(notaTemp);
            noteOn(3, notaByte, velocity);   // Channel 0, middle C, normal velocity
            



          
          }
        }
      
      }
}





//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//>>>>>> fonction principale pour exécuter la mélodie DBL venant du contrôleur 
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


void playDBL(String dblLine, int pos){//(int pitchInt, byte velocity){

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

//--------------------------------------------------------------------------
//-------------- EVENEMENTS NOTES ON ET OFF --------------------------------
//--------------------------------------------------------------------------
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

      noteOff(3, notaByte, 0);  
      

      int noteprecTemp = notePrecChord_3.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOff(3, notaByte, 0);  
        
        notePrecChord.set(pitchInt, 0);
      }

      noteprecTemp = notePrecChord_5.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOff(3, notaByte, 0);  
        
        notePrecChord.set(pitchInt, 0);
      }




    }else{
    
      
      byte notaByte = static_cast<byte>(notePrec.get(pitchInt));

      noteOff(3, notaByte, 0);  
      

      int noteprecTemp = notePrec_3.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOff(3, notaByte, 0);  
        
        notePrec.set(pitchInt, 0);
      }

      noteprecTemp = notePrec_5.get(pitchInt);

      if (noteprecTemp!=0){
        notaByte = static_cast<byte>(noteprecTemp);

        noteOff(3, notaByte, 0); 
        
        notePrec.set(pitchInt, 0);
      }



    }
}



//***************************************************************************************
// fonction pour décoder la gamme, l'accord et la mélodie DBL à partir des données reçus 
// dans le packet Multicast 
//***************************************************************************************

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
    serialBegin();  

    // activation fonctions Handle dès réception d'une note MIDI
    MIDI.setHandleNoteOn(handleNoteOn); 
    MIDI.setHandleNoteOff(handleNoteOff);    
    MIDI.begin(MIDI_CHANNEL_OMNI);
    
    // Eviter d'avoir une boucle entre MIDI IN et MIDI OUT
    MIDI.turnThruOff();
     
}

void loop()
{
  int packetSize = udp.parsePacket();
  if (packetSize) {
 
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
        /*
        debug("gamme : ");debugln(gamme);
        debug("accord : ");debugln(accord);        
        */
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
