//#include <SPI.h>              // SPI
#include <NewPing.h>         // Sensor ultrassonico
#include <TinyGPS++.h>       // Modulo GPS
#include <SdFat.h>           // Modulo Cartão SD
//#include <nRF24L01.h>
//#include <RF24.h>

// definindo as constantes do programa
// ID DO DISPOSITIVO
const int ID_dispositivo = 1;  // ID do pacote enviado

// SENSORES ICOS
const int S1 = 4; // pinos sensores ICOS
const int S2 = 3;

// SENSOR ULTRASSONICO
const int ECHO = 12; // pinos sensor ultrassonico
const int TRIG = 13;
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
#define LOG_COLUMN_COUNT 8
char* logColunas[LOG_COLUMN_COUNT] = {
  "ID", "ICOS Inferior", "ICOS Superior", "Nivel Ultrassonico", "Latitude", "Longitude", "Data", "Hora"
};

// controle de rate de log
const unsigned long LOG_RATE = 3000;
unsigned long ultimoLog = 0;

// definindo variaveis globais do programa
//float nivel = 0.0;
//int icos_inf;
//int icos_sup;

// inicialização dos objetos
SdFat sdCard;
TinyGPSPlus gps;
NewPing us(TRIG, ECHO);
//RF24 radio(CEpin,CSpin);
//const uint64_t pipe = 0xE8E8F0F0E1LL;

// DEFININDO STRUCT DE DADOS
// struct com dados dos sensores
typedef struct{
  int ID;
  int ICOS_INF;
  int ICOS_SUP;
  float NIVEL;
}
S_t;

// struct com dados do GPS
typedef struct{
  int ID;
  bool LAT_NEG;
  int LAT_DEG;
  uint16_t LAT_BILLIONTHS;
  bool LNG_NEG;
  int LNG_DEG;
  uint16_t LNG_BILLIONTHS;
  uint16_t ANO;
  uint8_t MES;
  uint8_t DIA;
  uint8_t HORA;
  uint8_t MINUTO;
  uint8_t SEGUNDO;
}
GPS_t;

// declarando as structs que armazenarão os dados
S_t dadosSensores;
GPS_t dadosGPS;

void setup()
{ 
  Serial.begin(9600); // comunicacao Serial com o computador
  Serial3.begin(GPSB);  // modulo GPS

  Serial.println();
  Serial.println(sizeof(dadosGPS));
  Serial.println();

  // define os sensores ICOS como input
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);

  Serial.println(F("\r\n----------- PICJr - MOPPE -----------"));
  Serial.println(F("------- Programa inicializado -------\r\n"));

  Serial.println(F("Iniciando Setup"));

  // inicializa modulo RF24
//  radio.begin();
//  radio.openWritingPipe(pipe);

  // Inicializa o modulo SD
  if(!sdCard.begin(pinSD, SPI_HALF_SPEED))
    sdCard.initErrorHalt();

  // cria novo arquivo a cada inicializacao
  updateFileName(); // cria um novo arquivo a cada inicializacao
  printHeader(); // imprime o cabecalho do arquivo
  
  Serial.println(F("Setup Finalizado"));
} // fecha void setup()

void loop()
{ 
  // obtencao dos dados
  dadosSensores.ICOS_INF = digitalRead(S1); // leitura do sensor ICOS inferior
  dadosSensores.ICOS_SUP = digitalRead(S2); // leitura do sensor ICOS superior
  dadosSensores.NIVEL    = dados_su();      // obtencao do nivel dado pelo sensor ultrassonico
  dados_gps();                    // alimenta o objeto gps

  Serial.println(gps.location.rawLng().negative);

  Serial.println();
  Serial.println(dadosSensores.ICOS_INF);
  Serial.println(dadosSensores.ICOS_SUP);
  Serial.println(dadosSensores.NIVEL);
  Serial.println(gps.location.lat(), 6);
  Serial.println(gps.location.lng(), 6);
  Serial.println(gps.time.value());
  Serial.println(gps.date.value());
  Serial.println();

  if((ultimoLog + LOG_RATE) <= millis()) // Se  LOG_RATE em  milissegundos desde o último registro
  {
    if(gps.location.isUpdated())
    {
      if(logData(ID_dispositivo, dadosSensores.ICOS_INF, dadosSensores.ICOS_SUP, dadosSensores.NIVEL)) // Registrar os dados do GPS
      {
        Serial.println("Dados Logados!"); //mostratr essa mensagem
        ultimoLog = millis(); // atualizar a variavel
      }
      else // se nao atualizou
      {
        Serial.println("Falha no log de dados."); // sera mostrado uma mensagem de erro no GPS
      }
    }
    else // dados do GPS nao validos
    {
      Serial.println("Dados de GPS inválidos. Buscando...");
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
      Serial.print(logNomeArquivo);
      Serial.println(" existe!");
    }
  }

  Serial.print("Nome do Arquivo: ");
  Serial.println(logNomeArquivo);
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
    sdCard.errorHalt("\r\nErro na abertura do arquivo!\r\n");
  }
}

byte logData(int ID, int inf, int sup, float us)
{
  SdFile logFile;
  bool sd_open = logFile.open(logNomeArquivo, O_RDWR | O_CREAT | O_AT_END);
  
  if(sd_open){
    logFile.print(ID);
    logFile.print(',');
    logFile.print(inf);
    logFile.print(',');
    logFile.print(sup);
    logFile.print(',');
    logFile.print(us);
    logFile.print(',');
    logFile.print(gps.location.lat(), 6);
    logFile.print(',');
    logFile.print(gps.location.lng(), 6);
    logFile.print(',');
    logFile.print(gps.date.value());
    logFile.print(',');
    logFile.print(gps.time.value());
    logFile.println();
    logFile.close();
    return 1;
  }
  else{
    sdCard.errorHalt("\r\nErro na abertura do arquivo!\r\n");
    return 0;
  }
}
