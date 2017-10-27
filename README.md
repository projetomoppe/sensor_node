# sensor_node

Este nó realiza a leitura dos dados dos sensores.
Salva os dados no cartão SD presente no dispositivo e envia os dados ao *sink_node*
através do módulo NRF24L01. Este módulo não possui acesso direto à internet.

## Componentes
- Arduino MEGA 2560;
- 2 Sensores ICOS LA16M40;
- Sensor Ultrassônico HC-SR04;
- Módulo GPS GY-NEO6MV2;
- Módulo NRF24L01;
- Módulo Cartão SD;
- Resistores para divisor de tensão;
- Jumpers.

## Princípio de Funcionamento

- **Sensores ICOS:** fazem a medição do nível de acordo com seu estado que pode ser 0 ou 1.
À medida que o nível sobe, passa pelo sensor, mudando seu estado.
O sensor pode ser instalado normalmente aberto (NA -> 0) ou normalmente fechado (NF -> 1).

- **Sensor Ultrassônico:** Mede o nível através de ondas sonoras.
As ondas são disparadas através do pino Trigger.
As ondas então rebatem no obstáculo (água) e retornam em direção ao módulo, onde os pino Echo as identifica.
Através do tempo e da velocidade do som é possível descobrir a distância do obstáculo ao módulo.
A leitura do nível acontece em tempo real e com esses dados é possível, inclusive, medir a velocidade
em que o nível sobe para assim realizar uma estimativa de tempo em que o nível subirá até o nível crítico.

- **Módulo GPS:** Conecta-se a satélites disponíveis e informa diversas informações de localização,
data, hora, elevação, velocidade, etc. Utilizado para exibir a localização do protótipo e mostrar dados como data, hora e elevação.

- **Módulo NRF24L01:** Envia a *struct* de dados ao *sink_node* através da comunicação por rádio frequência.

- **Módulo Cartão SD:** Mantém localmente um log de dados de cada dispositivo. Os dados são gravados em um arquivo .csv.

## Pinagem dos dispositivos

- Sensor ICOS LA16M40:
```
**Sensor Superior**
Jumper 1 -> 5V
Jumper 2 -> Resistor de 220 Ohms -> Arduino (D3) e GND

**Sensor Inferior**
Jumper 1 -> 5V
Jumper 2 -> Resistor de 220 Ohms -> Arduino (D4) e GND
```

- Sensor Ultrassônico HC-SR04:
```
VCC do módulo -> 5V
GND do módulo -> GND
TRIGGER do módulo -> Arduino (D10)
ECHO do módulo -> Arduino (D9)
```

- Módulo GPS:
```
VCC do módulo -> 3.3V
GND do módulo -> GND
TX do módulo -> RX3 (D15) Arduino MEGA 2560
RX do módulo -> Dividor de Tensão 3.3V -> TX3 (D14) Arduino MEGA 2560
```

- Módulo NRF24L01:
```
VCC do módulo -> 5V
GND do módulo -> GND
MISO do módulo -> MISO SPI Arduino MEGA 2560 (D50)
MOSI do módulo -> MISO SPI Arduino MEGA 2560 (D51)
SCK do módulo -> MISO SPI Arduino MEGA 2560 (D52)
CE do módulo -> D7 Arduino MEGA 2560
CSN do módulo -> D53 Arduino MEGA 2560
```

- Módulo Cartão SD
```
VCC do módulo -> 5V
GND do módulo -> GND
MISO do módulo -> MISO SPI Arduino MEGA 2560 (D50)
MOSI do módulo -> Dividor de Tensão 3.3V -> MOSI SPI Arduino MEGA 2560 (D51)
SCK do módulo -> Dividor de Tensão 3.3V -> SCK SPI Arduino MEGA 2560 (D52)
CS do módulo -> Dividor de Tensão 3.3V -> D6 Arduino MEGA 2560
```

## Construído com

- https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home - Biblioteca NewPing - Módulo HC-SR04
- http://arduiniana.org/libraries/tinygpsplus/ - Biblioteca TinyGPS++ - Módulo GPS
- https://github.com/maniacbug/RF24/ - Arduino driver para nRF24L01
- http://arduinominas.com.br/SDFATLib/html/index.html - Biblioteca Módulo Cartão SD

## Observações

- Necessário a inclusão da biblioteca nativa da IDE do Arduino SPI.h.
Serial Peripheral Interface (SPI) (Interface Periférica Serial na tradução livre) é um
protcolo síncrono de dados serial utilizado por microcontroladores
para comunicação com um ou mais dispositivos periféricos de forma rápida em pequenas distâncias.

- Um capacitor de 100uF foi soldado entre os pinos VCC e GND do módulo NRF24L01.
Esta foi a maneira encontrada para estabilizar o funcionamento do módulo, de modo a obter leituras consistentes.
Capacitores possuem a capacidade de amenizar ou eliminar ruídos.

- Um capacitor de 100uF foi soldado entre os pinos VCC e GND do módulo cartão SD.
Esta foi a maneira encontrada para estabilizar o funcionamento do módulo, de modo a obter funcionamento consistente.

## Biblioteca NewPing (Funções e Construtores)

- NewPing us(TRIG, ECHO);
  - Construtor do objeto "us". Recebe como parâmetro os pinos trigger e echo.
  
- us.ping_cm();
  - Função que retorna o valor do nível obtido pelo módulo em centímetros.

## Biblioteca TinyGPSPlus (Funções e Construtores)

- TinyGPSPlus gps;
  - Construtor do objeto "gps".
  
- gps.location.isValid();
  - Verifica se os dados obtidos pelo módulo GPS são dados válidos.
  
- gps.encode(Serial3.read());
  - Função que obtém a string de dados do GPS NMEA e codifica de modo a obter os dados separadamente.
  
## Biblioteca SdFat (Funções e Construtores)

- SdFat sdCard;
  - Construtor do objeto "sdCard".
  
- sdCard.begin(pinSD, SD_SCK_MHZ(50));
  - Inicializa o módulo SD. Recebe como parâmetro o pino de conexão com o Arduino e a frequência de comunicação.
  
- sdCard.initErrorHalt();
  - Em caso de erro de inicialização, encerra o programa e informa o erro que está ocorrendo.
  
- sdCard.exists(logNomeArquivo);
  - Verifica se o arquivo já existe no cartãoSD.
  
- SdFile logFile;
  - Define a instância para o arquivo.
  
- logFile.open(logNomeArquivo, O_RDWR | O_CREAT | O_AT_END);
  - Abre o arquivo desejado.
  
- logFile.print(dadosSensores.ID);
  - Imprime os dados no arquivo aberto.

## Biblioteca RF24 (Funções e Construtores)

- RF24 radio(CEpin, CSpin);
  - RF24::RF24 (uint8_t _cepin, uint8_t _cspin). Construtor -> Cria uma nova instância do driver.
  
- radio.begin();
  - void RF24::begin ( void ). Inicia a operção do chip.
  
- radio.openWritingPipe(pipe);
  - void RF24::openWritingPipe ( uint64_t address ). Abre um túnel para escrita. Apenas um túnel pode ser aberto por vez.
  
- radio.write(&dadosSensores, sizeof(dadosSensores));
  - bool RF24::write ( const void * buf, uint8_t len ). Escreve no túnel aberto para escrita.
  
## LEDs de Status
- LED 01 - Alimentação
  - Aceso -> Indica a alimentação do dispositivo;
  - Apagado -> Indica falta de alimentação do dispositivo;
  
- LED 02 - Módulo SD
  - Apagado -> Funcionamento normal;
  - Aceso -> Indica Falha de Inicialização;
  - 300ms ON 1000ms OFF -> Erro no log dos dados dos sensores;
  - 300ms ON 2000ms OFF -> Erro na impressão do cabeçalho do arquivo;
  - 300ms ON 3000ms OFF -> Erro no log de dados - abertura do arquivo;
  
- LED 03 - Módulo GPS
  - Apagado -> Funcionamento normal;
  - 300ms ON 1000ms OFF -> Dados do GPS inválidos;
  
- LED 04 - Módulo RF24
  - Apagado -> Funcionamento normal;
  - 300ms ON 1000ms OFF -> Falha no envio dos dados;
