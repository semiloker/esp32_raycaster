#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS     5  // Chip select
#define TFT_RST    16  // Reset
#define TFT_DC     17 // Data/command

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

AsyncWebServer server(80);
const char* ssid = "Mercusys";
const char* password = "syksyn94";

int gameMap[8][8] = 
{
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1}
};

float playerX = 1.5, playerY = 1.5;
float playerAngle = 0;

void setup() 
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected!");

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);

  tft.fillScreen(ST77XX_BLACK);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
  {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>ESP32 Raycaster</title>
        <style>
          body 
          {
            font-family: Arial, sans-serif;
            background-color: #2c2f3d;
            color: white;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
          }

          h1 
          {
            text-align: center;
          }

          #controller 
          {
            display: grid;
            grid-template-columns: 100px 100px 100px;
            grid-template-rows: 100px 100px 100px;
            gap: 10px;
            margin-top: 20px;
          }

          .button 
          {
            width: 80px;
            height: 80px;
            background-color: #4CAF50;
            border: none;
            border-radius: 50%;
            color: white;
            font-size: 20px;
            cursor: pointer;
            transition: background-color 0.2s;
          }

          .button:hover 
          {
            background-color: #45a049;
          }

          .button:active 
          {
            background-color: #388e3c;
          }

          .button:focus 
          {
            outline: none;
          }

          #position 
          {
            margin-top: 20px;
            font-size: 18px;
            text-align: center;
          }

        </style>
        <script>
          document.addEventListener('keydown', (event) => 
          {
            const key = event.key.toLowerCase();
            if ('wasdqe'.includes(key)) 
            {
              fetch(`/move?key=${key}`);
            }
          });

          function move(direction) {
            fetch(`/move?key=${direction}`);
          }
        </script>
      </head>
      <body>
        <h1>ESP32 Raycaster</h1>
        <p>Use W, A, S, D to move and Q, E to rotate the camera.</p>
        <div id="controller">
          <button class="button" onclick="move('w')">W</button>
          <button class="button" onclick="move('a')">A</button>
          <button class="button" onclick="move('d')">D</button>
          <button class="button" onclick="move('s')">S</button>
          <button class="button" onclick="move('q')">Q</button>
          <button class="button" onclick="move('e')">E</button>
        </div>
        <div id="position">
          <p>Player Position: X: <span id="posX">0.00</span> Y: <span id="posY">0.00</span></p>
          <p>Player Angle: <span id="angle">0.00</span></p>
        </div>
      </body>
      </html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  server.on("/move", HTTP_GET, [](AsyncWebServerRequest *request)
   {
    if (request->hasParam("key"))
     {
      String key = request->getParam("key")->value();

      if (key == "w") 
      {
        playerX += cos(playerAngle) * 0.1;
        playerY += sin(playerAngle) * 0.1;
      } else if (key == "s") 
      {
        playerX -= cos(playerAngle) * 0.1;
        playerY -= sin(playerAngle) * 0.1;
      } else if (key == "a") 
      {
        playerX -= sin(playerAngle) * 0.1;
        playerY += cos(playerAngle) * 0.1;
      } else if (key == "d") 
      { 
        playerX += sin(playerAngle) * 0.1;
        playerY -= cos(playerAngle) * 0.1;
      } else if (key == "q") 
      {
        playerAngle += 0.1;
      } else if (key == "e") 
      { 
        playerAngle -= 0.1;
      }
    }
    String positionInfo = "{\"posX\": \"" + String(playerX, 2) + "\", \"posY\": \"" + String(playerY, 2) + "\", \"angle\": \"" + String(playerAngle, 2) + "\"}";
    request->send(200, "application/json", positionInfo);
  });

  server.begin();
}

void render() 
{
  tft.fillScreen(ST77XX_BLACK);

  int screenWidth = tft.width();
  int screenHeight = tft.height();

  for (int x = 0; x < screenWidth; x++) 
  {
    float rayAngle = (playerAngle - 0.2) + (x / (float)screenWidth) * 0.4;
    float rayX = cos(rayAngle), rayY = sin(rayAngle);

    float distance = 0;
    while (distance < 16) 
    {
      int mapX = int(playerX + rayX * distance);
      int mapY = int(playerY + rayY * distance);
      if (gameMap[mapY][mapX] == 1) break;
      distance += 0.1;
    }

    int lineHeight = int(screenHeight / distance);
    int drawStart = (screenHeight - lineHeight) / 2;
    int drawEnd = (screenHeight + lineHeight) / 2;

    tft.drawLine(x, drawStart, x, drawEnd, ST77XX_WHITE);
  }
}

void loop() 
{
  render();
  delay(30);
}