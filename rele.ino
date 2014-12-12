#include <EtherCard.h>
#include "DHT.h"

#define DHTPIN A0
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

int maxtemp = -100,mintemp = 100;
int maxhum = -100, minhum = 100;
int hum = 35;
int temp = 25;

static byte mymac[] = { 0x54,0x55,0x58,0x10,0x00,0x24  };
static byte myip[] = { 172,16,56,3 };

#define BUFFER_SIZE   500
byte Ethernet::buffer[BUFFER_SIZE];
BufferFiller bfill;

#define CS_PIN       10

#define RELAIS_1     7
#define RELAIS_2     8
bool relais1Status = true;
bool relais2Status = false;

const char http_OK[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"Pragma: no-cache\r\n\r\n";

const char http_Found[] PROGMEM =
"HTTP/1.0 302 Found\r\n"
"Location: /\r\n\r\n";

const char http_Unauthorized[] PROGMEM =
"HTTP/1.0 401 Unauthorized\r\n"
"Content-Type: text/html\r\n\r\n"
"<h1>401 Unauthorized</h1>";

void homePage()
{
  int hum = dht.readHumidity(); //Le o valor da umidade
  int temp = dht.readTemperature(); //Le o valor da temperatura
  bfill.emit_p(PSTR("$F"
    "<title>Arduino</title>" 
    "Rel 1: <a href=\"?relais1=$F\">$F</a><br />"
    "LED 1: <a href=\"?relais2=$F\">$F</a><br />"
    "<br />"
    ),
  http_OK,
  relais1Status?PSTR("off"):PSTR("on"),
  relais1Status?PSTR("<font color=\"green\"><b>ON</b></font>"):PSTR("<font color=\"red\">OFF</font>"),
  relais2Status?PSTR("off"):PSTR("on"),
  relais2Status?PSTR("<font color=\"green\"><b>ON</b></font>"):PSTR("<font color=\"red\">OFF</font>"));
  bfill.emit_p(PSTR("Temperatura: $D <br/><br/>"
    "Temp Maxima: $D <br/>"
    "Temp Minima: $D <br/><br/>"
    "Humidade: $D <br/><br/>"
    "Hum Maxima: $D <br/>"
    "Hum Minima: $D <br/><br/>"),
    temp,
    maxtemp,
    mintemp,
    hum,
    maxhum,
    minhum
  );
}

void setup()
{
  Serial.begin(115200);  

  dht.begin();

  pinMode(RELAIS_1, OUTPUT);
  pinMode(RELAIS_2, OUTPUT);

  if (ether.begin(BUFFER_SIZE, mymac, CS_PIN) == 0)
    Serial.println("Cannot initialise ethernet.");
  else
    Serial.println("Ethernet initialised.");

  ether.staticSetup(myip);
    
  Serial.println("Setting up DHCP");
  if (!ether.dhcpSetup())
    Serial.println( "DHCP failed");
  
  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);

}

void loop()
{
  int hum = dht.readHumidity(); //Le o valor da umidade
  int temp = dht.readTemperature(); //Le o valor da temperatura
  
  if(temp > maxtemp) {maxtemp = temp;}
  if(temp < mintemp) {mintemp = temp;}
  if(hum > maxhum) {maxhum = hum;}
  if(hum < minhum) {minhum = hum;}
  
  digitalWrite(RELAIS_1, relais1Status); 
  digitalWrite(RELAIS_2, relais2Status); 

  delay(1);   // necessary for my system
  word len = ether.packetReceive();   // check for ethernet packet
  word pos = ether.packetLoop(len);   // check for tcp packet

  if (pos) {
    bfill = ether.tcpOffset();
    char *data = (char *) Ethernet::buffer + pos;
    if (strncmp("GET /", data, 5) != 0) {
      // Unsupported HTTP request
      // 304 or 501 response would be more appropriate
      bfill.emit_p(http_Unauthorized);
    }
    else {
      Serial.print("----");
      Serial.print(data);
      Serial.println("----");
      data += 5;

      if (data[0] == ' ') {
        // Return home page
        homePage();
      }
      else if (strncmp("?relais1=on ", data, 12) == 0) {
        relais1Status = true;        
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?relais2=on ", data, 12) == 0) {
        relais2Status = true;        
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?relais1=off ", data, 13) == 0) {
        relais1Status = false;        
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?relais2=off ", data, 13) == 0) {
        relais2Status = false;        
        bfill.emit_p(http_Found);
      }
      else {
        // Page not found
        bfill.emit_p(http_Unauthorized);
      }
    }

    ether.httpServerReply(bfill.position());    // send http response
  }
}
