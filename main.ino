/*MOMS (Machine Online Monitoring System) v0.5
  Author: Chetan Singh
  Created: 15.04.2017
  Updated: 07.05.2017
*/

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include "RTClib.h"

//Some Variables
const int ONhr = 0;
const int ONmnt = 1;
const int OFFhr = 2;
const int OFFmnt = 3;
const int daySt = 4;
const int wLED = 13;
const int dbLED = 12;
const int dataLED = 14;
const int mcIn = 16;
int crState;
int preState;
bool connSt;
int sec = 30, t_sec = 0;
int mnt, t_mnt = 1, hr;
int writeSt;
int tot_on_mnt;
int tot_off_mnt;
int tot_on_hr;
int tot_off_hr;
String str;

// Declaring MySQL Server Variables
IPAddress server_addr(192, 168, 43, 36);
char user[] = "rpi";
char password[] = "password";

// WiFi Connection Parameters
char ssid[] = "Chetan's WiFi";
char pass[] = "sunny999";

WiFiClient client;
WiFiServer server(80);
MySQL_Connection conn((Client *)&client);
RTC_DS1307 RTC;

char INSERT_SQL_ON[] = "INSERT INTO test.data_node (mcstatus, binary_status, mc_id) VALUES ('ON', 1, '12')";
char INSERT_SQL_OFF[] = "INSERT INTO test.data_node (mcstatus, binary_status, mc_id) VALUES ('OFF', 0, '12')";
char INSERT_SQL_TOT[] = "INSERT INTO test.total_time_info (mc_id, total_start_time, total_stop_time) VALUES ('%s', '%s', '%s')";
char totQuery[128];

void setup()
{
  RTC.begin();
  delay(500);
  delay(1000);

  pinMode(mcIn, INPUT);
  pinMode(wLED, OUTPUT);
  pinMode(dbLED, OUTPUT);
  pinMode(dataLED, OUTPUT);

  digitalWrite(wLED, LOW);
  digitalWrite(dbLED, LOW);
  digitalWrite(dataLED, LOW);

  Serial.begin(9600);
  EEPROM.begin(512);

  writeSt = EEPROM.read(daySt);
  tot_on_mnt = EEPROM.read(ONmnt);
  tot_off_mnt = EEPROM.read(OFFmnt);
  tot_on_hr = EEPROM.read(ONhr);
  tot_off_hr = EEPROM.read(OFFhr);
  delay(500);
  connectWIFI();
  connectDB();

  server.begin();
  Serial.println("Server started");
}

void loop() {

  DateTime now = RTC.now();
  crState = digitalRead(mcIn);

  //-----------------------------------------------Server Section----------------------------------------------------------------------------------------------------

  WiFiClient client2 = server.available();
  int chk = 0;
  String webMsg = "";
  if (client2.available()) {
    Serial.println("New client");
    String request = client2.readStringUntil('\r');
    Serial.println(request);
    client2.flush();

    // Match the request
    if (request.indexOf("/command=RESET") != -1)  {
      if (crState == 1) {
        chk = 1;
      } else if (crState == 0) {
        WiFi.disconnect();
        ESP.eraseConfig();
        ESP.restart();
      }
    } else {
      if (crState == 1) {
        webMsg = "<b style=\"color:green\"> Since ON </b>";
      } else {
        webMsg = "<b style=\"color:red\"> Since OFF </b>";
      }
    }
    client2.println("HTTP/1.1 200 OK");
    client2.println("Content-Type: text/html");
    client2.println("");
    client2.println("<!DOCTYPE HTML>");
    client2.println("<html>");

    client2.print("<b style=\"color:blue\"> <font size = \"5\"> Message from System 12: </font> </b> <br><br>");
    client2.print("<b> RTC Time: </b>");
    client2.print(now.hour());
    client2.print(":");
    client2.print(now.minute());
    client2.print(":");
    client2.print(now.second());
    client2.print("<br><br>");
    client2.print("<b> Status: </b>");
    client2.print(str);
    client2.print(", ");
    client2.print(webMsg);
    client2.print("<br>");

    if (chk == 1) {
      client2.print("<br>");
      client2.print("<b style=\"color:red\"> <font size = \"4\"> Can't RESET, the Machine is in ON State </font> </b>");
      client2.print("<br>");
    }

    client2.println("<br>");
    client2.println("<a href=\"/command=RESET\"\"><button> RESET SYSTEM </button></a>");
    client2.println("</html>");

    delay(1);
    Serial.println("Client disonnected");
    Serial.println("");
  } else {
    delay(1);
  }

  //----------------------------------------------Server Section Ends------------------------------------------------------------------------------------------------

  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print(" ");

  if (now.hour() == 23 && now.minute() == 59 && now.second() == 0) {
    Serial.println("Day Changed; Data Cleared");
    t_sec += 30;
    tot_off_hr = 0;
    tot_off_mnt = 0;
    tot_on_hr = 0;
    tot_on_mnt = 0;
    EEPROM.write(ONhr, 0);
    EEPROM.write(ONmnt, 0);
    EEPROM.write(OFFhr, 0);
    EEPROM.write(OFFmnt, 0);
    EEPROM.write(daySt, 0);
    EEPROM.commit();
  }

  //Checking the connection to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(wLED, HIGH);
  } else {
    digitalWrite(wLED, LOW);
    digitalWrite(dbLED, LOW);
    digitalWrite(dataLED, LOW);
    connectWIFI();
    connectDB();
  }

  if (crState == preState) {
    if (sec > 59) {
      if (t_sec > 59) {
        t_sec = 0;
        t_mnt++;
      }
      mnt = sec / 60;
      hr = mnt / 60;
      if (t_mnt > 59) {
        t_mnt = 0;
      }
      str = hr;
      str += "h ";
      str += t_mnt;
      str += "m ";
      str += t_sec;
      str += "s";
      Serial.println(str);
      t_sec++;
    } else {
      str = hr;
      str += "h ";
      str += mnt;
      str += "m ";
      str += sec;
      str += "s";
      Serial.println(str);
    }
    if (now.hour() % 3 == 0 && now.minute() == 00 && now.second() == 0) {
      if (crState == 1) {
        tot_on_hr += hr;
        tot_on_mnt += t_mnt;
      } else if (crState == 0) {
        tot_off_hr += hr;
        tot_off_mnt += t_mnt;
      }
      String dayStrON, dayStrOFF;
      dayStrON += tot_on_hr;
      dayStrON += "h ";
      dayStrON += tot_on_mnt;
      dayStrON += "m ";

      dayStrOFF += tot_off_hr;
      dayStrOFF += "h ";
      dayStrOFF += tot_off_mnt;
      dayStrOFF += "m ";
      Serial.println(dayStrON);
      Serial.println(dayStrOFF);

      writeDayTotal(dayStrON, dayStrOFF);
    } else if (now.hour() > 17 && now.hour() < 19 && now.minute() == 30) {
      writeSt = EEPROM.read(daySt);
      if (writeSt == 0) {
        if (crState == 1) {
          tot_on_hr += hr;
          tot_on_mnt += t_mnt;
        } else if (crState == 0) {
          tot_off_hr += hr;
          tot_off_mnt += t_mnt;
        }
        String dayStrON, dayStrOFF;
        dayStrON += tot_on_hr;
        dayStrON += "h ";
        dayStrON += tot_on_mnt;
        dayStrON += "m ";

        dayStrOFF += tot_off_hr;
        dayStrOFF += "h ";
        dayStrOFF += tot_off_mnt;
        dayStrOFF += "m ";
        Serial.println(dayStrON);
        Serial.println(dayStrOFF);

        writeDayTotal(dayStrON, dayStrOFF);
      }
    }
    sec++;
  } else if (crState != preState) {
    if (crState == 1) {
      tot_off_hr += hr;
      tot_off_mnt += t_mnt;
      EEPROM.write(OFFhr, tot_off_hr);
      EEPROM.write(OFFmnt, tot_off_mnt);
      EEPROM.commit();
      Serial.println("ON");
      writeON();
      preState = crState;
    } else if (crState == 0) {
      tot_on_hr += hr;
      tot_on_mnt += t_mnt;
      EEPROM.write(ONhr, tot_on_hr);
      EEPROM.write(ONmnt, tot_on_mnt);
      EEPROM.commit();
      Serial.println("OFF");
      writeOFF();
      preState = crState;
    }
    sec = 30;
    mnt = 0;
    hr = 0;
    t_sec = 0;
    t_mnt = 1;
    closeDB();
  }
  delay(1000);
}

void connectDB() {
  Serial.println("Connecting to database...");
  if (conn.connect(server_addr, 3306, user, password)) {
    connSt = true;
    Serial.println("DB Connection Created");
  } else {
    Serial.println("A Connection Could not be Established with Database, Trying Again");
    blinkLED(dbLED, 5);
    connSt = false;
    connectDB();
  }
  digitalWrite(dbLED, HIGH);
}

void closeDB() {
  conn.close();
  connSt = false;
  digitalWrite(dbLED, LOW);
  Serial.println("DB Connection Closed");
}

void connectWIFI() {
  WiFi.begin(ssid, pass);
  int count = 0;
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    if (count > 20) {
      count = 0;
      WiFi.disconnect();
      ESP.eraseConfig();
      ESP.restart();
    }
    blinkLED(wLED, 1);
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print ("IP address: ");
  Serial.println (WiFi.localIP());
  WiFi.mode(WIFI_STA);
  digitalWrite(wLED, HIGH);
}

void blinkLED(int pin, int rounds) {
  for (int i = 0; i < rounds; i++) {
    digitalWrite(pin, HIGH);
    delay (300);
    digitalWrite(pin, LOW);
    delay (300);
  }
}

void writeON() {
  if (connSt == false) {
    connectDB();
  }
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(INSERT_SQL_ON);
  delete cur_mem;
  blinkLED(dbLED, 5);
}

void writeOFF() {
  if (connSt == false) {
    connectDB();
  }
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(INSERT_SQL_OFF);
  delete cur_mem;
  blinkLED(dbLED, 5);
}

void writeDayTotal(String x, String y) {
  if (connSt == false) {
    connectDB();
  }
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  int on_len = x.length() + 1;
  int off_len = y.length() + 1;
  char ON_T[on_len];
  char OFF_T[off_len];

  x.toCharArray(ON_T, on_len);
  y.toCharArray(OFF_T, off_len);
  sprintf(totQuery, INSERT_SQL_TOT, "12", ON_T, OFF_T);
  cur_mem->execute(totQuery);
  blinkLED(dataLED, 5);

  t_sec += 30;
  tot_off_hr = 0;
  tot_off_mnt = 0;
  tot_on_hr = 0;
  tot_on_mnt = 0;
  EEPROM.write(ONhr, 0);
  EEPROM.write(ONmnt, 0);
  EEPROM.write(OFFhr, 0);
  EEPROM.write(OFFmnt, 0);
  EEPROM.write(daySt, 1);
  EEPROM.commit();
  delete cur_mem;
}
