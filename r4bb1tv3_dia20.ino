#include <TFT_eSPI.h>  // Biblioteca para o display TFT
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>  // Adiciona a biblioteca ESP WiFi

// Pinos dos botões
#define BUTTON_UP 22
#define BUTTON_DOWN 21
#define BUTTON_SELECT 4

// Inicializa o display TFT
TFT_eSPI tft = TFT_eSPI();  

// Variáveis de estado
unsigned long lastUpdateTime = 0;    // Último tempo de atualização
const unsigned long updateInterval = 1000;  // Intervalo desejado de atualização em milissegundos
int currentSelection = 0;
const int totalOptions = 4;
int subMenuSelection = 0;
const int subMenuOptions = 5;  // Adicionado "Back" como opção extra
int networkSelection = 0;
int totalNetworks = 0;
int currentChannel = 1;  // Canal inicial para monitoramento por canal
String networks[20];  // Suporte para até 20 redes WiFi

// Variáveis para armazenar informações da varredura inicial
uint8_t ap_count[14] = {0};
int32_t max_rssi[14] = {-100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100};
int packet_count[14] = {0};  // Contador de pacotes por canal
int initial_rssi[20];
int initial_channel[20];

// Informações da rede selecionada
String ssid;
String bssid;
int rssi;
int channel;
String encryptionType;

// Channel color mapping from channel 1 to 14
uint16_t channel_color[] = {
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA,
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA,
  TFT_RED, TFT_ORANGE
};

enum InterfaceState {
  MAIN_MENU,
  SUB_MENU,
  NETWORK_LIST,
  NETWORK_INFO,
  CHANNEL_MONITOR,
  INTENSITY_MONITOR,
  MONITOR_BY_NETWORK
};

InterfaceState currentState = MAIN_MENU;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

bool debounceButton(int pin) {
  if (digitalRead(pin) == LOW) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      lastDebounceTime = millis();
      return true;
    }
  }
  return false;
}

void drawText(int x, int y, const char *text, uint16_t color) {
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.print(text);
}

// Desenhar a interface principal
void drawMainMenu() {
  tft.fillScreen(TFT_BLACK);
  
  uint16_t highlightColor = TFT_RED;
  uint16_t normalColor = TFT_WHITE;

  // Menu Principal
  drawText(30, 70, "WiFi Analyser", currentSelection == 0 ? highlightColor : normalColor);
  drawText(30, 100, "WiFi Attacks", currentSelection == 1 ? highlightColor : normalColor);
  drawText(30, 130, "SD Manager", currentSelection == 2 ? highlightColor : normalColor);
  drawText(30, 160, "SD Reader", currentSelection == 3 ? highlightColor : normalColor);

  // Desenhar quadrado em volta da opção selecionada
  int rectY = 60 + currentSelection * 30;
  tft.drawRect(20, rectY, 200, 30, highlightColor);
}

// Desenhar o submenu do WiFi Analyser
void drawSubMenu() {
  tft.fillScreen(TFT_BLACK);
  
  uint16_t highlightColor = TFT_RED;
  uint16_t normalColor = TFT_WHITE;

  // Submenu WiFi Analyser
  drawText(10, 40, "Monitor by Channel", subMenuSelection == 0 ? highlightColor : normalColor);
  drawText(10, 70, "Monitor by Network", subMenuSelection == 1 ? highlightColor : normalColor);
  drawText(10, 100, "Monitor intensity", subMenuSelection == 2 ? highlightColor : normalColor);
  drawText(10, 130, "Network Info", subMenuSelection == 3 ? highlightColor : normalColor);
  drawText(10, 160, "Back", subMenuSelection == 4 ? highlightColor : normalColor);

  // Desenhar quadrado em volta da opção selecionada
  int rectY = 30 + subMenuSelection * 30;
  tft.drawRect(0, rectY, 240, 30, highlightColor);
}

// Desenhar a lista de redes WiFi
void drawNetworkList() {
  tft.fillScreen(TFT_BLACK);

  uint16_t highlightColor = TFT_RED;
  uint16_t normalColor = TFT_WHITE;

  // Opção de voltar
  drawText(10, 30, "Back", networkSelection == 0 ? highlightColor : normalColor);
  if (networkSelection == 0) {
    tft.drawRect(0, 20, 240, 30, highlightColor);
  }

  // Listar redes WiFi
  for (int i = 0; i < totalNetworks; i++) {
    if (i < 5) {  // Exibir até 5 redes na tela (depois de "Back")
      drawText(10, 60 + i * 30, networks[i].c_str(), networkSelection == (i + 1) ? highlightColor : normalColor);
      if (networkSelection == (i + 1)) {
        tft.drawRect(0, 50 + i * 30, 240, 30, highlightColor);
      }
    }
  }
}

void drawChannelMonitor() {
  tft.fillScreen(TFT_BLACK); // Limpa a tela
  
  uint16_t highlightColor = TFT_RED;
  uint16_t normalColor = TFT_WHITE;

  drawText(30, 30, ("Channel: " + String(currentChannel)).c_str(), normalColor); // Mostra o canal atual
  drawText(30, 70, ("Networks: " + String(ap_count[currentChannel - 1])).c_str(), normalColor); // Mostra a quantidade de redes no canal atual
  drawText(30, 110, ("Packets: " + String(packet_count[currentChannel - 1])).c_str(), highlightColor); // Mostra a quantidade de pacotes no canal atual

  // Desenhar quadrado em volta das informações
  tft.drawRect(20, 20, 200, 120, highlightColor);
}

void drawNetworkMonitor() {
  tft.fillScreen(TFT_BLACK); // Limpa a tela

  int networkIndex = networkSelection - 1;
  if (networkIndex >= 0 && networkIndex < totalNetworks) {
    String ssid = networks[networkIndex];
    int channel = initial_channel[networkIndex];
    int rssi = initial_rssi[networkIndex];

    drawText(10, 40, ("SSID: " + ssid).c_str(), TFT_WHITE);
    drawText(10, 70, ("Channel: " + String(channel)).c_str(), TFT_WHITE);
    drawText(10, 100, ("RSSI: " + String(rssi)).c_str(), TFT_WHITE);
  } else {
    drawText(10, 40, "No Network Selected", TFT_WHITE);
  }
}

// Desenhar informações detalhadas da rede
void drawNetworkInfo() {
  tft.fillScreen(TFT_BLACK);

  uint16_t highlightColor = TFT_RED;
  uint16_t normalColor = TFT_WHITE;
  uint16_t backgroundColor = TFT_BLACK; // Cor de fundo para o texto

  tft.setTextSize(2); // Tamanho do texto

  int x = 15;         // Posição X para os títulos
  int y = 5;         // Posição Y para titulos
  int lineSpacing = 40; // Espaçamento entre as linhas de texto

  // Títulos das informações
  tft.setTextColor(normalColor, backgroundColor);
  tft.setCursor(x, y);
  tft.print("SSID:");
  y += lineSpacing;
  tft.setCursor(x, y);
  tft.print("BSSID:");
  y += lineSpacing;
  tft.setCursor(x, y);
  tft.print("RSSI:");
  y += lineSpacing;
  tft.setCursor(x, y);
  tft.print("Channel:");
  y += lineSpacing;
  tft.setCursor(x, y);
  tft.print("Encryption:");

  int xinfo = 15;         // Posição X para informaçoes das redes
  int yinfo = 25;         // Posição Y para informaçoes das redes

  tft.setTextColor(highlightColor, backgroundColor);
  
  tft.setCursor(xinfo, yinfo);
  tft.print(ssid.c_str());
  yinfo += lineSpacing;
  tft.setCursor(xinfo, yinfo);
  tft.print(bssid.c_str());
  yinfo += lineSpacing;
  tft.setCursor(xinfo, yinfo);
  tft.print(String(rssi));
  yinfo += lineSpacing;
  tft.setCursor(xinfo, yinfo);
  tft.print(String(channel));
  yinfo += lineSpacing;
  tft.setCursor(xinfo, yinfo);
  tft.print(encryptionType.c_str());
}

#define MAX_NETWORKS 20  // Define MAX_NETWORKS como 20, ou outro valor apropriado para o seu projeto


void drawIntensityMonitor() {
  // Limpa a tela
  tft.fillScreen(TFT_BLACK);

  // Definições
  const int barWidth = 20;   // Largura de cada barra
  const int barSpacing = 10; // Espaçamento entre as barras
  const int graphHeight = 180;
  const int baseY = 240;     // Posição Y da base das barras
  const int startX = 10;     // Posição X inicial da primeira barra
  const int numColors = 7;   // Número de cores disponíveis
  const int maxSSIDLength = 10; // Comprimento máximo do SSID exibido
  uint16_t colors[numColors] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_ORANGE, TFT_CYAN, TFT_MAGENTA};

  // Verifica se 'totalNetworks' é válido
  if (totalNetworks <= 0 || totalNetworks > MAX_NETWORKS) {
    // Handle error or return
    return;
  }

  // Mapeia o valor de RSSI para a altura máxima das barras
  int maxRSSI = -30;  // Define o máximo de RSSI para uma barra completa
  int minRSSI = -100; // Define o mínimo de RSSI para uma barra mínima
  int barMaxHeight = graphHeight;  // Altura máxima da barra (pode ser ajustada conforme necessário)

  // Desenha barras e nomes das redes
  for (int i = 0; i < totalNetworks; i++) {
    // Mapeia o valor de RSSI para a altura da barra dentro do gráfico
    int barHeight = map(initial_rssi[i], minRSSI, maxRSSI, 0, barMaxHeight);
    int barX = startX + i * (barWidth + barSpacing);

    // Seleciona a cor da barra
    uint16_t color = colors[i % numColors];

    // Desenha a barra
    tft.fillRect(barX, baseY - barHeight, barWidth, barHeight, color);

    // Desenha o valor de RSSI acima da barra
    tft.setCursor(barX, baseY - barHeight - 15);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.print(initial_rssi[i]);

    // Desenha o nome da rede (SSID) abaixo da barra
    tft.setCursor(barX, baseY + 5);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Cor do texto e do fundo
    String ssid = networks[i];
    if (ssid.length() > maxSSIDLength) {
      ssid = ssid.substring(0, maxSSIDLength) + "...";  // Trunca SSID muito longo
    }
    tft.print(ssid);
  }

  // Desenha marcas de referência para os valores de RSSI
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(startX, baseY - graphHeight - 10);
  tft.print("-30 dBm");

  tft.setCursor(startX, baseY - graphHeight / 2 - 10);
  tft.print("-65 dBm");

  tft.setCursor(startX, baseY - 5);
  tft.print("-100 dBm");
}



void drawLoadingScreen() {
  tft.fillScreen(TFT_BLACK);
  
  // Set text size and color for the project name
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  
  // Manually center the text 'r4bb1t'
  const char *projectName = "r4bb1t";
  int projectNameLength = strlen(projectName) * 6 * 3; // 6 is the average width of a character, 3 is the text size
  int centerX = (tft.width() - projectNameLength) / 2;
  int centerY = tft.height() / 2 - 8 * 3; // 8 is the average height of a character, 3 is the text size

  // Draw the project name 'r4bb1t' centered
  drawText(centerX, centerY, projectName, TFT_RED);

  // Set text size for 'Loading...'
  tft.setTextSize(2);
  
  // Manually center the text 'Loading...' below the project name
  const char *loadingText = "Loading...";
  int loadingTextLength = strlen(loadingText) * 6 * 2; // 6 is the average width of a character, 2 is the text size
  centerX = (tft.width() - loadingTextLength) / 2;
  
  // Draw the text 'Loading...' centered below the project name
  drawText(centerX, centerY + 20 * 3, loadingText, TFT_RED); // 20 is an arbitrary spacing
}


// Converter tipo de criptografia em string
String encryptionTypeToString(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN:
      return "Open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2 Enterprise";
    default:
      return "Unknown";
  }
}

// Callback para captura de pacotes
void promiscuous_rx_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  int channel = pkt->rx_ctrl.channel;
  if (channel >= 1 && channel <= 14) {
    packet_count[channel - 1]++;
  }
}

void monitorChannel() {
  currentState = CHANNEL_MONITOR;
  drawChannelMonitor();
}

void monitorNetwork() {
  currentState = MONITOR_BY_NETWORK;
  drawNetworkMonitor();
}

void showNetworkInfo() {
  ssid = WiFi.SSID(networkSelection - 1);
  bssid = WiFi.BSSIDstr(networkSelection - 1);
  rssi = WiFi.RSSI(networkSelection - 1);
  channel = WiFi.channel(networkSelection - 1);
  encryptionType = encryptionTypeToString(WiFi.encryptionType(networkSelection - 1));
  currentState = NETWORK_INFO;
  drawNetworkInfo();
}

void updateDisplay() {
  switch (currentState) {
    case MAIN_MENU:
      drawMainMenu();
      break;
    case SUB_MENU:
      drawSubMenu();
      break;
    case NETWORK_LIST:
      drawNetworkList();
      break;
    case NETWORK_INFO:
      drawNetworkInfo();
      break;
    case CHANNEL_MONITOR:
      drawChannelMonitor();
      break;
    case MONITOR_BY_NETWORK:
      drawNetworkMonitor();
      break;
    case INTENSITY_MONITOR:
      drawIntensityMonitor();
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);

  tft.init();
  tft.setRotation(3);

  // Configurar a captura de pacotes
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);

  // Conectar ao WiFi
  drawLoadingScreen();
  WiFi.disconnect();
  delay(100);
  
  totalNetworks = WiFi.scanNetworks();
  for (int i = 0; i < totalNetworks && i < 20; i++) {
    networks[i] = WiFi.SSID(i);
    initial_rssi[i] = WiFi.RSSI(i);
    initial_channel[i] = WiFi.channel(i);
  }

  drawMainMenu();
}

void checkButtons() {
  if (debounceButton(BUTTON_UP)) {
    if (currentState == MAIN_MENU) {
      currentSelection = (currentSelection - 1 + totalOptions) % totalOptions;
    } else if (currentState == SUB_MENU) {
      subMenuSelection = (subMenuSelection - 1 + subMenuOptions) % subMenuOptions;
    } else if (currentState == NETWORK_LIST) {
      networkSelection = (networkSelection - 1 + totalNetworks + 1) % (totalNetworks + 1);
    } else if (currentState == CHANNEL_MONITOR) {
      currentChannel = (currentChannel - 1 + 14) % 14 + 1;
      memset(packet_count, 0, sizeof(packet_count)); // Zera a contagem de pacotes
    } else if (currentState == MONITOR_BY_NETWORK) {
      networkSelection = (networkSelection - 1 + totalNetworks + 1) % (totalNetworks + 1);
    }
    updateDisplay();
  }

  if (debounceButton(BUTTON_DOWN)) {
    if (currentState == MAIN_MENU) {
      currentSelection = (currentSelection + 1) % totalOptions;
    } else if (currentState == SUB_MENU) {
      subMenuSelection = (subMenuSelection + 1) % subMenuOptions;
    } else if (currentState == NETWORK_LIST) {
      networkSelection = (networkSelection + 1) % (totalNetworks + 1);
    } else if (currentState == CHANNEL_MONITOR) {
      currentChannel = currentChannel % 14 + 1;
      memset(packet_count, 0, sizeof(packet_count)); // Zera a contagem de pacotes
    } else if (currentState == MONITOR_BY_NETWORK) {
      networkSelection = (networkSelection + 1) % (totalNetworks + 1);
    }
    updateDisplay();
  }

  if (debounceButton(BUTTON_SELECT)) {
    if (currentState == MAIN_MENU) {
      if (currentSelection == 0) {
        currentState = SUB_MENU;
        subMenuSelection = 0;
      }
      // Adicione mais casos para outras seleções de menu principal, se necessário
    } else if (currentState == SUB_MENU) {
      if (subMenuSelection == 0) {
        monitorChannel();
      } else if (subMenuSelection == 1) {
        currentState = MONITOR_BY_NETWORK;
        networkSelection = 0;
        drawNetworkMonitor();
      } else if (subMenuSelection == 2) {
        currentState = INTENSITY_MONITOR;
        drawIntensityMonitor();
      } else if (subMenuSelection == 3) {
        currentState = NETWORK_LIST;
        networkSelection = 0;
        drawNetworkList();
      } else if (subMenuSelection == 4) {
        currentState = MAIN_MENU;
      }
    } else if (currentState == NETWORK_LIST) {
      if (networkSelection == 0) {
        currentState = SUB_MENU;
      } else {
        showNetworkInfo();
      }
    } else if (currentState == NETWORK_INFO) {
      currentState = NETWORK_LIST;
      drawNetworkList();
    } else if (currentState == CHANNEL_MONITOR) {
      currentState = SUB_MENU;
      drawSubMenu();
    } else if (currentState == MONITOR_BY_NETWORK) {
      currentState = SUB_MENU;
      drawSubMenu();
    } else if (currentState == INTENSITY_MONITOR) {
      currentState = SUB_MENU;
      drawSubMenu();
    }
    updateDisplay();
  }
}

void loop() {
  checkButtons();

  // Atualização regular de dados (se necessário)
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentTime;
    if (currentState == CHANNEL_MONITOR || currentState == MONITOR_BY_NETWORK) {
      totalNetworks = WiFi.scanNetworks();
      for (int i = 0; i < 14; i++) {
        ap_count[i] = 0;
        max_rssi[i] = -100;
      }
      for (int i = 0; i < totalNetworks && i < 20; i++) {
        int ch = WiFi.channel(i);
        ap_count[ch - 1]++;
        int rssi = WiFi.RSSI(i);
        if (rssi > max_rssi[ch - 1]) {
          max_rssi[ch - 1] = rssi;
        }
      }
      updateDisplay();
    }
  }
}
