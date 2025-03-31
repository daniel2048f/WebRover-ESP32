#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// Configuración del Access Point (AP)
const char* ssid = "COCHERC";
const char* password = "87654321";

// Configuración de IP fija
IPAddress local_IP(192,168,4,4);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);
Servo servoDireccion;

// Pines del controlador
int In2 = 25;   // Dirección 1
int In3 = 33;   // Dirección 2
int ENB = 32;   // PWM del motor

// Pin del servomotor
int servoPin = 13;

// Variables de control
char modo = 'P';
int velocidad = 168;  // Valor inicial (centro del rango)
int anguloServo = 90; // Posición inicial

const char* html = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>Control Coche RC</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <style>
        body {
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
            background: #1a1a1a;
            font-family: Arial, sans-serif;
            color: white;
            overflow: hidden;
            touch-action: none;
        }

        h1 {
            color: #00ff88;
            margin: 20px 0;
            font-size: 2em;
            text-shadow: 0 0 10px #00ff88;
        }

        .panel-control {
            background: #2d2d2d;
            padding: 25px;
            border-radius: 15px;
            width: 100%;
            max-width: 400px;
            box-shadow: 0 0 20px rgba(0,255,136,0.2);
        }

        .control-deslizante {
            margin: 20px 0;
        }

        label {
            display: block;
            margin-bottom: 10px;
            font-size: 1.1em;
            color: #00ff88;
        }

        input[type="range"] {
            width: 100%;
            height: 10px;
            background: #4a4a4a;
            border-radius: 5px;
            outline: none;
            -webkit-appearance: none;
        }

        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 25px;
            height: 25px;
            background: #00ff88;
            border-radius: 50%;
            cursor: pointer;
            box-shadow: 0 0 10px #00ff88;
        }

        .valor-actual {
            display: inline-block;
            padding: 5px 15px;
            background: #00ff88;
            color: #1a1a1a;
            border-radius: 5px;
            margin-left: 10px;
            font-weight: bold;
        }

        .botones-mando {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 15px;
            margin-top: 30px;
        }

        .boton {
            padding: 20px;
            border: none;
            border-radius: 10px;
            font-size: 1.2em;
            cursor: pointer;
            transition: all 0.2s;
            background: #4a4a4a;
            color: white;
        }

        .boton:hover {
            transform: scale(1.05);
            box-shadow: 0 0 15px #00ff88;
        }

        #avanzar { background: #00cc66; }
        #reversa { background: #ff4444; }
        #parar { background: #ffaa00; }
    </style>
</head>
<body>
    <h1>Control Remoto</h1>
    
    <div class="panel-control">
        <div class="control-deslizante">
            <label>Velocidad (85-230):</label>
            <input type="range" min="85" max="230" value="168" id="velocidadSlider"> 
            <!--Cambiar limites de slider velocidad aca-->
            <span class="valor-actual" id="velocidadValor">168</span>
        </div>

        <div class="control-deslizante">
            <label>Direccion (20-160):</label>
            <input type="range" min="20" max="160" value="90" id="direccionSlider"> 
            <!--Cambiar limites del slider angulo aca-->
            <span class="valor-actual" id="direccionValor">90</span>
        </div>

        <div class="botones-mando">
            <button class="boton" id="avanzar">Avanzar</button>
            <button class="boton" id="parar">Parar</button>
            <button class="boton" id="reversa">Reversa</button>
        </div>
    </div>

    <script>
        const velocidadSlider = document.getElementById('velocidadSlider');
        const direccionSlider = document.getElementById('direccionSlider');

        function enviarComando(comando) {
            fetch(`/command?cmd=${comando}`);
        }

        function actualizarVelocidad(valor) {
            valor = Math.max(85, Math.min(230, valor)); //Solo acepta un valor entre 85 y 230. Cambiar limites de slider velocidad aca
            fetch(`/velocidad?value=${valor}`);
            document.getElementById("velocidadValor").textContent = valor;
        }

        function actualizarDireccion(valor) {
            valor = Math.max(20, Math.min(160, valor)); //Solo acepta un valor entre 20 y 160. Cambiar limites del slider angulo aca
            fetch(`/direccion?value=${valor}`);
            document.getElementById("direccionValor").textContent = `${valor}`;
        }

        // Eventos
        velocidadSlider.addEventListener('input', (e) => {
            actualizarVelocidad(parseInt(e.target.value));
        });

        direccionSlider.addEventListener('input', (e) => {
            actualizarDireccion(parseInt(e.target.value));
        });

        document.getElementById('avanzar').addEventListener('click', () => {
            enviarComando('avanzar');
        });

        document.getElementById('reversa').addEventListener('click', () => {
            enviarComando('reversa');
        });

        document.getElementById('parar').addEventListener('click', () => {
            enviarComando('parar');
        });
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);

    // Configurar pines
    pinMode(In2, OUTPUT);
    pinMode(In3, OUTPUT);
    pinMode(ENB, OUTPUT);

    // Configurar PWM para motor
    ledcSetup(0, 10000, 8);
    ledcAttachPin(ENB, 0);

    // Configurar servo
    ESP32PWM::allocateTimer(3);
    servoDireccion.setPeriodHertz(50);
    servoDireccion.attach(servoPin, 500, 2400);
    servoDireccion.write(anguloServo);

    // Iniciar modo AP
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(ssid, password);

    // Configurar rutas
    server.on("/", []() {
        server.send(200, "text/html", html);
    });

    server.on("/command", []() {
        String cmd = server.arg("cmd");
        if (cmd == "avanzar") {
            modo = 'A';
            digitalWrite(In2, LOW);
            digitalWrite(In3, HIGH);
            ledcWrite(0, velocidad);
        }
        else if (cmd == "reversa") {
            modo = 'R';
            digitalWrite(In2, HIGH);
            digitalWrite(In3, LOW);
            ledcWrite(0, velocidad);
        }
        else if (cmd == "parar") {
            modo = 'P';
            digitalWrite(In2, LOW);
            digitalWrite(In3, LOW);
            ledcWrite(0, 0);
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/velocidad", []() {
        if (server.hasArg("value")) {
            velocidad = server.arg("value").toInt();
            if (modo != 'P') {
                ledcWrite(0, velocidad);
            }
            Serial.print("Velocidad: ");
            Serial.println(velocidad);
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/direccion", []() {
        if (server.hasArg("value")) {
            anguloServo = server.arg("value").toInt();
            servoDireccion.write(anguloServo);
            Serial.print("Dirección: ");
            Serial.println(anguloServo);
        }
        server.send(200, "text/plain", "OK");
    });

    // Estado inicial
    digitalWrite(In2, LOW);
    digitalWrite(In3, LOW);
    ledcWrite(0, 0);
    server.begin();
}

void loop() {
    server.handleClient();
}
