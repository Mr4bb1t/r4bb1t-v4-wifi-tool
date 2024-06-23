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

void drawNetworkList() {
  tft.fillScreen(TFT_BLACK);

  uint16_t highlightColor = TFT_RED;
  uint16_t normalColor = TFT_WHITE;

  // Opção de voltar
  drawText(10, 30, "Back", networkSelection == 0 ? highlightColor : normalColor);
  if (networkSelection == 0) {
    tft.drawRect(0, 20, 280, 30, highlightColor);
  }

  // Listar redes WiFi
  int startIndex = networkSelection - 1; // adjust the start index based on the current selection
  if (startIndex < 0) startIndex = 0; // ensure we don't go out of bounds

  for (int i = startIndex; i < totalNetworks && i < startIndex + 5; i++) {
    String networkName = networks[i];
    int maxLength = 17; // adjust this value to fit your screen width
    if (networkName.length() > maxLength) {
      networkName = networkName.substring(0, maxLength) + "...";
    }
    drawText(10, 60 + (i - startIndex) * 30, networkName.c_str(), networkSelection == (i + 1) ? highlightColor : normalColor);
    if (networkSelection == (i + 1)) {
      tft.drawRect(0, 50 + (i - startIndex) * 30, 280, 30, highlightColor);
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

bool redesenhoNecessario = true;

void drawNetworkMonitor() {
  if (redesenhoNecessario) {
  tft.fillScreen(TFT_BLACK);

  uint16_t highlightColor = TFT_RED;
  uint16_t normalColor = TFT_WHITE;

  // Opção de voltar
  drawText(10, 30, "Back", networkSelection == 0? highlightColor : normalColor);
  if (networkSelection == 0) {
    tft.drawRect(0, 20, 240, 30, highlightColor);
  }
  
  // Listar redes WiFi
  int startIndex = networkSelection - 1; // ajuste o índice de início com base na seleção atual
  if (startIndex < 0) startIndex = 0; // certifique-se de que não vá além dos limites
  for (int i = startIndex; i < totalNetworks && i < startIndex + 5; i++) {
    String networkName = networks[i];
    int maxLength = 20; // ajuste esse valor para caber na largura da tela
    if (networkName.length() > maxLength) {
      networkName = networkName.substring(0, maxLength) + "...";
    }
    drawText(10, 60 + (i - startIndex) * 30, networkName.c_str(), networkSelection == (i + 1)? highlightColor : normalColor);
    if (networkSelection == (i + 1)) {
      tft.drawRect(0, 50 + (i - startIndex) * 30, 240, 30, highlightColor);
       redesenhoNecessario = false;
     }
    }
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

#define MAX_NETWORKS 5  // Define MAX_NETWORKS como 20, ou outro valor apropriado para o seu projeto

#define GRAPH_BASELINE 200
#define CHANNEL_WIDTH 20
#define RSSI_FLOOR -100
#define RSSI_CEILING -40
#define GRAPH_HEIGHT 100

int n; // number of WiFi networks found

void drawWiFiIntensityGraph() {
  // Limpa a tela
  tft.fillScreen(TFT_BLACK);

  // Desenha o eixo x (canais)
  tft.drawFastHLine(0, GRAPH_BASELINE, 320, TFT_WHITE);
  for (int i = 1; i <= 14; i++) {
    tft.setTextColor(channel_color[i - 1]);
    tft.setCursor((i * CHANNEL_WIDTH) - ((i < 10)?3:6), GRAPH_BASELINE + 2);
    tft.print(i);
  }

  // Desenha as barras de intensidade do sinal WiFi
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; i++) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    int32_t channel = WiFi.channel(i);

    uint16_t color = channel_color[channel - 1];
    int height = constrain(map(rssi, RSSI_FLOOR, RSSI_CEILING, 1, GRAPH_HEIGHT), 1, GRAPH_HEIGHT);

    tft.fillRect((channel * CHANNEL_WIDTH) - (CHANNEL_WIDTH / 2), GRAPH_BASELINE - height, CHANNEL_WIDTH, height, color);

    // Mostra o nome da rede WiFi
    tft.setTextColor(TFT_WHITE);
    tft.setCursor((channel * CHANNEL_WIDTH) - (CHANNEL_WIDTH / 2), GRAPH_BASELINE - height - 10);
    tft.print(ssid);
  }
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
      drawWiFiIntensityGraph();
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
        drawWiFiIntensityGraph();
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
  if (debounceButton(BUTTON_UP)) {
    if (currentState == MONITOR_BY_NETWORK) {
      networkSelection = (networkSelection - 1 + totalNetworks + 1) % (totalNetworks + 1);
      drawNetworkMonitor();
    }
  }

  if (debounceButton(BUTTON_DOWN)) {
    if (currentState == MONITOR_BY_NETWORK) {
      networkSelection = (networkSelection + 1) % (totalNetworks + 1);
      drawNetworkMonitor();
    }
  }

  if (debounceButton(BUTTON_SELECT)) {
    if (currentState == MONITOR_BY_NETWORK) {
      showNetworkInfo();
    }
  }


    updateDisplay();
  }
}

boolean dataChanged = false; // Declare the flag as a global variable

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
      dataChanged = true; // Set the flag to indicate that data has changed
    }
  }

  if (dataChanged) {
    updateDisplay(); // Only call updateDisplay() if data has changed
    checkButtons(); // Call checkButtons() again to process button presses
    dataChanged = false; // Reset the flag
  }
}
