//librerias
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <DHT.h>
#include <DHT_U.h>


WiFiClient espClient;
PubSubClient client(espClient);

// Sustituir con datos tu red------------------------------------------------------------------------------------------------------------------------
const char* ssid     = "Juan Jose";
const char* password = "pacoymono";

// Credenciales de tu servidor MQTT------------------------------------------------------------------------------------------------------------------------
const char *mqtt_server = "test.mosquitto.org";
const char *mqtt_username = "rw";
const char *mqtt_password = "readwrite";
const int mqtt_port = 1883;

// Tópicos e suscritos------------------------------------------------------------------------------------------------------------------------
const char *topic1 = "Fb/led";
const char *topic2 = "Fb/intensidad";
const char *topic3 = "Fb/ventilador";
//Topicos publicados------------------------------------------------------------------------------------------------------------------------
const char *topic4 = "Fb/Temperatura";
const char *topic5 = "Fb/Calor";
const char *topic6= "Fb/Sensor";
const char *topic7= "Fb/Conteo";

//variables------------------------------------------------------------------------------------------------------------------------

//Led
#define led 2
//Pwm ------------------------------------------------------------------------------------------------------------------------
const int Pwm_pin = 2;
const int freq = 5000;
const int led_channel = 0;
const int resolution = 8;
String slider_value = "0";
//Ventilador------------------------------------------------------------------------------------------------------------------------
#define vent 23

//Temperatura------------------------------------------------------------------------------------------------------------------------
int temp = 22;			
int TEMPERATURA;
int HUMEDAD;

//Tipo de sensor
DHT dht(temp, DHT11);

//Sensor de movimiento------------------------------------------------------------------------------------------------------------------------
#define sensorPir 21
int estado = LOW; //sin movimiento
int valor = 0; //almacena valor PIR
#define ledSensor 4
int contador =0;



void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Llegó un mensaje del tópico: ");
  Serial.print(topic);
  Serial.print(". Mensaje: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();



   //Leds------------------------------------------------------------------------------------------------------------------------
     if (String(topic) == topic1)
    {
    Serial.print("Cambiando la salida a:");
    if (messageTemp == "100")
    {
      Serial.println("Led encendido");
      digitalWrite(led, HIGH);
    }
    else if (messageTemp == "0")
    {
      Serial.println("Led apagado");
      digitalWrite(led, LOW);
    }
    }
    //Intesidad de los leds------------------------------------------------------------------------------------------------------------------------
   if (String(topic) == topic2 )
  {
    slider_value = messageTemp;
    ledcWrite(led_channel, slider_value.toInt());
  }

  Serial.println();
   
   //ventilador------------------------------------------------------------------------------------------------------------------------
  if (String(topic) == topic3)
    {
    if (messageTemp == "on")
    {
      Serial.println("Ventilador encendido");
      digitalWrite(vent, HIGH);
    }
    else if (messageTemp == "off")
    {
      Serial.println("Ventilador apagado");
      digitalWrite(vent, LOW);
    }
     
    }

}
void reconnect()
{
  // Loop hasta que se conecte------------------------------------------------------------------------------------------------------------------------
  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.print("Intentando conexión MQTT..");
    // Intento de reconección
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("¡conectado!");
      client.subscribe(topic1); // Subscripción al tópico del led
      client.subscribe(topic2); // Subscripción al tópico del pwm
      client.subscribe(topic3); // Subscripción al tópico del ventilador
      
      
    }
    else
    {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentar otra vez en 5 segundos");
      // Espera 5 segundos y vuelve a intentar
      delay(5000);
    }
  }
}

void setup_wifi()
{

  delay(10);
  // Conexión a tu red Wi-Fi------------------------------------------------------------------------------------------------------------------------
  Serial.println();
  Serial.print("Conectándose a: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Dirección IP del ESP32: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
}

void setup()
{

  //estado led------------------------------------------------------------------------------------------------------------------------
  pinMode(led,OUTPUT);
  digitalWrite(led,LOW);
  
   // Slider (pwm)------------------------------------------------------------------------------------------------------------------------
  ledcSetup(led_channel, freq, resolution);
  ledcAttachPin(Pwm_pin, led_channel);
  ledcWrite(led_channel, slider_value.toInt());
  
  //estado vent------------------------------------------------------------------------------------------------------------------------
  pinMode(vent,OUTPUT);
  digitalWrite(vent,LOW);

  //temperatura------------------------------------------------------------------------------------------------------------------------
  dht.begin();
   
   //Sensor movimiento------------------------------------------------------------------------------------------------------------------------
  pinMode(ledSensor, OUTPUT);
  pinMode(sensorPir, INPUT);

  Serial.begin(9600);
  delay(10);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a:\t");
  Serial.println(ssid); 

  // Esperar a que nos conectemos------------------------------------------------------------------------------------------------------------------------
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(200);
   Serial.print('.');
  }

  // Mostrar mensaje de exito y dirección IP asignada------------------------------------------------------------------------------------------------------------------------
  Serial.println();
  Serial.print("Conectado a:\t");
  Serial.println(WiFi.SSID()); 
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() 
{

 if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // obtencion de valor de temperatura------------------------------------------------------------------------------------------------------------------------
  TEMPERATURA = dht.readTemperature();

  // obtencion de valor de humedad------------------------------------------------------------------------------------------------------------------------
  HUMEDAD = dht.readHumidity();		

  //Imprimir temperatura en grados------------------------------------------------------------------------------------------------------------------------
  char temperatura[8];
  dtostrf(TEMPERATURA, 1, 2, temperatura);
  client.publish(topic4, temperatura);


  //Imprimir porcentaje de calor------------------------------------------------------------------------------------------------------------------------
  float hic = dht.computeHeatIndex(TEMPERATURA,HUMEDAD,false);
   char calor[8];
  dtostrf(hic, 1, 2, calor);
  client.publish(topic5, calor);

 //si hace mucho calor me activa el ventilador------------------------------------------------------------------------------------------------------------------------
  if (hic>=28 ){
       Serial.println("Ventilador encendido");
      digitalWrite(vent, HIGH);
     }
  else{

    digitalWrite(vent, LOW);
  }
 
 //sensor movimiento------------------------------------------------------------------------------------------------------------------------
  
  estado = digitalRead(sensorPir);	// lectura de estado de señal
  
  if (estado == HIGH){		// si la señal esta en alto indicando movimiento
    digitalWrite(ledSensor, HIGH);
    contador = contador+1;
    client.publish(topic6, "hay alguien en la puerta");
    delay(10000);	
    client.publish(topic6, "");	
  }
  
   else {
    digitalWrite(ledSensor, LOW);
    
  }
  if(contador>0) {
  char conteo[8];
  dtostrf(contador, 1, 2, conteo); // cuenta cuantas personas te han visitado
  client.publish(topic7, conteo);
  
  }
     
}