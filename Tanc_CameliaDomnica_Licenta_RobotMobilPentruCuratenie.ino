//Licenta 2024 - Robot mobil pentru curatenie
#include "Wire.h"   
#include "LiquidCrystal_I2C.h"
#include <avr/io.h>
#include <avr/interrupt.h>

//Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Initializare pini componente
//Senzori ultrasonici(left, middle, right)
const int echo_L = 5;                                 
const int trig_L = 6;
const int echo_M = A3;
const int trig_M = A2;
const int echo_R = A1;
const int trig_R = A0;
float distance_L, distance_M, distance_R;
//Modul bluetooth
char incomingByte;
//Motoare
const int enableA = 9;
const int in1 = 7;
const int in2 = 8;
const int enableB = 11;
const int in3 = 10;
const int in4 = 12;
//Pompa apa
const int pump = 2;
volatile bool pumpState = false; // starea initiala a pompei - oprita
volatile bool timerEnabled = true; // starea timer-ului - pornit
//Timp functionare pompa apa
const unsigned long intervalOn = 2000; //2 secunde pornita
const unsigned long intervalOff = 10000; //10 secunde oprita
int contor=0; 
//Directia robotului, distanta pentru ocolirea obstacolelor, distanta pentru iesirea din spatiu inchis
int direction = 1; 
const int avoid_distance = 25; 
const int space_distance = 15; 
//Pinul senzorului de detectare a liniei negre(perete)
const int lineSensorPin = A6; 


void setup()
{ 
  pinMode(lineSensorPin, INPUT); //Setam pinul senzorului de detectare a liniei negre ca input
  //Motoare
  pinMode(enableA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enableB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  //Pompa apa
  pinMode(pump, OUTPUT);
  //Senzori ultrasonici
  pinMode(trig_L, OUTPUT);
  pinMode(echo_L, INPUT);
  pinMode(trig_M, OUTPUT);
  pinMode(echo_M, INPUT);
  pinMode(trig_R, OUTPUT);
  pinMode(echo_R, INPUT);

  //Stare initiala a pompei de apa: oprita
  digitalWrite(pump, LOW); 
  //Stare initiala a motoarelor:oprite
  analogWrite(enableA, 0);
  analogWrite(enableB, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);


  //Initializam ecranul LCD
  lcd.init();
  //Setam culoarea de fundal
  lcd.backlight();
  //Starea initiala e ecranului(stergem ecranul)
  lcd.clear();
  //Pozitionam cursorul pe prima linie si coloana
  lcd.setCursor(0, 0);
  lcd.print("Welcome!");
  //Pozitionam cursorul pe a doua  linie si prima coloana
  lcd.setCursor(0, 1);
  lcd.print("Licenta 2024");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Choose option");
  lcd.setCursor(0, 1);
  lcd.print("M/A");
  //Pornim comunicarea seriala
  Serial.begin(9600);

  //Timer1
  noInterrupts(); //Dezactivam intreruperile

  TCCR1A = 0; //Setam registrul TCCR1A la 0
  TCCR1B = 0; //Setam registrul TCCR1B la 0
  TCNT1 = 0; //Setam contorul la 0

  //Setam valoarea la care se va produce intreruperea (1 Hz, 1 secunda interval)
  OCR1A = 15624; // (16*10^6) / (1024*1) - 1 (trebuie să fie < 65536)

  //CTC (Clear Timer on Compare Match) mod
  TCCR1B |= (1 << WGM12);
  //prescaler 1024
  TCCR1B |= (1 << CS12) | (1 << CS10);

  //Permite intreruperea la compararea cu OCR1A
  TIMSK1 |= (1 << OCIE1A);

  pumpState = false;
  timerEnabled = true;
  contor = 0;

  interrupts(); //Activam intreruperile

  delay(2000);
}


void loop()
{
  if (Serial.available() > 0)   //Verificam daca avem date si le citim
  {
    incomingByte = Serial.read();   
  }

  switch(incomingByte)   
  {
    case 'M':
    lcd.clear();
    lcd.print("Manual Mode");
    manualMode();
    incomingByte='*';
    break;
    
    case 'A':
    lcd.clear();
    lcd.print("Autonomous Mode");
    automaticMode();
    incomingByte='*';
    break;
    
    delay(5000);
  }
}



ISR(TIMER1_COMPA_vect) {
  if (!timerEnabled) {
    return; //Daca temporizatorul este dezactivat, dezactivam pompa
  }
  
  contor++;
  
  if (pumpState == false && contor >= intervalOff / 1000) {
    pumpState = true;
    digitalWrite(pump, LOW); //Pornim pompa
    contor = 0;
  } else if (pumpState == true && contor >= intervalOn / 1000) {
    pumpState = false;
    digitalWrite(pump, HIGH); //Oprim pompa
    contor = 0;
  }
}

void manualMode()
{
  while(true)
  {
  if (Serial.available() > 0)   //Citim comenzile primite
  {
    incomingByte = Serial.read();   
  }

  switch(incomingByte)    
  {
    case 'F':
    moveForward();
    lcd.clear();
    lcd.print("Forward");
    incomingByte='*';
    break;
    
    case 'B':
    moveBackward();
    lcd.clear();
    lcd.print("Backward");
    incomingByte='*';
    break;
    
    case 'L':
    moveLeft_Manual();
    lcd.clear();
    lcd.print("Left");
    incomingByte='*';
    break;
    
    case 'R':
    moveRight_Manual();
    lcd.clear();
    lcd.print("Right");
    incomingByte='*';
    break;
    
    case 'S':
    moveStop();
    lcd.clear();
    lcd.print("Stop");
    incomingByte='*';
    break;
    
    case 'P':
    digitalWrite(pump, LOW);
    lcd.clear();
    lcd.print("Pump ON");
    incomingByte='*';
    break;
    
    case 'p':
    digitalWrite(pump, HIGH); 
    incomingByte='*';
    break;
    
    delay(5000);
  }

  }
}



void automaticMode()
{
  while(true)
  {
  //Citim cele trei distante
  distance_L = readDistance_L(trig_L, echo_L);    
  distance_M = readDistance_M(trig_M, echo_M);
  distance_R = readDistance_R(trig_R, echo_R);
  lcd.clear();
  lcd.print("Cleaning...");

  //Daca e linie neagra, atunci e perete - intoarcere
  if (analogRead(lineSensorPin)>=690) {   
    //Stop
    moveStop();
   //Intoarcere 180 grade
    if (direction == 1) {
      moveLeft();
      delay(2000);
    }
    else if (direction==-1){
      moveRight();
      delay(2000);
    }
    //Schimbare directie
    direction *= -1;
  }
  else if (distance_M <= avoid_distance) 
  {
    //Detectare obstacole in fata
    moveStop();
    lcd.clear();
    lcd.print("obstacle");
    timerEnabled = false;
    if (distance_L > distance_R) 
    { 
      //Ocolire prin stanga
      avoidLeft();
    } 
    else if (distance_R >= distance_L)
   {
      //Ocolire prin dreapta
      avoidRight();
   }
  }
  else if (distance_M<=space_distance && distance_L<=space_distance && distance_R<=space_distance)
  {
    moveStop();
    lcd.clear();
    lcd.print("dead end");
    //Robotul este blocat intr-un spatiu inchis si va da cu spatele pana cand exista spatiu destul
    do
      {
        moveBackward();
        timerEnabled = false;
      }while(distance_M<=space_distance && distance_L<=space_distance && distance_R<=space_distance);
    //Dupa ce iese din spatiul inchis, va ocoli prin stanga sau dreapta 
    moveStop();
    timerEnabled = false;
    if (distance_L > distance_R) { 
      avoidLeft();
    } 
    else if (distance_R >= distance_L)
   {
      avoidRight();
    }
  }
  else if(analogRead(lineSensorPin)<=300) //Identificare cadran alb - oprire curatenie
  {
    while(true)
    {
      moveStop();
      timerEnabled=false;
      lcd.clear();
      lcd.println("Done cleaning.");
    }
  }
  else 
  {
    //Daca nu sunt obtacole, robotul merge inainte
    moveForward();
    timerEnabled = true;
  }
}
}


float readDistance_L(int triggerPin, int echoPin) {
  //Trimiterea unui semnal trigger catre senzor
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  
  //Masurarea timpului pentru care pinul echo este HIGH
  float duration = pulseIn(echoPin, HIGH);
  
  //Calcularea distantei pe baza timpului masurat
  float distanceL = duration * 0.034 / 2; // Distanta = Timp * Viteza sunetului / 2
  return distanceL;
}

float readDistance_M(int triggerPin, int echoPin) {
  //Trimiterea unui semnal trigger catre senzor
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  
  //Masurarea timpului pentru care pinul echo este HIGH
  float duration = pulseIn(echoPin, HIGH);
  
  //Calcularea distantei pe baza timpului masurat
  float distanceM = duration * 0.034 / 2; // Distanta = Timp * Viteza sunetului / 2
  return distanceM;
}

float readDistance_R(int triggerPin, int echoPin) {
  //Trimiterea unui semnal trigger catre senzor
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  
  //Masurarea timpului pentru care pinul echo este HIGH
  float duration = pulseIn(echoPin, HIGH);
  
  //Calcularea distantei pe baza timpului masurat
  float distanceR = duration * 0.034 / 2; // Distanta = Timp * Viteza sunetului / 2
  return distanceR;
}


void moveForward()
{
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void moveBackward()
{
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void moveLeft()
{
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(650);
  moveStop();
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(350); 
  moveStop();
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(650);
  moveStop();
}

void moveRight()
{
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(650);
  moveStop();
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(350);
  moveStop();
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(650); 
  moveStop();
}


void moveLeft_Manual()
{
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

}

void moveRight_Manual()
{
  analogWrite(enableA, 1);
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);

}

void moveStop()
{
  analogWrite(enableA, 0);
  analogWrite(enableB, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void avoidLeft()
{
  analogWrite(enableA, 1); //stanga
  analogWrite(enableB, 1);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(550);
  moveStop();
  analogWrite(enableA, 1); //fata
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(400);
  moveStop();
  analogWrite(enableA, 1); //dreapta
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(550);
  moveStop();
  analogWrite(enableA, 1); //fata
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(600);
  moveStop();
  analogWrite(enableA, 1); //dreapta
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(550);
  moveStop();
  analogWrite(enableA, 1); //fata
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(400);
  moveStop();
  analogWrite(enableA, 1); //stanga
  analogWrite(enableB, 1);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(550);
  moveStop();
}

void avoidRight()
{
  //Ocolire prin dreapta
  analogWrite(enableA, 1); //dreapta
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(550);
  moveStop();
  analogWrite(enableA, 1); //fata
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(400);
  moveStop();
  analogWrite(enableA, 1); //stanga
  analogWrite(enableB, 1);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(550);
  moveStop();
  analogWrite(enableA, 1); //fata
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(600);
  moveStop();
  analogWrite(enableA, 1); //stanga
  analogWrite(enableB, 1);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(550);
  moveStop();
  analogWrite(enableA, 1); //fata
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(400);
  moveStop();
  analogWrite(enableA, 1); //dreapta
  analogWrite(enableB, 1);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(550);
  moveStop();
}
