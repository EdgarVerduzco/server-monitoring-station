
# 📡 TIVEG Concentrador SMS → JSON → API  

<div align="center">
    <a href="#"><img src="https://img.shields.io/badge/version-1.0.0-blue.svg" alt="Version"></a>
    <a href="#"><img src="https://img.shields.io/badge/platform-ESP32-green.svg" alt="ESP32"></a>
    <a href="#"><img src="https://img.shields.io/badge/modem-SIM800L-orange.svg" alt="SIM800L"></a>
    <a href="#"><img src="https://img.shields.io/badge/license-MIT-yellow.svg" alt="License"></a>
    <br>
</div>

<div align="center">
    <p><img src="resource/topologia.png" width="400px"></p>
    <br/>   
</div>

Este proyecto contiene el firmware y documentación para el **Concentrador TIVEG**, un sistema modular que recibe SMS en formato **PDU/UDH**, los reconstruye y sanitiza, y los envía como **JSON** hacia una API HTTP/HTTPS.

---

## 📂 Estructura de Archivos  

```
tiveg_concentrador/
├── docs/                     # Documentación adicional
├── resources/                # Recursos gráficos (diagramas, imágenes)
│   └── topologia.png
├── firmware/
│   ├── main.ino              # Código principal (URCs, barridos, envío API)
│   ├── config.h              # Selección de tarjeta, pines y parámetros globales
│   ├── secrets.h             # Credenciales privadas (SSID, password, API_URL)
│   ├── wifi_config.h         # Conexión y reconexión WiFi
│   ├── http_utils.h          # Manejo de HTTP/HTTPS con retries
│   ├── JsonQueue.h           # Cola FIFO de mensajes JSON
│   ├── SIM800SmsManager.h    # Clase para comandos AT y envío de SMS
│   ├── SIM800SmsManager.cpp
│   ├── SmsPduParser.h        # Parser de PDU → texto
│   ├── SmsPduParser.cpp
│   ├── SmsConcatManager.h    # Ensamblado de SMS multipartes
│   └── SmsConcatManager.cpp
└── README.md                 # Este archivo
```

---

## 📑 Estructura del JSON

El formato de datos JSON que utiliza el concentrador incluye los siguientes campos básicos:

```json
{
    "from": "+521234567890",     // Número de origen del SMS
    "text": "{ 'cmd':'status' }", // Contenido del SMS (ya sanitizado)
    "ts": "2025-09-05 12:33:21", // Marca de tiempo de recepción
    "id": 31                     // Índice interno de mensaje en SIM antes de borrarse
}
````

> **Nota:** Los SMS multipart se ensamblan automáticamente en un solo `text` antes de enviarse a la API.

---

## 🖥️ Modo Consola

El concentrador cuenta con un **modo consola** para debug y comandos manuales.
Se activa enviando:

```
###
```

y permite interactuar directamente con el SIM800L.

### Ejemplo: SMS atascados por corrupción de multipartes

```
[SWEEP] Encontrado SMS idx=31 (status=1)
[SWEEP] Encontrado SMS idx=32 (status=1)
...
+CMGL: 31,1,"",156
+CMGL: 32,1,"",78
```

**Comandos útiles:**

| Acción                 | Comando AT                     | Descripción                      |
| ---------------------- | ------------------------------ | -------------------------------- |
| Listar mensajes UNREAD | `AT+CMGL=0`                    | Muestra los SMS no leídos        |
| Listar mensajes READ   | `AT+CMGL=1`                    | Muestra los SMS ya leídos        |
| Borrar un mensaje      | `AT+CMGD=31`                   | Borra SMS en índice 31           |
| Borrar varios          | `AT+CMGD=31` <br> `AT+CMGD=32` | Borra múltiples mensajes         |
| Confirmar limpieza     | `AT+CMGL=1`                    | Verifica que los SMS ya no estén |

---

## ⚙️ Configuración rápida

### `secrets.h`

```cpp
#define WIFI_SSID     "MiRedWiFi"
#define WIFI_PASSWORD "MiPassword"
#define API_URL       "http://192.168.1.10:5000/api/data"
#define MIRROR_URL    ""   // opcional
```

### `config.h`

```cpp
#define USE_BOARD 0       // 0 = TTGO T-Call, 1 = DualMCU ONE
#define USE_HTTPS 1       // 0 = HTTP, 1 = HTTPS
#define HTTPS_INSECURE 1  // 0 = validar cert, 1 = ignorar (solo pruebas)
#define SIM_BAUD 9600
```

---

## 📝 Notas Importantes

* ⚡ Fuente estable ≥ **2 A** para el SIM800L (picos altos).
* 📶 Solo funciona en redes **2.4 GHz**.
* ✅ En producción: `USE_HTTPS=1` y `HTTPS_INSECURE=0`.
* 🧹 Los SMS procesados se borran automáticamente de la memoria SIM.

---

## 🐞 Problemas Comunes

| Problema            | Causa                      | Solución                         |
| ------------------- | -------------------------- | -------------------------------- |
| No conecta WiFi     | SSID incorrecto, red 5 GHz | Revisar `secrets.h`              |
| SMS no recibidos    | CNMI mal configurado       | Confirmar `AT+CNMI=2,1,0,0,0`    |
| API no recibe datos | JSON mal formado           | Revisar sanitización y `API_URL` |
| SIM se satura       | SMS corruptos sin limpiar  | Usar consola con `AT+CMGD`       |



