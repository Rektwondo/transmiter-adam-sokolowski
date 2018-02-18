//Za³aczenie plikow nag³owkowych / bibliotek
#include <SPI.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
// Definiujemy sta³e (gdy w programie uzyjemy ktorejs nazwy to przy kompilacji podmieni nazwe na liczbe.
#define MIN_V 3450 // Wartosc minimalna napiecia zasilania (bateria roz³adowana)
#define MAX_V 4050 // Wartosc maksymalna napiecia zasilania (bateria na³adowana)
#define MAX_BLE_WAIT 80 // Maksymalny czas (sek.) proby nawiazania po³aczenia Bluetooth
#define SLEEP_TIME 34 // Czas uspienia (jedna minuta trwa ok. 7 jednostek) #define MAX_NFC_READTRIES 10 // Ilosc podejmowanych prob dla kazdego skanu NFC
//Definiujemy zmienne globalne - dostepne w kazdej chwili z kazdego miejsca w programie, niektorym zostaja nadane wartosci, a niektorym jeszcze nie.
//const int - int to zmienna liczbowa calkowita z zakresu od -32767 do 32767
const int SSPin = 10;
const int IRQPin = 9;
const int NFCPin1 = 7;
const int NFCPin2 = 8;
30
const int NFCPin3 = 4;
const int BLEPin = 3;
const int BLEState = 2;
const int MOSIPin = 11;
const int SCKPin = 13;
//byte to tez zmienna liczbowa ca³kowita ale jej zakres to tylko osiem bajtow, czyli moze przyjac wartosci od -128 do 127.
byte RXBuffer[24];
byte NFCReady = 0;
byte FirstRun = 1;
byte batteryLow;
int batteryPcnt;
long batteryMv; //
ca³kowita ale zapisywana na 32 bitach (nie jak int na 16)
int noDiffCount = 0;
int sensorMinutesElapse;
float lastGlucose;
float trend[16]; //Tabela trend z 16stoma elementami typu float
SoftwareSerial ble_Serial(5, 6); // Funkcja zdefiniowana w bibliotece SoftwareSerial - definicja pinow wejsc oraz wyjsc Bluetooth (RX i TX)
//Funkcja setup ustawia kontroler tak by pozniej mog³y wykonac sie jego inne funkcje. Znajdzie pasmo komunikacji z modulem Bluetooth, ustawi BM19 jako urzadzenie peryferyjne
void setup() {
//pinMode to zdefiniowanie pinu i jego trybu - w pierwszym mamy, ze program bedzie uzywa³ pinu nazwanego IRQPin jako wyjsciowego- czyli procesor bedzie nim sterowa³ ustawiajac stan na wysoki lub niski (napiecie/brak napiecia)
pinMode(IRQPin, OUTPUT);
//digitalWrite- odnosimy sie do konkretnego pinu <tutaj IRQPin> i ustawiamy jego stan <HIGH - czyli: stan wysoki, napiecie na pinie,
long to inaczej "int long" czyli zmienna liczbowa
31
logiczna jedynka> <LOW - stan niski, brak napiecia, logiczne zero>
digitalWrite(IRQPin, HIGH);
pinMode(SSPin, OUTPUT);
digitalWrite(SSPin, HIGH);
pinMode(NFCPin1, OUTPUT);
digitalWrite(NFCPin1, HIGH);
pinMode(NFCPin2, OUTPUT);
digitalWrite(NFCPin2, HIGH);
pinMode(NFCPin3, OUTPUT);
digitalWrite(NFCPin3, HIGH);
pinMode(BLEPin, OUTPUT);
digitalWrite(BLEPin, HIGH);
pinMode(BLEState, INPUT); // Tutaj jest pin wejsciowy- procesor bedzie
z niego odczytywa³ stany (HIGH/LOW)
     pinMode(MOSIPin, OUTPUT);
     pinMode(SCKPin, OUTPUT);
Serial.begin(9600); //Serial.begin(liczba) <- definicja czestotliwosci z jaka bedziemy wysy³ac/odbierac dane przez "serial" cdo jakiegos innego urzadzenia/wyssietlacza/programatora
//Tabela osmiu long intow - bo w zwyk³ym int nie da sie zapisac liczby wiekszej niz 32767.
// W nawiasach {} po przecinku nadawane sa wartosci kolejnym zmiennym w tabeli.
long bleBaudrate[8] = { 1200,2400,4800,9600,19200,38400,57600,115200 };
     for (int i = 0; i<8; i++)
{
//Podobnie do Serial.begin, tylko przyjmuje wartosci zapisane w tabeli bleBaudrate, czyli testuje dla kazzego wykonania petli inna czestotlwosc (dla i=0 to 1200, i=7 to 115200)
      ble_Serial.begin(bleBaudrate[i]);
ble_Serial.write("AT"); //ble_Serial.write to wypisanie wartosci w nawiasie na wyjscie do ble_serial (Bluetootha).
32
delay(500); //Odczekanie 500ms (domyslnie delay jest w milisekundach)
//ble_Serial.read(); - przeczyta to co jest w kolejce odebranych przez Bluetooth, a ze zmienna jest char to wczyta tylko pierwszy znak
      char c = ble_Serial.read();
      char d = ble_Serial.read();
//Instrukcja w if'ie, tutaj mamy sam break ktory wykona sie gdy element w nawiasie przyjmie wartosc TRUE (czy inaczej 1), jesli bedzie FALSE (lub 0) to program przejdzie dalej.
// Tutaj wystepuja dwa warunki po³aczone znakiem && czyli przyjeta zostanie wartosc true tylko gdy oba beda poprawne. Pod C bedzie znak O, a pod D znak K.
      if (c == 'O' && d == 'K')
break; //Break - gdy program dojdzie do tego miejsca (czyli if da wartosc true) to natychmiast wyjdzie z tej petli
}
//Ogolnie ta petla mia³a za zadanie znalesc czestotliwosc, w ktorej dostanie odpowiedz, gdy ja przerwiemy to zdefiniowane zmienne zostaja utracone (czyli char c i d)
//Jednak wartosc jaka by³a na ble_Serial.begin, ta w ktorej uzyska³ odpowiedz zostanie juz ustawiona.
//Czyli ta petla sprawdza i ustala jakiej czestotliwosci program bedzie uzywa³ do komunkacji z tym modu³em.
     delay(100); //Poczekaj 100ms
ble_Serial.write("AT+NAMEAdamSM"); ///ble_Serial.write to wypisanie wartosci w nawiasie na wyjscie do ble_serial (Tym komunikatem nakaze Bluetoothowi zmienic nazwe na AdamSM)
     delay(500);
     ble_Serial.write("AT+RESET");
     delay(500);
33
     SPI.begin();
     SPI.setDataMode(SPI_MODE0);
     SPI.setBitOrder(MSBFIRST);
     SPI.setClockDivider(SPI_CLOCK_DIV32);
//Program czeka 0,01s, a nastepnie zmienia stan pinu IRQPin na niski, odczeka 100 mikrosekund.
     delay(10);
     digitalWrite(IRQPin, LOW);
     delayMicroseconds(100);
     digitalWrite(IRQPin, HIGH);
     delay(10);
     digitalWrite(IRQPin, LOW);
}
//Ta funkcja resetujemy modu³ Bluetooth. Wynika to ze sposobu jego obs³ugi.
void restartBLE() {
     digitalWrite(BLEPin, HIGH);
     digitalWrite(5, HIGH);
     digitalWrite(6, HIGH);
     delay(500);
     ble_Serial.write("AT+RESET");
     delay(500);
}
// Ponizsza funkcja ustawia komunikacje miedzy NFC, a Arduino. Potem robi ma³y test i w zaleznosci od jego powodzenia ustawia zmienna globalna NFCReady, ktora bedzie informowa³a o aktualnym (tak na prawde ostatnio sprawdzonym) stanie komunikacji z NFC
void SetProtocol_Command() {
digitalWrite(SSPin, LOW); //Zmieniamy stan pinu SSPin na niski
//Zapis liczby w sposob 0x00 mowi, ze piszemy teraz systemem szesnastkowym czyli kolejno w linijkach uzywamy funkcji transfer z biblioteki SPI
//Zadajemy po kolei wartosci na 0, 2, 2, 1, D, gdzie D w zapisie
34
dziesietnym ma wartosc 13.
     SPI.transfer(0x00);
     SPI.transfer(0x02);
     SPI.transfer(0x02);
     SPI.transfer(0x01
     SPI.transfer(0x0D);
     digitalWrite(SSPin, HIGH);
     delay(1);
     digitalWrite(SSPin, LOW);
//Petla while (wykonuje sie do momentu az warunek w nawiasie jest TRUE lub 1)
while (RXBuffer[0] != 8) // RXBuffer to tablica elementow byte
// Tutaj sprawdzamy pierwszy element tablicy i dopoki jest inny niz osiem,
to petla sie wykonuje
{
// Podstawiamy pod pierwszy element tabeli odpowiedz BM19 na liczbe 3.
      RXBuffer[0] = SPI.transfer(0x03);
      RXBuffer[0] = RXBuffer[0] & 0x08;
// ^ Tutaj nastepuje koniunkcja bitowa (w systemie dwojkowym, natomiast zapisywana szesnastkowo)
}
digitalWrite(SSPin, HIGH); // Krotki skok na stan wysoki delay(1);
digitalWrite(SSPin, LOW);
//Tutaj wysy³amy dwojke i dostajemy odpowiedzi.
     SPI.transfer(0x02);
//Tutaj zapisujemy do RXuffera to co przyszlo w odpowiedzi
35
//Odpowiedz bedzie taka: SPI.transfer(buffer, size). Wiec pierwsza rzecz (buffer) wpiszemy na miejsce 0 w tablicy, a size na miejsce 1. (Tablice tez numeruje sie od 0, od lewej do prawej strony)
     RXBuffer[0] = SPI.transfer(0);
     RXBuffer[1] = SPI.transfer(0);
     digitalWrite(SSPin, HIGH);
//Jeli otrzymane dane maja wartosc zero, to wszystko jest w porzadku.
     if ((RXBuffer[0] == 0) & (RXBuffer[1] == 0))
     {
      Serial.println("NFC gotowy do pracy!");
      NFCReady = 1;
     }
else {
Serial.println("NFC nie skonfigurowany poprawnie!");
      NFCReady = 0;
     }
}
void Inventory_Command() {
     digitalWrite(SSPin, LOW);
     SPI.transfer(0x00);
     SPI.transfer(0x04);
     SPI.transfer(0x03);
     SPI.transfer(0x26);
     SPI.transfer(0x01);
     SPI.transfer(0x00);
     digitalWrite(SSPin, HIGH);
     delay(1);
     digitalWrite(SSPin, LOW);
//Petla identyczna jak ta z wczesniejszej funkcji, czyli wysy³amy 3, dopoki nie dostaniemy osemki.
36
     while (RXBuffer[0] != 8)
     {
      RXBuffer[0] = SPI.transfer(0x03);
      RXBuffer[0] = RXBuffer[0] & 0x08;
     }
     digitalWrite(SSPin, HIGH);
     delay(1);
     digitalWrite(SSPin, LOW);
     SPI.transfer(0x02);
     RXBuffer[0] = SPI.transfer(0);
     RXBuffer[1] = SPI.transfer(0);
//Powyzej znowu wysy³amy rzadanie przeczytania, zapisujemy pierwsze wartosci czyli bufor i rozmiar na miejscach 0 i 1.
//W tej petli program sprawdza wartosc na buforze i wype³nia otrzymanymi "pakietami" danych kolejne elementy na tablicy RXBufor
     for (byte i = 0; i<RXBuffer[1]; i++)
      RXBuffer[i + 2] = SPI.transfer(0);
     digitalWrite(SSPin, HIGH);
     delay(1);
//Jezeli wszystko by³o dobrze, to pierwsza wys³ana wartosc, tj. rozmiar bufora powinna byc 128. Jesli jest ok, to znowu zmieniamy ta globalna od stanu, na wartosc 2.
     if (RXBuffer[0] == 128)
     {
Serial.println("Sensor zakresie ... OK");
      NFCReady = 2;
     }
else {
      Serial.println("Sensor poza zakresem");
      NFCReady = 1;
     }
}
37
float Read_Memory() {
     byte oneBlock[8];
     String hexPointer = "";
     String trendValues = "";
     String hexMinutes = "";
     String elapsedMinutes = "";
     float trendOneGlucose;
     float trendTwoGlucose;
     float currentGlucose;
     float shownGlucose;
     float averageGlucose = 0;
     int glucosePointer;
     int validTrendCounter = 0;
     float validTrend[16];
     byte readError = 0;
     int readTry;
//Tutaj petla for, ktora wykona sie trzynascie razy zaczynamy od 3, bo bedziemy chcieli wysy³ac liczby od 3 w gore, zwiekszajac o 1 za kazdym wykonaniem (linijka 325)
     for (int b = 3; b < 16; b++) {
      readTry = 0;
//Petla do-while rozni sie od while tym, ze wykona sie za kazdym razem, pozniej na koncu sprawdzi warunek (zapisany przy s³owie while) jesli jest TRUE (1) to wykona sie kolejny raz. Jesli nie to przejdzie dalej.
do {
readError = 0; //Z kazdym wykoniem petli bedziemy ustawiac
zmienna na 0.
            digitalWrite(SSPin, LOW);
            SPI.transfer(0x00);
38
            SPI.transfer(0x04);
            SPI.transfer(0x03);
            SPI.transfer(0x02);
            SPI.transfer(0x20);
            SPI.transfer(b);
            digitalWrite(SSPin, HIGH);
            delay(1);
            digitalWrite(SSPin, LOW);
//Czekamy na jedynke na trzecim bicie (czyli np. liczbe (dziesietnie) od 8 do 15, 24-31 itd.)
            while (RXBuffer[0] != 8)
            {
                  RXBuffer[0] = SPI.transfer(0x03);
                  RXBuffer[0] = RXBuffer[0] & 0x08;
            }
            digitalWrite(SSPin, HIGH);
            delay(1);
//Czytamy co nam odpowiada, identycznie jak w funkcji Inventory_Command
            digitalWrite(SSPin, LOW);
            SPI.transfer(0x02);
            RXBuffer[0] = SPI.transfer(0);
            RXBuffer[1] = SPI.transfer(0);
            for (byte i = 0; i<RXBuffer[1]; i++)
            RXBuffer[i + 2] = SPI.transfer(0);
//Sprawdzenie czy ten bufor jest taki jak powinien, jesli nie to zglosi blad w odczycie.
            if (RXBuffer[0] != 128)
            readError = 1;
            digitalWrite(SSPin, HIGH);
            delay(1);
39
//Tutaj do tablicy oneBlock wrzucone zostaja elementy tablicy RXBuffer pomijajac jej pierwsze 3 komorki.
            for (int i = 0; i < 8; i++)
            oneBlock[i] = RXBuffer[i + 3];
            char str[24];
            unsigned char * pin = oneBlock;
//Unsigned char to taki sam char tylko bardziej pojemny, bo nie marnujemy jednego bitu na okreslenie znaku (+ czy -).
//Tutaj zapisujemy tablice oneBlock tak, ze jej wartosci beda dostepne pod "pin"
const char * hex = "0123456789ABCDEF"; //sta³y wskaznik na obiekty char ktorymi sa wszystkie cyfry w szesnastkowym
            char * pout = str;
//Ta petla wpisuje z kazdym wykonieniem dane w kolejne komorki tablicy pout, za kazdym wykoniem na dwie koljene
//Czyli w pierwszym na 0 i 1, w drugim na 2 i 3 itd.
//Dane wpisywane na pout przechodza "ciecie bitowe" czyli koniunkcje bitowa z liczba F (bitowo 1111) wiec dla kazdej liczby ucinamy bity powyzej trzeciego (numerowanie od zera), a reszte zerujemy.
//Dane w pin czyli w oneBlock (a te pochodza z RXBuffor) maja 8 bitow <od zerowego do siodmego> (sprawdzalismy czy RXBuffer[0] to 128 (128 to jedynka i siedem zer w dwojkowym))
//Pierwsze 4 bity <tj. bit 7,6,5,4> zapisujemy szesnastkowo w parzystej komorce tablicy pout(dla pierwszego wykonania w zerowej), nastepnie kolejne <tj. 3,2,1,0> w nieparzystych
//Ta petla przepisuje wartosci liczbowe z tablicy oneblock na ciag znakow typu char, ktore odpowiadaja wartosciom szesnastkowym tych liczb //Zapisujemy to za pomoca wskaznika pout na tablice str
            for (; pin < oneBlock + 8; pout += 2, pin++) {
            pout[0] = hex[(*pin >> 4) & 0xF];
40
// >> jest znakiem przesuniecia bitowego w prawo
            pout[1] = hex[*pin & 0xF];
            }
            pout[0] = 0;
//if wykona sie jesli readerror bedzie 0. ! to zaprzeczenie, wiec !0 ma wartosc logiczna 1 (TRUE), a dla !1 FALSE
if (!readError) {
Serial.println(str); //wysylamy na serial komunikat z
tablicy str
dopisujemy to co jest na tablicy str
trendValues += str; //Do stringa trendValues
}
            readTry++;
      } while ((readError) && (readTry < MAX_NFC_READTRIES));
// Tutaj dopiero konczy sie petla do-while i to jest jest jej warunek
}
readTry = 0;
//Ta petla jest prawie identyczna jak poprzednia, tyle ze zczytujemy dane z innej komorki, tym razem chodzi o czas jaki up³yna³ od ostatniego pomiaru
     do {
      readError = 0;
      digitalWrite(SSPin, LOW);
      SPI.transfer(0x00);
      SPI.transfer(0x04);
      SPI.transfer(0x03);
      SPI.transfer(0x02);
      SPI.transfer(0x20);
      SPI.transfer(39);
      digitalWrite(SSPin, HIGH);
41
delay(1);
digitalWrite(SSPin, LOW);
while (RXBuffer[0] != 8)
{
      RXBuffer[0] = SPI.transfer(0x03);
      RXBuffer[0] = RXBuffer[0] & 0x08;
}
digitalWrite(SSPin, HIGH);
delay(1);
digitalWrite(SSPin, LOW);
SPI.transfer(0x02);
RXBuffer[0] = SPI.transfer(0);
RXBuffer[1] = SPI.transfer(0);
for (byte i = 0; i<RXBuffer[1]; i++)
      RXBuffer[i + 2] = SPI.transfer(0);
if (RXBuffer[0] != 128)
      readError = 1;
digitalWrite(SSPin, HIGH);
delay(1);
for (int i = 0; i < 8; i++)
      oneBlock[i] = RXBuffer[i + 3];
char str[24];
unsigned char * pin = oneBlock;
const char * hex = "0123456789ABCDEF";
char * pout = str;
for (; pin < oneBlock + 8; pout += 2, pin++) {
pout[0] = hex[(*pin >> 4) & 0xF];
      pout[1] = hex[*pin & 0xF];
}
      pout[0] = 0;
      if (!readError)
            elapsedMinutes += str;
elapsedMinutes zawartosc tablicy str
// Tutaj dopisujemy do stringa
 readTry++;
} while ((readError) && (readTry < MAX_NFC_READTRIES));
42
     if (!readError)
     {
// dd = nazwa.substring(a,b) - ze stringa nazwa bierze znaki pomiedzy a i b, nastepnie wstawia je do zmiennej dd.
hexMinutes = elapsedMinutes.substring(10, 12) + elapsedMinutes.substring(8, 10);
hexPointer = trendValues.substring(4, 6);
// strtoul - str (string) to ul (liczba calkowita) - czyli zamienia ciag znakow na wartosci liczbowe
sensorMinutesElapse = strtoul(hexMinutes.c_str(), NULL, 16); glucosePointer = strtoul(hexPointer.c_str(), NULL, 16);
//Wypisanie wskazania glukozy
      Serial.println("");
      Serial.print("Wskazanie glukozy: ");
      Serial.print(glucosePointer);
      Serial.println("");
int ii = 0;
for (int i = 8; i <= 200; i += 12) {
            if (glucosePointer == ii)
            {
                  if (glucosePointer == 0)
                  {
String trendNow = trendValues.substring(190, 192) + trendValues.substring(188, 190);
String trendOne = trendValues.substring(178, 180) + trendValues.substring(176, 178);
String trendTwo = trendValues.substring(166, 168) + trendValues.substring(164, 166);
currentGlucose = Glucose_Reading(strtoul(trendNow.c_str(), NULL, 16));
43
trendOneGlucose = Glucose_Reading(strtoul(trendOne.c_str(), NULL, 16));
trendTwoGlucose = Glucose_Reading(strtoul(trendTwo.c_str(), NULL, 16));
                        if (FirstRun == 1)
                              lastGlucose = currentGlucose;
                        if (((lastGlucose - currentGlucose) > 50) ||
((currentGlucose - lastGlucose) > 50))
                        {
                              if (((lastGlucose - trendOneGlucose) > 50) ||
((trendOneGlucose - lastGlucose) > 50))
                                    currentGlucose = trendTwoGlucose;
                              else
                                    currentGlucose = trendOneGlucose;
} }
                  else if (glucosePointer == 1)
                  {
String trendNow = trendValues.substring(i - 10, i - 8) + trendValues.substring(i - 12, i - 10);
                        String trendOne =
+ trendValues.substring(188, 190);
                        String trendTwo =
+ trendValues.substring(176, 178);
currentGlucose = Glucose_Reading(strtoul(trendNow.c_str(), trendOneGlucose = Glucose_Reading(strtoul(trendOne.c_str(), trendTwoGlucose = Glucose_Reading(strtoul(trendTwo.c_str(),
trendValues.substring(190, 192)
trendValues.substring(178, 180)
NULL, 16));
NULL, 16));
NULL, 16));
                        if (FirstRun == 1)
                              lastGlucose = currentGlucose;
                        if (((lastGlucose - currentGlucose) > 50) ||
((currentGlucose - lastGlucose) > 50))
                        {
                              if (((lastGlucose - trendOneGlucose) > 50) ||
44
((trendOneGlucose - lastGlucose) > 50))
                                    currentGlucose = trendTwoGlucose;
} }
else {
String trendOne = - 20) + trendValues.substring(i - 24, i - String trendTwo = - 32) + trendValues.substring(i - 36, i -
currentGlucose = Glucose_Reading(strtoul(trendNow.c_str(), trendOneGlucose = Glucose_Reading(strtoul(trendOne.c_str(), trendTwoGlucose = Glucose_Reading(strtoul(trendTwo.c_str(),
trendValues.substring(i - 22, i
22);
trendValues.substring(i - 34, i
34);
NULL, 16));
NULL, 16));
NULL, 16));
else
      currentGlucose = trendOneGlucose;
String trendNow = trendValues.substring(i - 10, i - 8) + trendValues.substring(i - 12, i - 10);
                        if (FirstRun == 1)
                              lastGlucose = currentGlucose;
                        if (((lastGlucose - currentGlucose) > 50) ||
((currentGlucose - lastGlucose) > 50))
                        {
                              if (((lastGlucose - trendOneGlucose) > 50) ||
((trendOneGlucose - lastGlucose) > 50))
                                    currentGlucose = trendTwoGlucose;
} }
}
ii++; }
else
      currentGlucose = trendOneGlucose;
45
for (int i = 8, j = 0; i<200; i += 12, j++) {
String t = trendValues.substring(i + 2, i + 4) +
trendValues.substring(i, i + 2);
trend[j] = Glucose_Reading(strtoul(t.c_str(), NULL, 16));
}
      for (int i = 0; i<16; i++)
      {
if (((lastGlucose - trend[i]) > 50) || ((trend[i] - lastGlucose) > 50))
                  continue;
            else
{
validTrend[validTrendCounter] = trend[i]; validTrendCounter++;
} }
      if (validTrendCounter > 0)
      {
            for (int i = 0; i < validTrendCounter; i++)
                  averageGlucose += validTrend[i];
            averageGlucose = averageGlucose / validTrendCounter;
if (((lastGlucose - currentGlucose) > 50) || ((currentGlucose - lastGlucose) > 50))
                  shownGlucose = averageGlucose;
            else
shownGlucose = currentGlucose;
} else
if ((lastGlucose == currentGlucose) && (sensorMinutesElapse > 21000))
            noDiffCount++;
      if (lastGlucose != currentGlucose)
            noDiffCount = 0;
shownGlucose = currentGlucose;
46
      if (currentGlucose != 0)
            lastGlucose = currentGlucose;
      NFCReady = 2;
      FirstRun = 0;
//Funkcja zwraca do elementu ktory ja wywo³a³ wartosc shownGlucose lub wartosc 0, w zaleznosci od tego czy wymiana danych przebieg³a poprawnie.
      if (noDiffCount > 5)
            return 0;
      else
            return shownGlucose;
} else {
      Serial.print("Bledny odczyt");
      NFCReady = 0;
      readError = 0;
}
return 0; }
//Funkcja wezmie wartosc jaka jej wyslemy w val (musi to byc unsigned int- czyli int bez znaku (zawsze dodatni) czyli przedzia³ od zera do 65535), za³ozy maske bitowa o d³ugosci 12 bitow, podzieli przez 8 i po³.
//Po przepisaniu liczby na dwojkowy zeruje bity powyzej 11stego (zatem najwieksza liczba jaka przepusci niezmieniona to 4095 (dziesietnie), a najwieksza jaka zwroci to 481 z reszta.
float Glucose_Reading(unsigned int val) {
     int bitmask = 0x0FFF;
     return ((val & bitmask) / 8.5);
}
//Budowa ciagu znakow ktory bedziemy wysy³ac do aplikacji xDrip
47
//Przy wywo³ywaniu tej funkcji trzeba zadac jej wartosc glucose, funkcja po zakonczeniu zwraca wartosc string.
String Build_Packet(float glucose) {
//Utworzenie pustego stringa o nazwie packet, nastenie dopisywanie kolejnych elementow.
unsigned long raw = glucose * 1000;
String packet = "";
packet = String(raw); //po tej linijce packet zawiera tylko wartosc
raw, czyli glucose pomnozona przez 1000.
packet += ' ';
packet += "216";
packet += ' ';
packet += String(batteryPcnt); //Wstawienie tego co znajduje sie w
globalnej zmiennej batteryPcnt
     packet += ' ';
     packet += String(sensorMinutesElapse);
//Koniec budowy stringa
     Serial.println("");
     Serial.print("Poziom glukozy: ");
     Serial.print(glucose);
     Serial.println("");
     Serial.print("Trend 15sto minutowy");
     Serial.println("");
     for (int i = 0; i<16; i++)
     {
      Serial.print(trend[i]);
      Serial.println("");
     }
     Serial.print("Battery level: ");
     Serial.print(batteryPcnt);
     Serial.print("%");
     Serial.println("");
     Serial.print("Battery mVolts: ");
     Serial.print(batteryMv);
48
     Serial.print("mV");
     Serial.println("");
     Serial.print("Sensor lifetime: ");
     Serial.print(sensorMinutesElapse);
     Serial.print(" minutes elapsed");
     Serial.println("");
//Po tym wszystkim zwracamy utworzony pakiet temu co wywo³a³o nasza funkcje
     return packet;
}
void Send_Packet(String packet) {
//Jesli pierwszy znak stringa wynosi zero, to znaczy ze nieporawnie go zbudowano.
     if ((packet.substring(0, 1) != "0"))
     {
Serial.println("");
Serial.print("Pakiet do xDrip'a: ");
Serial.print(packet);
Serial.println("");
ble_Serial.print(packet); //Tutaj pakiet wysy³amy przez Bluetooth,
nie zwracamy serialem.
      delay(1000);
     }
else {
      Serial.println("");
Serial.print("Pakiet nie wys³any! Mozliwy b³edny odczyt lub sensor straci³ waznosc");
      Serial.println("");
      delay(1000);
     }
}
49
//Tutaj oblicza sie aktualne napiecie na zrodle zasilania i zwraca ta wartosc.
int readVcc() {
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
     ADMUX = _BV(MUX3) | _BV(MUX2);
#else
ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); #endif
     delay(2);
     ADCSRA |= _BV(ADSC);
     while (bit_is_set(ADCSRA, ADSC));
     uint8_t low = ADCL;
     uint8_t high = ADCH;
batteryMv = (high << 8) | low; // << przesuniecie bitowe o 8 w lewo
batteryMv = 1125300L / batteryMv;
int batteryLevel = min(map(batteryMv, MIN_V, MAX_V, 0, 100), 100); return batteryLevel;
}
void goToSleep(const byte interval, int time) {
SPI.end(); //Wyjscie NFC z trybu SPI (aktywnego urzadzenia peryferyjnego). Nastepnie wy³aczenie napiecia na ponizszych pinach.
     digitalWrite(MOSIPin, LOW);
     digitalWrite(SCKPin, LOW);
50
     digitalWrite(NFCPin1, LOW);
     digitalWrite(NFCPin2, LOW);
     digitalWrite(NFCPin3, LOW);
     digitalWrite(IRQPin, LOW);
     digitalWrite(5, LOW);
     digitalWrite(6, LOW);
     digitalWrite(BLEPin, LOW);
     digitalWrite(BLEState, LOW);
     for (int i = 0; i<time; i++) {
      MCUSR = 0;
//0b00011000 to oznaczenie liczby dwojkowej (tutaj jest to dziesietnie 24)
      WDTCSR |= 0b00011000;
      WDTCSR = 0b01000000 | interval;
//ponizej uzycie funkcji z bilioteki <avr/sleep.h> opsiane sa tutaj: http://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html
set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Tu ustawienie jaki tryb uspienia ma sie rozpoczac jesli dostanie rozkaz wy³aczenia
      sleep_enable();  //tutaj nakazuje mu sie uaktywnic.
      sleep_cpu(); //Uspienie
     }
}
ISR(WDT_vect)
{
     wdt_disable();
}
//Funkcja do wybudzania
void wakeUp() {
sleep_disable();
power_all_enable();
wdt_reset(); //Resetujemy zmiany na bitach WDTCR
 51
restartBLE();
//Czekamy na wybudzenie Bluetooth
for (int i = 0; ((i < MAX_BLE_WAIT) && (digitalRead(BLEState) != HIGH)); i++)
     {
      delay(1000);
      Serial.print("Czekam na polacznie z Bluetoothem.");
      Serial.println("");
}
//Tutaj za³aczamy napiecia na potrzebnych pinach
     digitalWrite(NFCPin1, HIGH);
     digitalWrite(NFCPin2, HIGH);
     digitalWrite(NFCPin3, HIGH);
     digitalWrite(IRQPin, HIGH);
     SPI.begin();
     SPI.setDataMode(SPI_MODE0);
     SPI.setBitOrder(MSBFIRST);
     SPI.setClockDivider(SPI_CLOCK_DIV32);
     delay(10);
     digitalWrite(IRQPin, LOW);
     delayMicroseconds(100);
     digitalWrite(IRQPin, HIGH);
     delay(10);
     digitalWrite(IRQPin, LOW);
//Ustawiamy wartosc NFCReady na zero, bo nie wazne jaka byla przed uspieniem, dopoki nie sprawdzimy na nowo zak³adamy ze nie dziala.
     NFCReady = 0;
}
//Przejscie w stan uspienia samowolnie z powodu niskiego napiecia, wszystko dzieje sie tak samo jak we wczesniejszych
//Po czasie uspienia wybudza sie i konczy dzialanie funkcji
52
void lowBatterySleep() {
     SPI.end();
     digitalWrite(MOSIPin, LOW);
     digitalWrite(SCKPin, LOW);
     digitalWrite(NFCPin1, LOW);
     digitalWrite(NFCPin2, LOW);
     digitalWrite(NFCPin3, LOW);
     digitalWrite(IRQPin, LOW);
     digitalWrite(5, LOW);
     digitalWrite(6, LOW);
     digitalWrite(BLEPin, LOW);
     Serial.print("Niski poziom baterii: ");
     Serial.print(batteryPcnt);
     Serial.print("%");
     Serial.println("");
delay(100);
     for (int i = 0; i<10; i++) {
      digitalWrite(SCKPin, HIGH);
      delay(50);
      digitalWrite(SCKPin, LOW);
      delay(100);
}
     MCUSR = 0;
     WDTCSR |= 0b00011000;
     WDTCSR = 0b01000000 | 0b100001;
     set_sleep_mode(SLEEP_MODE_PWR_DOWN);
     sleep_enable();
     sleep_cpu();
     sleep_disable();
     power_all_enable();
     wdt_reset();
}
//Nasza g³owna funkcja, ktora bedzie sie wykonywa³a w ko³ko
53
void loop() {
batteryPcnt = readVcc(); //Tutaj wywo³ujemy nasza funkcje readVcc i ona nam zwraca wartosc. Jesli < 1 to w petli while przechodzi w tryb spoczynku.
     if (batteryPcnt < 1)
      batteryLow = 1;
while (batteryLow == 1) //Jesli nie wykona³ sie poprzedni if, to petla sie nie rozpocznie.
{
lowBatterySleep(); //Rozpoczynamy funkcje uspienia z powodu niskiego
poziomu napiecia. Konczy sie ona jednak czesciowym wybudzeniem ukladu.
batteryPcnt = readVcc(); //Po tym pobudzeniu wywo³ujemy znowu funkcje odczytu napiecia.
     if (batteryPcnt > 10)
//Jesli jest juz powyzej 10% to odwo³ujemy stan "batteryLow" (petla while po dojsciu do konca nie wykona sie juz kolejny raz). Pozniej startujemy funkcje wakeup- do wybudzenia ukladu by zacza³ dzialac.
{
} }
batteryLow = 0;
wakeUp();
delay(100);
//Sprawdzamy czy NFC zosta³ uznany jako dzia³ajacy (wartosc NFCReady), jesli wartosc wynosi zero to zaczynamy dzia³ac w ifie
     if (NFCReady == 0)
     {
//Wywo³ujemy nasza funkcje SetProtocol_Command()
      SetProtocol_Command();
      delay(100);
54
}
     else if (NFCReady == 1)
     {
      for (int i = 0; i<3; i++) {
            Inventory_Command();
            if (NFCReady == 2)
                  break;
            delay(1000);
      }
//Jesli po 3 probach dalej nie ma niczego w zasiegu NFC to przechodzi w tryb uspienia
      if (NFCReady == 1) {
            goToSleep(0b100001, SLEEP_TIME);  // wywo³anie funkcji
uspienia z takimi parametrami jak podane
wakeUp(); //Wywo³anie funkcji do wybudzenia
            delay(100); //czekamy 0,1s
      }
} else {
String xdripPacket = Build_Packet(Read_Memory()); // Stworzenie stringa, ktory przyjmie wartosc taka jak zwroci mu funkcja Build_Packet. Ta funkcje odpali sie z parametrem w nawiasie, ale zeby go wyliczyc najpierw processor uruchomi funkcje Read_Memory, ktora po zakonczeniu zwroci wartosc. Ta wartosc uzyje funkcja Build_Packet, ktora zwroci jeszcze inna wartosc, ktora ostatecznie znajdzie sie w tym stringu.
Send_Packet(xdripPacket); //Tutaj wywo³ujemy funkcje Send_Packet z argumentem w postaci stringa utworzonego linie wyzej.
goToSleep(0b100001, SLEEP_TIME); //Tryb uspienia wakeUp();
delay(100);
} }