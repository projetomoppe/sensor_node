#define DEBUG

#include <SPI.h>             // SPI
#include <NewPing.h>         // Sensor ultrassonico
#include <TinyGPS++.h>       // Modulo GPS
#include <SdFat.h>           // Modulo Cartão SD
#include <nRF24L01.h>        // Modulo Com RF  - RF24
#include <RF24.h>            // Modulo Com RF  - RF24

// definindo as constantes do programa
// ID DO DISPOSITIVO
const int ID_dispositivo = 1;  // ID do pacote enviado

// LEDs de status
// LED_1 - Informa se dispositivo está alimentado
// LED_2 - Indica falha no módulo SD (Ligado - Erro Inicializacao / 300ms ON e 2000ms OFF - erro no log de dados)
// LED_3 - Indica falha no módulo GPS
// LED_4 - Indica falha no módulo RF24
const int LED_2 = 11;
const int LED_3 = 12;
const int LED_4 = 13;

// SENSORES ICOS
// Identificacao Sensor Superior - Azul e Branco
// Identificacao Sensor Inferior - Verde e Marron
const int S1 = 4; // pinos sensores ICOS - sensor inferior
const int S2 = 3; // sensor superior

// SENSOR ULTRASSONICO
const int ECHO = 9; // pinos sensor ultrassonico
const int TRIG = 10;
const float REF = 35.0; // referencia de altura de instalacao do sensor ultrassonico

// MODULO GPS
static const uint32_t GPSB = 9600; // definindo a velocidade de comunicação do módulo GPS

// MODULO RF24
const int CEpin = 7;
const int CSpin = 53;

// MODULO SD CARD
const int pinSD = 6;

// nome dos arquivos
#define LOG_FILE_PREFIX "log"
#define MAX_LOG_FILES 100
#define LOG_FILE_SUFIX "csv"
char logNomeArquivo[10];

// colunas do arquivo
#define LOG_COLUMN_COUNT 11
char* logColunas[LOG_COLUMN_COUNT] = {
  "ID", "ICOS Inferior", "ICOS Superior", "Nivel Ultrassonico", "Latitude", "Longitude", "Elevacao", "Data", "Hora", "Minuto", "Segundo"
};

// controle de rate de log
const unsigned long LOG_RATE = 3000;
unsigned long ultimoLog = 0;

// inicialização dos objetos
SdFat sdCard;
TinyGPSPlus gps;
NewPing us(TRIG, ECHO);
RF24 radio(CEpin, CSpin);
const uint64_t pipe = 0xF0F0F0F0E1LL;

// DEFININDO STRUCT DE DADOS
// struct com dados dos sensores
typedef struct{
  int ID;                    // ID do dispositivo - inteiro tamanho variavel (3 maximo)
  int ICOS_INF;              // sensor ICOS inferior - inteiro tamanho 1
  int ICOS_SUP;              // sensor ICOS superior - inteiro tamanho 1
  float NIVEL;               // nivel do ultrassonico - float com 2 casas decimais (cm)
  bool LAT_NEG;              // booleano para latitude negativa (se 1 é negativo)
  int LAT_DEG;               // graus da latitude - inteiro tamanho 2
  uint16_t LAT_BILLIONTHS;   // decimal da latitude - inteiro tamanho 32
  bool LNG_NEG;              // booleano para longitude negativa (se 1 é negativo)
  int LNG_DEG;               // graus da longitude - inteiro tamanho 2
  uint16_t LNG_BILLIONTHS;   // decimal da latitude - inteiro tamanho 32
  double ELEVACAO;           // elevacao - double
  uint16_t ANO;              // ano - inteiro tamanho 4
  uint8_t MES;               // mês - iinteiro tamanho 2
  uint8_t DIA;               // dia - inteiro tamanho 2
  uint8_t HORA;              // hora - inteiro tamanho 2
  uint8_t MINUTO;            // minuto - inteiro tamanho 2
  uint8_t SEGUNDO;           // segundo - inteiro tamanho 2
}
S_t;

// declarando as structs que armazenarão os dados
S_t dadosSensores;

void setup()
{
  #ifdef DEBUG
    Serial.begin(57600); // comunicacao Serial com o computador
  
    Serial.println(F("\r\n----------- PICJr - MOPPE -----------"));
    Serial.println(F("------- Programa inicializado -------\r\n"));
    Serial.println(F("Iniciando Setup..."));
  #endif
  
  // Define PIN Mode LEDs
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
  
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  digitalWrite(LED_4, LOW);

  // modulo GPS
  Serial3.begin(GPSB);

  // define os sensores ICOS como input
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);

  // Inicializa o modulo SD
  if(!sdCard.begin(pinSD, SD_SCK_MHZ(50))){
    digitalWrite(LED_2, HIGH);
    
    #ifdef DEBUG
      sdCard.initErrorHalt();
    #endif
  }

  // cria novo arquivo a cada inicializacao
  updateFileName(); // cria um novo arquivo a cada inicializacao
  printHeader(); // imprime o cabecalho do arquivo

  #ifdef DEBUG
    Serial.println(F("Setup Finalizado!"));
  #endif

  // RF24
  radio.begin();
  radio.openWritingPipe(pipe);
} // fecha void setup()

void loop()
{ 
  // obtencao dos dados dos sensores
  dadosSensores.ID       = ID_dispositivo;
  dadosSensores.ICOS_INF = digitalRead(S1); // leitura do sensor ICOS inferior
  dadosSensores.ICOS_SUP = digitalRead(S2); // leitura do sensor ICOS superior
  dadosSensores.NIVEL    = dados_su();      // obtencao do nivel dado pelo sensor ultrassonico
  
  dados_gps(); // alimenta o objeto gps

  // alimenta struct
  dadosSensores.LAT_NEG = gps.location.rawLat().negative;
  dadosSensores.LAT_DEG = gps.location.rawLat().deg;
  dadosSensores.LAT_BILLIONTHS = gps.location.rawLat().billionths;
  dadosSensores.LNG_NEG = gps.location.rawLng().negative;
  dadosSensores.LNG_DEG = gps.location.rawLng().deg;
  dadosSensores.LNG_BILLIONTHS = gps.location.rawLng().billionths;
  dadosSensores.ELEVACAO = gps.altitude.meters();
  dadosSensores.ANO = gps.date.year();
  dadosSensores.MES = gps.date.month();
  dadosSensores.DIA = gps.date.day();
  dadosSensores.HORA = gps.time.hour();
  dadosSensores.MINUTO = gps.time.minute();
  dadosSensores.SEGUNDO = gps.time.second();

  // DEBUG
  #ifdef DEBUG
    Serial.println(dadosSensores.ID);
    Serial.println(dadosSensores.ICOS_INF);
    Serial.println(dadosSensores.ICOS_SUP);
    Serial.println(dadosSensores.NIVEL);
    Serial.println(dadosSensores.LAT_NEG);
    Serial.println(dadosSensores.LAT_DEG);
    Serial.println(dadosSensores.LAT_BILLIONTHS);
    Serial.println(dadosSensores.LNG_NEG);
    Serial.println(dadosSensores.LNG_DEG);
    Serial.println(dadosSensores.LNG_BILLIONTHS);
    Serial.println(dadosSensores.ELEVACAO);
    Serial.println(dadosSensores.ANO);
    Serial.println(dadosSensores.MES);
    Serial.println(dadosSensores.DIA);
    Serial.println(dadosSensores.HORA);
    Serial.println(dadosSensores.MINUTO);
    Serial.println(dadosSensores.SEGUNDO);
    Serial.println();
  #endif

  if((ultimoLog + LOG_RATE) <= millis()) // Se  LOG_RATE em  milissegundos desde o último registro
  {
    if(gps.location.isValid())
    {
      if(logData(dadosSensores)) // Registrar dados no cartao SD
      {
        #ifdef DEBUG
          Serial.println("DADOS LOGADOS"); //mostratr essa mensagem
        #endif
        ultimoLog = millis(); // atualizar a variavel
      }
      else // se nao atualizou
      {
        digitalWrite(LED_2, HIGH);
        delay(300);
        digitalWrite(LED_2, LOW);
        delay(1000);
        
        #ifdef DEBUG
          Serial.println("Falha no log de dados."); // sera mostrado uma mensagem de erro no GPS
        #endif
      }

      // ENVIAR DADOS AO SINK NODE
      bool ok = radio.write(&dadosSensores, sizeof(dadosSensores));
      if(ok) {
        #ifdef DEBUG
          Serial.println(F("DADOS ENVIADOS"));
        #endif
      } else {
        digitalWrite(LED_4, HIGH);
        delay(300);
        digitalWrite(LED_4, LOW);
        delay(1000);
        
        #ifdef DEBUG
          Serial.println(F("FALHA NO ENVIO DOS DADOS"));
        #endif
      }
    }
    else // dados do GPS nao validos
    {
      digitalWrite(LED_3, HIGH);
      delay(300);
      digitalWrite(LED_3, LOW);
      delay(1000);
      
      #ifdef DEBUG
        Serial.println("Dados de GPS invalidos. Buscando...");
      #endif
    }
  }
} // fecha void loop()

// obtendo dados do sensor ultrassonico
float dados_su(){
  float dist = us.ping_cm();
  float nivel = REF - dist;
  return nivel;
}

// alimentando o objeto gps com dados do módulo GPS
void dados_gps(){
  while(Serial3.available())
    gps.encode(Serial3.read());
}

void updateFileName(){

  for(int i=0; i < MAX_LOG_FILES; i++){
    memset(logNomeArquivo, 0, strlen(logNomeArquivo));
    sprintf(logNomeArquivo,"%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFIX);
    
    if (!sdCard.exists(logNomeArquivo))
      break;
    else{
      #ifdef DEBUG
        Serial.print(logNomeArquivo);
        Serial.println(" existe!");
      #endif
    }
  }

  #ifdef DEBUG
    Serial.print("Nome do Arquivo: ");
    Serial.println(logNomeArquivo);
  #endif
}

void printHeader(){
  
  SdFile logFile;
  bool sd_open = logFile.open(logNomeArquivo, O_RDWR | O_CREAT | O_AT_END);
  
  if(sd_open){
    for(int i=0; i < LOG_COLUMN_COUNT; i++){
      logFile.print(logColunas[i]);
      if(i < (LOG_COLUMN_COUNT -1))
         logFile.print(',');
      else
         logFile.println();
    }
    logFile.close();
  }
  else{
    digitalWrite(LED_2, HIGH);
    delay(300);
    digitalWrite(LED_2, LOW);
    delay(2000);
    
    #ifdef DEBUG
      Serial.println("\r\nErro na abertura do arquivo (HEADER)!");
    #endif
  }
}

byte logData(S_t dadosSensores)
{
  SdFile logFile;
  bool sd_open = logFile.open(logNomeArquivo, O_RDWR | O_CREAT | O_AT_END);

  // grava os dados no arquivo do cartao SD
  if(sd_open){
    logFile.print(dadosSensores.ID);
    logFile.print(',');
    logFile.print(dadosSensores.ICOS_INF);
    logFile.print(',');
    logFile.print(dadosSensores.ICOS_SUP);
    logFile.print(',');
    logFile.print(dadosSensores.NIVEL);
    logFile.print(',');
    logFile.print(dadosSensores.LAT_NEG ? "-" : "+");
    logFile.print(dadosSensores.LAT_DEG);
    logFile.print(".");
    logFile.print(dadosSensores.LAT_BILLIONTHS);
    logFile.print(',');
    logFile.print(dadosSensores.LNG_NEG ? "-" : "+");
    logFile.print(dadosSensores.LNG_DEG);
    logFile.print(".");
    logFile.print(dadosSensores.LNG_BILLIONTHS);
    logFile.print(',');
    logFile.print(dadosSensores.ELEVACAO);
    logFile.print(',');

    if(dadosSensores.DIA < 10)
      logFile.print("0");
      
    logFile.print(dadosSensores.DIA);
    logFile.print("/");

    if(dadosSensores.MES < 10)
      logFile.print("0");
      
    logFile.print(dadosSensores.MES);
    logFile.print("/");
    logFile.print(dadosSensores.ANO);
    logFile.print(',');

    if(dadosSensores.HORA < 10)
      logFile.print("0");
    
    logFile.print(dadosSensores.HORA);
    logFile.print(',');

    if(dadosSensores.MINUTO < 10)
      logFile.print("0");
    
    logFile.print(dadosSensores.MINUTO);
    logFile.print(',');

    if(dadosSensores.SEGUNDO < 10)
      logFile.print("0");
    
    logFile.print(dadosSensores.SEGUNDO);
    logFile.println();
    logFile.close();
    return 1;
  }
  else{
    digitalWrite(LED_2, HIGH);
    delay(300);
    digitalWrite(LED_2, LOW);
    delay(3000);
    
    #ifdef DEBUG
      Serial.println("\r\nErro na abertura do arquivo! (DADOS)");
    #endif
    
    return 0;
  }
}
