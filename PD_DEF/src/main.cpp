// REALIZADO POR: ANDER LOZANO & ÁLVARO MARCOS RODRÍGUEZ
#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <WiFiClient.h>


// --- CONFIGURACIÓN DE USUARIO ---

const char* ssid = "Redmi";      
const char* password = "prueba123"; 
const char* admin_user = "admin";
const char* admin_pass = "admin";

// --- PINES Y UMBRALES ---
#define DHTPIN 4
const int MQ2_PIN = 12;
const float TEMP_MAXIMA = 32.0; // Temperatura actualizada
const int GAS_UMBRAL = 750;


// --- OBJETOS Y VARIABLES GLOBALES ---

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
WiFiServer server(80);

float temperaturaActual = 0.0;
float humedadActual = 0.0;
int gasActual = 0;
bool alertaTemperaturaActiva = false;
bool alertaGasActiva = false;
String session_token = "";
unsigned long previousMillisSensores = 0;
const long intervalSensores = 2000;


//  CONTENIDO WEB (HTML, JS, CSS) EN PROGMEM


// --- PÁGINA DE LOGIN ---
const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="es"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Login</title>
<style>body{font-family:Arial,sans-serif;background-color:#1a1a2e;color:white;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;}
.login-box{background-color:#16213e;padding:40px;border-radius:15px;box-shadow:0 0 20px rgba(0,0,0,0.5);width:300px;}h2{text-align:center;color:#e94560;margin-bottom:20px;}
input{width:100%;padding:10px;margin-bottom:15px;border-radius:5px;border:1px solid #0f3460;background:#1a1a2e;color:white;box-sizing:border-box;}
button{width:100%;padding:10px;border:none;border-radius:5px;background-color:#e94560;color:white;font-size:16px;cursor:pointer;}button:hover{opacity:0.8;}</style></head>
<body><div class="login-box"><h2>Acceso al Sistema</h2><form method="POST" action="/login"><input type="text" name="username" placeholder="Usuario" required><br><input type="password" name="password" placeholder="Contraseña" required><br><button type="submit">Entrar</button></form></div></body></html>
)rawliteral";

// --- PÁGINA PRINCIPAL / DASHBOARD ---
const char dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Monitor de Sala de Servidores</title>
    <style>
        :root{--bg-color:#1a1a2e;--panel-color:#16213e;--text-color:#00c49a;--primary-text:#f0f0f0;--alert-color:#e94560;}
        body{font-family:'Segoe UI',sans-serif;background-color:var(--bg-color);color:var(--primary-text);margin:0;padding:20px;box-sizing:border-box;}
        /* Animación de parpadeo para la alerta */
        @keyframes flash-red{ 0%,100%{background-color:var(--bg-color);} 50%{background-color:#5d0000;}}
        .flashing-alert{animation:flash-red 1s infinite;}
        .container{display:flex;flex-wrap:wrap;gap:20px;max-width:1400px;margin:auto;}
        .panel{background-color:var(--panel-color);border-radius:15px;padding:20px;box-shadow:0 4px 15px rgba(0,0,0,.4);}
        .panel-left{flex:1;min-width:320px;display:flex;flex-direction:column;justify-content:center;align-items:center;}
        .panel-right{flex:2;min-width:400px;display:flex;flex-direction:column;}
        .server-icon{width:120px;height:120px;margin-bottom:20px;fill:var(--text-color);transition:fill .3s;}
        .data-display{text-align:center;}
        .temp-value{font-size:5rem;font-weight:700;color:var(--text-color);}
        .hum-value{font-size:1.5rem;color:var(--primary-text);}
        .log-container{background-color:#0f121d;border:1px solid #333;border-radius:8px;height:350px;overflow-y:auto;padding:15px;font-family:'Courier New',Courier,monospace;font-size:0.9em;flex-grow:1;margin-bottom:20px;}
        .log-entry{margin-bottom:5px;}.log-time{color:#888;}.log-temp{color:#4dd0e1;}.log-hum{color:#81c784;}.log-gas{color:#ffd54f;}.log-alert{color:var(--alert-color);font-weight:bold;}
        .controls{display:flex;gap:15px;flex-wrap:wrap;align-items:center;}
        button{padding:10px 18px;font-size:14px;font-weight:700;border:none;border-radius:8px;cursor:pointer;transition:.2s;}
        .btn-stop{background-color:var(--alert-color);color:white;}
        .btn-test{background-color:#0f3460;color:white;}
        .btn-logout{background-color:#555;color:white;margin-left:auto;}
        button:hover{opacity:.8;}
        #alert-display{position:fixed;top:20px;left:50%;transform:translateX(-50%);background:var(--alert-color);color:white;padding:15px 30px;border-radius:10px;z-index:1000;font-size:1.2rem;font-weight:bold;display:none;box-shadow:0 5px 15px rgba(0,0,0,.5);}
        @media (max-width:960px){.container{flex-direction:column;}.panel-right{min-width:unset;}}
    </style>
</head>
<body>
    <div id="alert-display"></div>
    <div class="container">
        <div id="panel-left" class="panel panel-left">
            <svg id="server-svg" class="server-icon" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path d="M4,2H20A2,2 0 0,1 22,4V20A2,2 0 0,1 20,22H4A2,2 0 0,1 2,20V4A2,2 0 0,1 4,2M4,4V8H20V4H4M4,10V14H20V10H4M4,16V20H20V16H4M6,6H8V6.5H6V6M6,12H8V12.5H6V12M6,18H8V18.5H6V18M10,6.25H18V6.75H10V6.25M10,12.25H18V12.75H10V12.25M10,18.25H18V18.75H10V18.25Z"/></svg>
            <div class="data-display">
                <div class="temp-value"><span id="temp-value">--</span>°C</div>
                <div class="hum-value">Humedad: <span id="hum-value">--</span>% | Gas: <span id="gas-value">--</span></div>
            </div>
        </div>
        <div class="panel panel-right">
            <h3>Registro Histórico</h3>
            <div id="log-container" class="log-container"></div>
            <div class="controls">
                <button class="btn-stop" onclick="stopAlert()">Parar Alerta</button>
                <button class="btn-test" onclick="testAlert('temp')">Test Temp</button>
                <button class="btn-test" onclick="testAlert('gas')">Test Gas</button>
                <a href="/logout"><button class="btn-logout">Cerrar Sesión</button></a>
            </div>
        </div>
    </div>
<script>
    function handleAlert(data) {
        const alertBox = document.getElementById('alert-display');
        if (data.alert !== 'none') {
            const message = data.alert === 'gas' ? '¡ALERTA DE GAS/INCENDIO!' : '¡ALERTA DE TEMPERATURA ALTA!';
            alertBox.innerText = message;
            alertBox.style.display = 'block';
            document.body.classList.add('flashing-alert'); // <-- PARPADEO REACTIVADO
            document.getElementById('server-svg').style.fill = 'var(--alert-color)';
        } else {
            alertBox.style.display = 'none';
            document.body.classList.remove('flashing-alert'); // <-- PARPADEO REACTIVADO
            document.getElementById('server-svg').style.fill = 'var(--text-color)';
        }
    }
    function addLogEntry(data) {
        const logContainer = document.getElementById('log-container');
        const time = new Date().toLocaleTimeString('es-ES');
        let entry = `<div class="log-entry"><span class="log-time">[${time}]</span> `;
        entry += `<span class="log-temp">T: ${data.t.toFixed(1)}°C</span> / `;
        entry += `<span class="log-hum">H: ${data.h.toFixed(1)}%</span> / `;
        entry += `<span class="log-gas">Gas: ${data.g}</span>`;
        if (data.alert === 'gas') entry += ` <span class="log-alert">-> INCENDIO DETECTADO!</span>`;
        else if (data.alert === 'temp') entry += ` <span class="log-alert">-> TEMPERATURA ALTA!</span>`;
        entry += `</div>`;
        logContainer.innerHTML += entry;
        logContainer.scrollTop = logContainer.scrollHeight;
    }
    function fetchData() {
        fetch('/data')
            .then(response => {
                if (!response.ok) { if(response.status === 403) window.location.href = '/'; return; }
                return response.json();
            })
            .then(data => {
                if(!data) return;
                document.getElementById('temp-value').innerText = data.t.toFixed(1);
                document.getElementById('hum-value').innerText = data.h.toFixed(1);
                document.getElementById('gas-value').innerText = data.g;
                addLogEntry(data);
                handleAlert(data);
            }).catch(error => console.error('Error:', error));
    }
    function stopAlert() { fetch('/stop_alert').then(fetchData); }
    function testAlert(type) { fetch('/test?type=' + type); }
    document.addEventListener('DOMContentLoaded', setInterval(fetchData, 2000));
</script>
</body></html>
)rawliteral";


// --- FUNCIONES AUXILIARES ---

void leerSensores() {
    temperaturaActual = dht.readTemperature();
    humedadActual = dht.readHumidity();
    gasActual = analogRead(MQ2_PIN);

    if (isnan(temperaturaActual)) temperaturaActual = -99.0;
    if (isnan(humedadActual)) humedadActual = -99.0;
    
    // --- LÓGICA DE ALERTAS CORREGIDA PARA QUE LOS BOTONES DE TEST FUNCIONEN ---
    alertaGasActiva = alertaGasActiva || (gasActual > GAS_UMBRAL);
    
    if (!alertaGasActiva) {
        alertaTemperaturaActiva = alertaTemperaturaActiva || (temperaturaActual > TEMP_MAXIMA);
    } else {
        // Si hay una alerta de gas, la de temperatura se basa solo en el valor real
        alertaTemperaturaActiva = (temperaturaActual > TEMP_MAXIMA);
    }
    // No hay llamadas a funciones de notificación aquí
}

void create_session_token() { session_token = String(random(0, 0x7fffffff), HEX); }

bool isAuthenticated(String& req) {
    if (session_token == "") return false;
    if (req.indexOf("Cookie: session-id=" + session_token) != -1) return true;
    return false;
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(MQ2_PIN, INPUT);
    analogReadResolution(12);

    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi...");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());
    
    server.begin();
 }
 
   // --- LOOP PRINCIPAL CORREGIDO ---
void loop() {
    if (millis() - previousMillisSensores >= intervalSensores) {
        previousMillisSensores = millis();
        leerSensores();
    }

    WiFiClient client = server.available();
    if (client) {
        String req_header = ""; String req_body = ""; bool is_post = false;
        String currentLine = "";

        // Bucle para leer la petición del cliente
        while (client.connected() && client.available()) {
            char c = client.read();
            req_header += c; // Construimos el header completo para la autenticación

            if (c == '\n') {
                if (currentLine.length() == 0) { // Línea en blanco, fin de las cabeceras
                    if (is_post) {
                        // Leer el cuerpo del POST si existe
                        while(client.available()){
                           req_body += (char)client.read();
                        }
                    }
                    break;
                } else {
                    if (currentLine.startsWith("POST")) is_post = true;
                    currentLine = "";
                }
            } else if (c != '\r') {
                currentLine += c;
            }
        }
        
        // --- GESTIÓN DE RUTAS (ROUTING) ---
        if (req_header.indexOf("POST /login") >= 0) {
            if (req_body.indexOf("username=" + String(admin_user)) >= 0 && req_body.indexOf("password=" + String(admin_pass)) >= 0) {
                create_session_token();
                // CORRECTO: Enviar cada cabecera en una línea separada
                client.println("HTTP/1.1 302 Found");
                client.println("Location: /");
                client.println("Set-Cookie: session-id=" + session_token + "; Path=/");
                client.println(); // Línea en blanco para terminar las cabeceras
            } else { 
                // CORRECTO: Redirección simple
                client.println("HTTP/1.1 302 Found");
                client.println("Location: /");
                client.println();
            }
        } else if (req_header.indexOf("GET /logout") >= 0) {
            session_token = "";
            // CORRECTO: Redirección y borrado de cookie
            client.println("HTTP/1.1 302 Found");
            client.println("Location: /");
            client.println("Set-Cookie: session-id=; Max-Age=0"); // Max-Age=0 le dice al navegador que la borre
            client.println();
        } else if (req_header.indexOf("GET /data") >= 0) {
            if (!isAuthenticated(req_header)) { 
                client.println("HTTP/1.1 403 Forbidden");
                client.println();
            } else {
                // CORRECTO: Cabeceras para JSON
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: application/json");
                client.println("Connection: close"); // Buena práctica para cerrar la conexión
                client.println(); // Fin de cabeceras
                
                String alertType = alertaGasActiva ? "gas" : (alertaTemperaturaActiva ? "temp" : "none");
                String json = "{\"t\":" + String(temperaturaActual, 1) + ",\"h\":" + String(humedadActual, 1) + ",\"g\":" + String(gasActual) + ",\"alert\":\"" + alertType + "\"}";
                client.print(json);
            }
        } else if (req_header.indexOf("GET /stop_alert") >= 0) {
            if(isAuthenticated(req_header)) { 
                alertaGasActiva = false; 
                alertaTemperaturaActiva = false; 
                client.println("HTTP/1.1 200 OK");
                client.println();
            } else { 
                client.println("HTTP/1.1 403 Forbidden");
                client.println();
            }
        } else if (req_header.indexOf("GET /test?type=temp") >= 0) {
            if(isAuthenticated(req_header)) { 
                alertaTemperaturaActiva = true; 
                client.println("HTTP/1.1 200 OK");
                client.println();
            } else { 
                client.println("HTTP/1.1 403 Forbidden");
                client.println();
            }
        } else if (req_header.indexOf("GET /test?type=gas") >= 0) {
            if(isAuthenticated(req_header)) { 
                alertaGasActiva = true; 
                client.println("HTTP/1.1 200 OK");
                client.println();
            } else { 
                client.println("HTTP/1.1 403 Forbidden");
                client.println();
            }
        } else if (req_header.indexOf("GET / ") >= 0) {
            // CORRECTO: Cabeceras para HTML
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            
            if (isAuthenticated(req_header)) {
                client.print(dashboard_html);
            } else {
                client.print(login_html);
            }
        } else { 
            client.println("HTTP/1.1 404 Not Found");
            client.println();
        }

        delay(1); // Pequeña espera para asegurar que se envían los datos
        client.stop();
    }
}