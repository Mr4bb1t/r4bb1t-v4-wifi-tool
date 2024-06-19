#include <TFT_eSPI.h>  // Biblioteca para o display TFT
#include <SPI.h>
#include <WiFi.h>

// Pinos dos botões
#define BUTTON_UP 22
#define BUTTON_DOWN 21
#define BUTTON_SELECT 4

// Inicializa o display TFT
TFT_eSPI tft = TFT_eSPI();  

// Variáveis de estado
unsigned long lastUpdateTime = 0;    // Último tempo de atualização
int previousPacketCount = -1;        // Contador anterior de pacotes
bool initialUpdateDone = false;      // Flag para indicar a primeira atualização
const unsigned long updateInterval = 1000;  // Intervalo desejado de atualização em milissegundos
int currentSelection = 0;
const int totalOptions = 4;
bool inSubMenu = false;
bool inNetworkList = false;
bool inNetworkInfo = false;
bool inChannelMonitor = false;
bool inintensityMonitor = false;
int subMenuSelection = 0;
const int subMenuOptions = 5;  // Adicionado "Back" como opção extra
int networkSelection = 0;
int totalNetworks = 0;
int currentChannel = 1;  // Canal inicial para monitoramento por canal
String networks[20];  // Suporte para até 20 redes WiFi

// Variáveis para armazenar informações da varredura inicial
uint8_t ap_count[14] = {0};
int32_t max_rssi[14] = {-100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100};
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

void drawText(int x, int y, const char* text, uint16_t color) {
  tft.setCursor(x, y);
  tft.setTextColor(color, TFT_BLACK);  // Cor do texto e fundo
  tft.setTextSize(2);
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

  // Botão de voltar
  y += lineSpacing + 5; // Espaçamento extra para o botão "Back"
  tft.setTextColor(highlightColor, backgroundColor);
  tft.setCursor(x + 80, y);
  tft.print("Back");
  tft.drawRect(5, y - 5, tft.width() - 10, 30, highlightColor); // Ajuste as dimensões conforme necessário
}

// Tela de carregamento
void drawLoadingScreen() {
  tft.setTextSize(4);
  tft.fillScreen(TFT_BLACK);
  // Desenhar uma borda colorida ao redor
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_RED);
  tft.drawRect(1, 1, tft.width() - 2, tft.height() - 2, TFT_RED);
  drawText(60, 90, "r4bb1t", TFT_RED);
}

// Função para converter o tipo de criptografia em string
String encryptionTypeToString(uint8_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN:
      return "Open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA/PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2/PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/WPA2/PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2 Enterprise";
    default:
      return "Unknown";
  }
}

void monitorPacketsByChannel() {
  tft.fillScreen(TFT_BLACK);

  // Atraso inicial de 2 segundos
  delay(2000);

  while (inChannelMonitor) {
    unsigned long currentTime = millis();

    // Verificar se é hora de atualizar a tela (aproximadamente a cada segundo)
    if (currentTime - lastUpdateTime >= updateInterval || !initialUpdateDone) {
      // Limpar a parte da tela onde os dados são exibidos
      tft.fillRect(0, 0, tft.width(), tft.height() - 20, TFT_BLACK);

      // Exibir informações do canal
      drawText(10, 10, ("Channel: " + String(currentChannel)).c_str(), TFT_WHITE);

      // Contar pacotes no canal atual
      int packetCount = 0;
      for (int i = 0; i < totalNetworks; i++) {
        if (initial_channel[i] == currentChannel) {
          packetCount++;
        }
      }

      // Atualizar apenas se a quantidade de pacotes mudar ou na primeira atualização
      if (packetCount != previousPacketCount || !initialUpdateDone) {
        drawText(10, 40, ("Packets: " + String(packetCount)).c_str(), TFT_WHITE);
        previousPacketCount = packetCount;
        lastUpdateTime = currentTime;
        initialUpdateDone = true;
      }
    }

    // Navegação de canais
    if (digitalRead(BUTTON_UP) == LOW) {
      currentChannel = (currentChannel % 13) + 1;  // Incrementar o canal
      delay(200);  // Debounce
    }
    if (digitalRead(BUTTON_DOWN) == LOW) {
      currentChannel = (currentChannel == 1) ? 13 : (currentChannel - 1);  // Decrementar o canal
      delay(200);  // Debounce
    }
    if (digitalRead(BUTTON_SELECT) == LOW) {
      inChannelMonitor = false;
      drawSubMenu();
      delay(200);  // Debounce
    }
  }
}

    // Navegação de canais
    if (digitalRead(BUTTON_UP) == LOW) {
      currentChannel = (currentChannel % 13) + 1;  // Incrementar o canal
      delay(200);  // Debounce
    }
    if (digitalRead(BUTTON_DOWN) == LOW) {
      currentChannel = (currentChannel == 1) ? 13 : (currentChannel - 1);  // Decrementar o canal
      delay(200);  // Debounce
    }
    if (digitalRead(BUTTON_SELECT) == LOW) {
      inChannelMonitor = false;
      drawSubMenu();
      delay(200);  // Debounce
    }


// Monitorar intensidade de sinal
void monitorintensity() {
  tft.fillScreen(TFT_BLACK);

  // Inicializar estatísticas de canais
  for (int i = 0; i < 14; i++) {
    ap_count[i] = 0;
    max_rssi[i] = -100;
  }

  for (int i = 0; i < totalNetworks; i++) {
    int channel = initial_channel[i];
    int rssi = initial_rssi[i];

    int height = map(rssi, -100, 0, 0, 240);  // Mapear RSSI para altura da barra

    uint16_t color = channel_color[channel - 1];

    // channel stat
    ap_count[channel - 1]++;
    if (rssi > max_rssi[channel - 1]) {
      max_rssi[channel - 1] = rssi;
    }

    // draw RSSI bar
    tft.fillRect(26 * (channel - 1) + 2, 240 - height, 24, height, color);
    tft.setTextColor(color, TFT_BLACK);
    tft.setCursor(26 * (channel - 1) + 2, 242 - height);
    tft.println(rssi);
  }

  // draw graph x-axis
  for (int i = 0; i < 14; i++) {
    tft.setTextColor(channel_color[i]);
    tft.setCursor(26 * i + 2, 0);
    tft.print(i + 1);
  }
}

void monitorIntensityLoop() {
  while (inintensityMonitor) {
    monitorintensity();
    delay(1000);  // Delay entre atualizações, ajuste conforme necessário

    if (digitalRead(BUTTON_SELECT) == LOW) {
      inintensityMonitor = false;
      drawSubMenu();
      delay(200);  // Debounce
    }
  }
}

// Configuração inicial
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(3); // Ajustar a rotação conforme necessário
  tft.setTextSize(4);
  drawLoadingScreen();  // Mostrar tela de carregamento

  // Inicializar WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  totalNetworks = WiFi.scanNetworks();
  for (int i = 0; i < totalNetworks && i < 20; i++) {
    networks[i] = WiFi.SSID(i);
    initial_rssi[i] = WiFi.RSSI(i);
    initial_channel[i] = WiFi.channel(i);
  }
  
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  
  // Desenhar a interface inicial
  drawMainMenu();
}

// Função principal
void loop() {
  if (digitalRead(BUTTON_UP) == LOW) {
    if (inSubMenu) {
      subMenuSelection = (subMenuSelection - 1 + subMenuOptions) % subMenuOptions;
      drawSubMenu();
    } else if (inNetworkList) {
      networkSelection = (networkSelection - 1 + (totalNetworks + 1)) % (totalNetworks + 1);
      drawNetworkList();
    } else if (!inNetworkInfo && !inintensityMonitor && !inChannelMonitor) {
      currentSelection = (currentSelection - 1 + totalOptions) % totalOptions;
      drawMainMenu();
    }
    delay(200);  // Debounce
  }
  
  if (digitalRead(BUTTON_DOWN) == LOW) {
    if (inSubMenu) {
      subMenuSelection = (subMenuSelection + 1) % subMenuOptions;
      drawSubMenu();
    } else if (inNetworkList) {
      networkSelection = (networkSelection + 1) % (totalNetworks + 1);
      drawNetworkList();
    } else if (!inNetworkInfo && !inintensityMonitor && !inChannelMonitor) {
      currentSelection = (currentSelection + 1) % totalOptions;
      drawMainMenu();
    }
    delay(200);  // Debounce
  }
  
  if (digitalRead(BUTTON_SELECT) == LOW) {
    if (inSubMenu) {
      if (subMenuSelection == 3) {  // Network Info
        inNetworkList = true;
        inSubMenu = false;  // Exit submenu
        drawNetworkList();
      } else if (subMenuSelection == 4) {  // Back
        inSubMenu = false;
        drawMainMenu();
      } else if (subMenuSelection == 2) {  // Monitor intensity
        inintensityMonitor = true;
        inSubMenu = false;  // Exit submenu
        monitorIntensityLoop();
      } else if (subMenuSelection == 0) {  // Monitor by Channel
        inChannelMonitor = true;
        inSubMenu = false;  // Exit submenu
        monitorPacketsByChannel();
      } else {
        // Actions for other submenu selections
        Serial.println(subMenuSelection);  // For testing purposes
      }
    } else if (inNetworkList) {
      if (networkSelection == 0) {  // Back
        inNetworkList = false;
        inSubMenu = true;  // Return to submenu
        drawSubMenu();
      } else {
        // Display information for the selected network
        inNetworkInfo = true;
        inNetworkList = false;  // Exit network list
        ssid = WiFi.SSID(networkSelection - 1);
        bssid = WiFi.BSSIDstr(networkSelection - 1);
        rssi = WiFi.RSSI(networkSelection - 1);
        channel = WiFi.channel(networkSelection - 1);
        encryptionType = encryptionTypeToString(WiFi.encryptionType(networkSelection - 1));
        drawNetworkInfo();
      }
    } else if (inNetworkInfo) {
      // Return to the network list from network info
      inNetworkInfo = false;
      inNetworkList = true;
      networkSelection = 0;  // Reset the selection to "Back"
      drawNetworkList();
    } else {
      if (currentSelection == 0) {
        inSubMenu = true;
        drawSubMenu();
      } else {
        // Logic for other main menu options
        Serial.println(currentSelection);  // For testing purposes
      }
    }
    delay(200);  // Debounce
  }
}
