#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <Time.h>
#include <ESP8266TrueRandom.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <SoftwareSerial.h>


#define FIREBASE_HOST "cardetection-50617.firebaseio.com"
#define FIREBASE_AUTH "pOluA5aMXplPQ2Bv6cgs1UvKDhCkiEYXm3eHrPVY"

String ssid     = "";
const char* password = "t0llg4t3";
const int buzzPin = 15;

int timezone = 7 * 3600;
int dst = 0;

String stats = "out";
String in = "default";
String out = "default";
String isPaid = "false";
int chipId;
int mulai, selesai, total;
String apName, tarif;
String plat = "default";
String jenis = "default";
String dateIn, dateOut;
byte uuidNumber[8];

LiquidCrystal_I2C lcd(0x27,16,2);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buzzPin,OUTPUT);
  Serial.begin(9600);  
  lcd.begin();
  lcd.noBacklight();
  lcd.display();
  chipId = ESP.getChipId();
  connWifi();
  readDataKendaraan();
}

void loop() {
  mulai = getTime().toInt();
  Serial.print("Waktu Mulai Layanan : ");
  Serial.println(mulai);
  Serial.println("Status : " +stats);
  Serial.println("Gate In : " +in +" Gate Out : " +out); 
  Serial.println("platNo : " +plat +" tipe : " +jenis);
  ESP8266TrueRandom.uuid(uuidNumber);
  String uuidStr = ESP8266TrueRandom.uuidToString(uuidNumber);
  Serial.println("The UUID number is " + uuidStr);
  
  //sendDataDetect(stats,in,out); 
  if(stats == "out" && in != "default" && out != "default"){
    displayLcd();
    readDataTarif(in,out,jenis);
    Serial.println("Tarif : " +tarif);
    sendDataTrans(in,out,jenis,tarif,uuidStr,isPaid, dateIn, getDate());
    out = "default"; in = "default";
    selesai = getTime().toInt();
    Serial.print("Waktu Selesai Layanan : ");
    Serial.println(selesai);
    total = selesai - mulai;
    Serial.print("Waktu Layanan : ");
    Serial.print(total);
    Serial.println(" detik");
    Serial.println();
    WiFi.disconnect();
    delay(100);
    delay(10000);
  }else if(stats == "in"){
    beep2();
    displayLcd();
    selesai = getTime().toInt();
    Serial.print("Waktu Selesai Layanan : ");
    Serial.println(selesai);
    total = selesai - mulai;
    Serial.print("Waktu Layanan : ");
    Serial.print(total);
    Serial.println(" detik");
    Serial.println();
    dateIn = getDate();
    WiFi.disconnect();
    delay(10000);
  }else{
    lcd.noBacklight();
    displayLcd();
    WiFi.disconnect();
    delay(10000);
  }
  delay(1000);  
  while (WiFi.status() != WL_CONNECTED){
    if(stats == "in"){
      connWifi();  
    }else{
      connWifi();  
      out = "default";
    }
  }
}

//Connect to database
void firebasereconnect(){
  Serial.println("Trying to reconnect");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.print("Connect to Host : ");
  if (Firebase.failed()) {
    Serial.println("Failed");
  }else{
    Serial.println("Success");
  }
}

//Create a connection to selected network
void connWifi(){
  // We start by connecting to a WiFi network
  delay(1000);
  WiFi.disconnect();
  Serial.println();
  Serial.println();
  ssid = "";
  scanNetSort();

  if(getValue(ssid,'_',0)=="in" || getValue(ssid,'_',0)=="out"){
    beep();
    lcd.noBacklight();
    lcd.display();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connecting");
    Serial.print("Connecting to ");
    Serial.println(ssid);
  
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);  
  }else{
    lcd.noDisplay();
    Serial.println("There is no connection");
    connWifi();
  }
  

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("");
    lcd.clear();
    Serial.println("WiFi connected");
    
    configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");
    Serial.println("\nWaiting for Internet time");
  
    while(!time(nullptr)){
       Serial.print("*");
       delay(1000);
    }

    firebasereconnect();
  }
  

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.print("Connect to Host : ");
  if (Firebase.failed()) {
    Serial.println("Failed");
    firebasereconnect();
  }else{
    Serial.println("Success");
  }

  splitApName(ssid);

  if(plat == "default")
    readDataKendaraan();
  
}

void displayLcd(){
  lcd.setCursor(0,0);
  lcd.print("St:");
  lcd.setCursor(3,0);
  lcd.print(stats);
  lcd.setCursor(7,0);
  lcd.print("in:");
  lcd.setCursor(10,0);
  lcd.print(in);
  lcd.setCursor(0,1);
  lcd.print("out:");
  lcd.setCursor(4,1);
  lcd.print(out);
}

void beep(){
  digitalWrite(LED_BUILTIN, HIGH);
  tone(buzzPin,5000);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  noTone(buzzPin);
}

void beep2(){
  digitalWrite(LED_BUILTIN, HIGH);
  tone(buzzPin,5000);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  noTone(buzzPin);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  tone(buzzPin,5000);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  noTone(buzzPin);
}

void beep3(){
  digitalWrite(LED_BUILTIN, HIGH);
  tone(buzzPin,5000);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  noTone(buzzPin);
}

//Scan Network using RSSI 
void scanNet(){
  int numberOfNetworks = WiFi.scanNetworks();
  int indices[numberOfNetworks];
  for(int i =0; i<numberOfNetworks; i++){
      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
      String a = getValue(WiFi.SSID(i),'_',0);
      if(a == "in" || a == "out"){
        ssid = WiFi.SSID(i);  
      }
  }
}

void scanNetSort(){
  int n = WiFi.scanNetworks();
  if(n==0){
    Serial.println("There is no Network");
  }else{
    int indices[n];
    for(int i=0; i<n; i++){
      indices[i] = i;
    }
    
    for(int i=0; i<n; i++){
      for(int j=0; j<n; j++){
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }

    for(int i=0; i<n; i++){
      Serial.println("The Strongest Network : ");
      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
      String a = getValue(WiFi.SSID(i),'_',0);
      if(a == "in" || a == "out"){
        ssid = WiFi.SSID(i);  
      }
    }
  }
  
}

//Read vehicle data from DB
void readDataKendaraan(){
  String st = "/DataKendaraan/" ;
  String a = "/";
  String path = st +chipId +a;
  Serial.println("String path : " +path);
  Serial.print("Connect to DB : ");
  FirebaseObject object = Firebase.get(path);
  if(Firebase.failed()){
    Serial.println("Failed");
  }else{
    Serial.println("Success");
  }
  plat = object.getString("platNo");
  jenis = object.getString("tipe");
}

void readDataTarif(String in, String out, String jenis){
  String st = "/DataTarif/" ;
  String a = "/";
  String path = st +in +a +out +a +jenis +a;
  Serial.println("String path : " +path);
  Serial.print("Connect to DB : ");
  FirebaseObject object = Firebase.get(path);
  if(Firebase.failed()){
    Serial.println("Failed");
  }else{
    Serial.println("Success");
  }
  tarif = object.getInt("tarif");
}

//Send detection data to DB
void sendDataDetect(String a, String b, String c){
  String parent = "/DataDetect/";
  String del = "/";
  String path = parent +chipId +del;
  Serial.println("path : " +path);
  Firebase.setString(path+"status",a);
  Firebase.setString(path+"gateIn",b);
  Firebase.setString(path+"gateOut",c);
  if(Firebase.failed()){
    Serial.println("Send Data Failed"); 
  }else{
    Serial.println("Send Data Success");
  }
}

void sendDataTrans(String in, String out, String jenis, String tarif, String randString, String isPaid, String dateIn, String dateOut){
  String parent = "/DataTransaksi/";
  String a = "/";
  String path = parent +chipId +a +randString +a;
  Firebase.setString(path+"gateIn",in);
  Firebase.setString(path+"gateOut",out);
  Firebase.setString(path+"tipe",jenis);
  Firebase.setString(path+"tarif",tarif);
  Firebase.setString(path+"isPaid",isPaid);
  Firebase.setString(path+"dateIn",dateIn);
  Firebase.setString(path+"dateOut",dateOut);
  if(Firebase.failed()){
    Serial.println("Failed");
    beep3();
  }else{
    Serial.println("Success");
    beep2();
  }
}

void splitApName(String text){
  stats = getValue(text,'_',0);
  if(stats == "in"){
    in = getValue(text,'_',1);
  }else{
    out = getValue(text,'_',1);
  }
}

String getTime(){
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);

  int j = p_tm->tm_hour;
  int m = p_tm->tm_min;
  int d = p_tm->tm_sec;
  String jam = String(j);
  String men = String(m);
  String det = String(d);
  String waktu = jam +men +det; 
  return waktu;
}

String getDate(){
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);

  int t = p_tm->tm_mday;
  int b = p_tm->tm_mon + 1;
  int th = p_tm->tm_year + 1900;
  int j = p_tm->tm_hour;
  int m = p_tm->tm_min;
  int d = p_tm->tm_sec;
  
  String tgl = String(t);
  String bln = String(b);
  String thn = String(th);
  String jam = String(j);
  String men = String(m);
  String det = String(d);
  String tanggal = tgl +"/" +bln +"/" +thn +" ";
  String waktu = jam +":" +men +":" +det; 
  String date = tanggal + waktu;
  return date;
}

String getValue(String data, char separator, int index){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
