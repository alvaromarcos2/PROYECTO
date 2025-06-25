# Informe Final de Proyecto: Sistema de Monitorización de Servidores con ESP32-S3

**Autor:** Álvaro Marcos Rodríguez
**Fecha:** 25 de Junio de 2025
**Plataforma de Desarrollo:** PlatformIO

---

## 1. Resumen

Este documento detalla el diseño, desarrollo e implementación de un sistema diseñado para la monitorización en tiempo real de las condiciones ambientales de una sala de servidores. El sistema integra sensores de temperatura, humedad y gases para la detección proactiva de riesgos como el sobrecalentamiento y los incendios.

El proyecto culmina en un dashboard web seguro y dinámico que permite a los administradores supervisar el estado del entorno desde cualquier dispositivo con un navegador, recibiendo alertas visuales inmediatas ante cualquier anomalía. La arquitectura del sistema se centra en la eficiencia, la responsividad y la robustez, utilizando tecnologías web modernas sobre un hardware con recursos limitados.

---

## 2. Objetivos del Proyecto

Los objetivos principales que guiaron el desarrollo de este sistema fueron:

*   **Monitorización Continua:** Capturar de forma constante y fiable datos de temperatura, humedad y concentración de gases.
*   **Detección de Alertas:** Definir y detectar condiciones anómalas (temperatura superior a 32°C, presencia de humo/gas) para activar alertas de forma automática.
*   **Interfaz de Usuario Centralizada:** Desarrollar un dashboard web accesible que presente los datos de forma clara e intuitiva.
*   **Visualización en Tiempo Real:** Asegurar que la interfaz se actualice dinámicamente sin necesidad de recargas manuales, proporcionando una experiencia de usuario fluida (real-time).
*   **Acceso Seguro:** Implementar un sistema de autenticación para proteger el acceso al dashboard y a la configuración del sistema.
*   **Robustez y Eficiencia:** Diseñar un firmware no bloqueante y eficiente en el uso de memoria, capaz de gestionar múltiples tareas (lectura de sensores y servicio web) de forma concurrente.

---

## 3. Arquitectura del Sistema

El sistema se estructura en tres capas interconectadas, conformando una solución completa de IoT (Internet de las Cosas).

### 3.1. Capa de Hardware
*   **CPU:** Microcontrolador **ESP32-S3 Dual-Core**.
*   **Sensor de Temperatura y Humedad:** **DHT11**, conectado a un pin digital, utiliza un protocolo propietario de 1-hilo para la transmisión de datos.
*   **Sensor de Gases y Humo:** **MQ-2**, conectado a un pin del Convertidor Analógico a Digital (ADC), proporciona una señal de voltaje proporcional a la concentración de gases combustibles, actuando como detector de incendios.

### 3.2. Capa de Backend (Firmware del ESP32)

*   **Conectividad:** Se establece una conexión a una red WiFi para permitir el acceso a través de la red local.
*   **Servidor Web Asíncrono:** Se implementa un servidor web en el puerto 80 que atiende peticiones HTTP. El código es **no bloqueante**, utilizando la función `millis()` para gestionar tareas concurrentes sin detener el procesador.
*   **API RESTful:** El servidor expone una API simple para la comunicación con el frontend:
    *   `GET /`: Sirve la página de login o el dashboard principal.
    *   `POST /login`: Procesa las credenciales del usuario.
    *   `GET /logout`: Cierra la sesión del usuario.
    *   `GET /data`: Devuelve el estado actual de los sensores en formato **JSON**.
    *   `GET /test` y `GET /stop_alert`: Endpoints para controlar las alertas manualmente.
*   **Gestión de Sesiones:** Se ha implementado un sistema de autenticación basado en **cookies HTTP** para securizar el acceso. Tras un login exitoso, el servidor genera un token de sesión único.

### 3.3. Capa de Frontend (Dashboard Web)

*   **Actualización en Tiempo Real con AJAX:** El JavaScript utiliza la técnica **AJAX** (a través de la función `fetch()`) para solicitar periódicamente los datos JSON a la API del ESP32. Esto permite actualizar los valores en pantalla y el log histórico sin recargar la página.
*   **Visualización de Datos:** Muestra la temperatura y humedad actuales, y un log histórico con marcas de tiempo para cada lectura.
*   **Alertas Visuales:** Ante una condición de alerta, la interfaz responde de forma inmediata cambiando colores, mostrando mensajes y activando una animación de parpadeo para captar la atención del administrador.

---

## 4. Funcionamiento Detallado y Decisiones de Diseño

### 4.1. Flujo de Datos
El flujo de información es cíclico y robusto:
1.  Los sensores capturan los datos del entorno.
2.  El `loop()` principal del ESP32 los lee cada 2 segundos.
3.  Los datos se almacenan en variables globales y se evalúan las condiciones de alerta.
4.  El cliente web (navegador) solicita los datos cada 2 segundos a la API `/data`.
5.  El servidor responde con un objeto JSON.
6.  El JavaScript del cliente recibe el JSON y actualiza el DOM (la página) para reflejar los nuevos datos y el estado de las alertas.

### 4.2. Dual-Core y Concurrencia
Aunque el ESP32-S3 posee dos núcleos, el framework de Arduino por defecto reserva el **Núcleo 0** para la gestión de la pila de red (WiFi), mientras que el código de la aplicación se ejecuta en el **Núcleo 1**. Este diseño, aunque no explota el paralelismo de forma manual, garantiza una alta estabilidad en la conectividad. Una posible mejora futura sería refactorizar el código para usar **FreeRTOS** y asignar la lectura de sensores al Núcleo 0 y el servidor web al Núcleo 1, logrando un paralelismo real.

### 4.3. Eficiencia de Memoria
Para optimizar el uso de la limitada memoria RAM del microcontrolador, todo el contenido estático de la web (HTML, CSS, y JavaScript) se almacena en la memoria Flash del programa utilizando la directiva `PROGMEM`. Esta es una decisión de diseño crítica que permite al sistema funcionar de manera estable.

---

## 5. Conclusiones y Futuras Mejoras

El proyecto ha cumplido con éxito todos sus objetivos. Se ha demostrado la viabilidad de implementar una solución completa de IT, con backend y frontend, en un único microcontrolador.

Las decisiones de diseño, como el uso de una API JSON, la técnica AJAX y la optimización de memoria con `PROGMEM`, han sido interesantes de aprender que han conllevado que el programa se ejecutara de manera correcta.

**Posibles Mejoras Futuras:**
*   **Notificaciones Push:** Integrar un servicio externo (como `ntfy.sh` a través de HTTP) para enviar notificaciones push a dispositivos móviles, permitiendo una respuesta a alertas incluso sin tener el dashboard abierto.
*   **Configuración Remota:** Añadir una sección en el dashboard para permitir al administrador modificar los umbrales de alerta de forma remota.
