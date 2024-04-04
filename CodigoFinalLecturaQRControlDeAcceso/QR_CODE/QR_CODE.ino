// Original software Copyright (c) 2020 Alvaro Viebrantz
// Added relay and compare names function by Davide Gatti www.survivalhacking.it 
// Added comprobarIdentidad and SD function by Manuel Torrealba, Peter Litwin, Nerio Alvarado, José Alcala

//Libraries
#include <Arduino.h>
#include "ESP32QRCodeReader.h"

/* ======================================== Including the libraries of SD CARD. */
#include "FS.h"
#include "SD_MMC.h"

/* ======================================== */

/* ======================================== Including the libraries of SEPARADOR. */
#include <Separador.h>

/* ======================================== */

const int timex = 30;       // Time that relay will be closed the number is to be multiplied for 100ms then 30 = 3000ms = 3s
const int relay_pin = 3;         // I/O pin where relay is connected
const int led_pin = 4;         // I/O pin where relay is connected
int relay_timeout = 0;      // Timeout Counter for delayed relay
String datosListaCSV = ""; //List where the list data is stored
Separador s; //Stores the value of the separator

ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);  // initialize Cam Module

/* ________________________________________________________________________________ Functions of SD CARD*/
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Read from file: ");
  while (file.available()) {
    datosListaCSV += file.readString();
    Serial.println();
    Serial.print(datosListaCSV);
    Serial.println();
    //Serial.write(file.read());
  }
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine to check the user entered by camera and the content in the csv. */
void comprobarIdentidad(String datosLista, String datosEscaneado) {

  /* ---------------------------------------- Data to evaluate from the csv and the camera. NOTE: NO DELETE (+" ")*/
  String datosCSV = datosLista+" ";
  String datosCamara = datosEscaneado;
  /* ---------------------------------------- */
  
  /* ---------------------------------------- Declare variables. */
  
  //count line breaks (people in list)
  int contadorCSV = 0;
  //counts blank spaces (scanned data)
  int contadorCamara = 0;

  //checks that the verification was carried out
  bool comprobacionRealizada = false;

  //separates the people contained in the list by a "|"
  String personasCSV = "";
  //separates the data of the people contained in the list by a ","
  String datosPersonasCSV = "";

  //stores data from CSV file
  String expedienteCSV = "";
  String cedulaCSV = "";
  String accesoCSV = "";
  //String nombreCSV = "";

  //stores the read QR data
  String cedulaCamara = "";
  String expedienteCamara = "";
  String tipoPersonaCamara = "";
  //String nombreCamara = "";
  /* ---------------------------------------- */
  
  /* ---------------------------------------- function to check the spaces and assign that value to the contadorCamara. */
  for (int i = 0; i < datosCamara.length(); i++) {
    if (datosCamara[i] == ' ') {
      contadorCamara++;
    }
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- function to check line breaks and assign a value to the contadorCSV. */
  for (int i = 0; i < datosCSV.length(); i++) {
    if (datosCSV[i] == '\n') {
      contadorCSV++;
    }
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Function to separate the people contained in the CSV file by a slash "|". */
  for (int i = 2; i < contadorCSV; i++){
    if (i > 2){
      personasCSV += ("|");
    }
    personasCSV += s.separa(datosCSV, '\n', i);
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Main data checking function. */
  for (int i = 0; i < (contadorCSV-1); i++){
    
    //Data extracted from CSV file
    datosPersonasCSV = s.separa(personasCSV, '|', i);

    cedulaCSV = s.separa(datosPersonasCSV, ';', 3);
    expedienteCSV = s.separa(datosPersonasCSV, ';', 4);
    accesoCSV = s.separa(datosPersonasCSV, ';', 0);
    /* ---------------------------------------- */

    for (int j = 0; j < (contadorCamara); j++) {
      
      //Breaks the iteration when the check is done
      if (comprobacionRealizada){
        break;
      }

      cedulaCamara = s.separa(datosCamara, ' ', j);
      if (cedulaCSV == cedulaCamara && accesoCSV == "SI"){
        tipoPersonaCamara = s.separa(datosCamara, ' ', j+2);
        if (tipoPersonaCamara == "ESTUDIANTE") {
          expedienteCamara = s.separa(datosCamara, ' ', j+1);
          if (expedienteCSV == expedienteCamara){
            Serial.println("ACCESO CONCEDIDO A: "+cedulaCSV);

            if (relay_timeout == 0) {                   // check if relay is not activated 
              relay_timeout=timex;                      // If valid enable relay for proper timeout
            }
            for (int i = 0; i < 8; i++) {                // Blink 8 times the led to show wrong gode
              digitalWrite(led_pin, HIGH);               // activate flash light LED
              delay(30);                 
              digitalWrite(led_pin, LOW);                // deactivate flash light LED
              delay(30);                 
            }

            comprobacionRealizada = true;
            break;
          }
        }
        if (accesoCSV == "SI" && (tipoPersonaCamara == "DOCENTE" || tipoPersonaCamara == "ADMINISTRATIVO" || tipoPersonaCamara == "OBRERO" || tipoPersonaCamara == "JUBILADO")) {
          Serial.println("ACCESO CONCEDIDO AL "+tipoPersonaCamara+": "+cedulaCSV);
          
          if (relay_timeout == 0) {                   // check if relay is not activated 
            relay_timeout=timex;                      // If valid enable relay for proper timeout
          }
          for (int i = 0; i < 8; i++) {                // Blink 8 times the led to show wrong gode
            digitalWrite(led_pin, HIGH);               // activate flash light LED
            delay(30);                 
            digitalWrite(led_pin, LOW);                // deactivate flash light LED
            delay(30);                 
          }
          
          comprobacionRealizada = true;
          break;
        }
      }
    }
  }
  /* ---------------------------------------- */

}
  /* ---------------------------------------- */

void onQrCodeTask(void *pvParameters)               // Function of QrCode reader
{
  struct QRCodeData qrCodeData;
  String DataRead;

  while (true)
  {
    if (reader.receiveQrCode(&qrCodeData, 100))         // Qr Module received a data
    {
      Serial.println("[OK] - Found QRCode");
      if (qrCodeData.valid)                             // check if valid data received
      {
        DataRead = ((const char *)qrCodeData.payload);  // Put in string the readed value
  
        Serial.print("Code found: ");
        Serial.println(DataRead);
        comprobarIdentidad(datosListaCSV,DataRead);
      }
      else
      {
        Serial.print("Invalid: ");                      //invalid barcode
        Serial.println((const char *)qrCodeData.payload);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup()                                           // setup reader/cam/serial
{
  Serial.begin(115200);
  Serial.println();

  /* ---------------------------------------- SD CARD .TXT */
  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }

  Serial.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  readFile(SD_MMC, "/ListaDeAcceso.txt");              //read list file
  testFileIO(SD_MMC, "/test.txt");                     //check read and write speed
  Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
  /* ---------------------------------------- */

  pinMode (relay_pin, OUTPUT);                        //defines relay_pin as outputs
  pinMode (led_pin, OUTPUT);                          //defines led_pin as outputs
  digitalWrite(led_pin, LOW);                         // deactivate flash light LED

  reader.setup();

  Serial.println("Setup QRCode Reader");

  reader.beginOnCore(1);

  Serial.println("Begin on Core 1");

  xTaskCreate(onQrCodeTask, "onQrCode", 4 * 1024, NULL, 4, NULL);

  Serial.println("Go on Core 1");
}

void loop()
{
  delay(100);            
  if ( relay_timeout != 0) {                        // if relay timeout <> 0 or signalOutside_pin = HIGH enable relay
    digitalWrite(relay_pin, HIGH);
    if ( relay_timeout == timex) {                  // if first timeout count flash led to tell proper reading
      digitalWrite(led_pin, HIGH);                  // activate flash light LED
    } else {                                        // else disable relay
      digitalWrite(led_pin, LOW);                   // deactivate flash light LED
    }
    relay_timeout--;
  } else {                                          // else disable relay
    digitalWrite(relay_pin, LOW);
    digitalWrite(led_pin, LOW);                     // deactivate flash light LED
  }

}
