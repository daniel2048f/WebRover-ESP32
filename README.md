# Carrito RC con ESP32

Este proyecto consiste en un carrito controlado de forma inalámbrica a través de un punto de acceso (Access Point) creado por un módulo ESP32. El sistema permite regular la dirección a través de un servomotor y controlar la velocidad y el sentido de movimiento (adelante o atrás) de dos motoreductores mediante un puente H.

---

## Tabla de Contenidos
1. [Descripción General](#descripción-general)
2. [Componentes Principales](#componentes-principales)
3. [Esquema de Conexiones](#esquema-de-conexiones)
4. [Funcionamiento del Código](#funcionamiento-del-código)
   - [Modo Access Point](#modo-access-point)
   - [Configuración del Servidor Web](#configuración-del-servidor-web)
   - [Control del Servomotor](#control-del-servomotor)
   - [Control del Motor Principal](#control-del-motor-principal)
   - [Interfaz Web](#interfaz-web)
5. [Uso del Carrito](#uso-del-carrito)
6. [Posible Captura de Pantalla](#posible-captura-de-pantalla)
7. [Consideraciones Adicionales](#consideraciones-adicionales)
8. [Créditos y Licencia](#créditos-y-licencia)

---

## Descripción General

Este código permite controlar un vehículo tipo “carrito RC” mediante una interfaz web alojada directamente en el ESP32. Al encender el circuito, el ESP32 crea una red WiFi a la que se conecta el dispositivo de control (smartphone, tablet o computadora). Desde el navegador de ese dispositivo se ingresa a la página de control, la cual permite:

- Ajustar la **velocidad** (PWM) de los motores.
- Cambiar la **dirección** a través de un servomotor (ángulo de giro).
- Seleccionar el **sentido de giro** del motor (avanzar, reversa) o **detener** el movimiento.

De esta manera, no se requiere un router externo ni una infraestructura adicional para poder maniobrar el carrito, ya que el ESP32 actúa como punto de acceso y servidor web de forma autónoma.

---

## Componentes Principales

1. **ESP32**: Microcontrolador WiFi que crea el punto de acceso y el servidor web, y también genera las señales PWM para los motores y el servomotor.  
2. **Puente H** (por ejemplo L298N, L293D u otro similar): Permite controlar la dirección de giro y la velocidad de los motores.  
3. **Dos motoreductores**: Motores de corriente continua con reducción, usados para el movimiento de avance y retroceso.  
4. **Servomotor** (en este caso, conectado al pin 13 del ESP32): Se encarga de la dirección del carrito, variando su ángulo entre 20° y 160°.  
5. **Baterías Li-Ion**: Dos baterías de iones de litio (generalmente 3.7 V cada una) que alimentan tanto el ESP32 como los motores. Se recomienda usar un regulador apropiado para el ESP32 y el servomotor.  
6. **Cableado** y componentes electrónicos necesarios para las conexiones, incluyendo jumpers y un circuito para la alimentación.

---

## Esquema de Conexiones

- **Servomotor**:
  - Señal de control: `servoPin` (pin 13 en el ESP32).
  - Alimentación: 5V (o la tensión requerida por el servo) y GND común con el ESP32.

- **Puente H**:
  - Entradas de control:
    - `In2`: Pin 25 del ESP32.
    - `In3`: Pin 33 del ESP32.
    - `ENB`: Pin 32 del ESP32 (PWM para la velocidad).
  - Salidas a los dos motores (conectadas según el puente H en uso).
  - Alimentación de los motores según sus especificaciones (usualmente por encima de 5V, dependiendo de la potencia de cada motoreductor).

- **Alimentación**:
  - Se recomienda tener una fuente dedicada para los motores que sea capaz de suministrar la corriente adecuada.
  - El ESP32 funciona típicamente a 3.3V, pero su pin VIN o 5V (dependiendo de la placa) se puede usar si se alimenta a través de un regulador apropiado.

> **Nota**: Asegúrate de que todas las tierras (GND) estén conectadas en común: la del ESP32, la del puente H y la del servomotor.

---

## Funcionamiento del Código

A continuación, se describe la lógica principal incluida en el archivo fuente:

### Modo Access Point

```cpp
WiFi.softAPConfig(local_IP, gateway, subnet);
WiFi.softAP(ssid, password);
```

1. Se configuran la dirección IP y la máscara de red del ESP32 en modo **punto de acceso**.
2. Se crea la red con nombre (`COCHERC`) y contraseña (`87654321`).

Una vez inicializado, el ESP32 queda a la espera de conexiones WiFi de cualquier dispositivo que quiera controlar el carrito.

### Configuración del Servidor Web

```cpp
WebServer server(80);
```

- Se crea un servidor en el puerto 80 (puerto HTTP por defecto).
- Se definen **rutas** (endpoints) para manejar diferentes peticiones:
  - `"/"`: Retorna la página principal (HTML, CSS y JavaScript embebido) para el **control gráfico**.
  - `"/command"`: Recibe comandos (`avanzar`, `reversa`, `parar`) para controlar el sentido de giro de los motores.
  - `"/velocidad"`: Ajusta la velocidad del motor mediante PWM (0-255, aunque en el ejemplo se restringe a 85-230).
  - `"/direccion"`: Ajusta la posición (ángulo) del servomotor.

### Control del Servomotor

```cpp
ESP32PWM::allocateTimer(3);
servoDireccion.setPeriodHertz(50);
servoDireccion.attach(servoPin, 500, 2400);
```

- Se configura un timer para generar la señal PWM a 50 Hz, típica de los servomotores.
- El método `servoDireccion.write(anguloServo);` se usa para enviar al servo la posición deseada (en grados).  
- Por defecto, el ángulo inicial está en 90°. A través de la interfaz web, se restringe entre 20° y 160° (ajustable).

### Control del Motor Principal

- El pin `ENB` se configura para generar la señal PWM.  
- Los pines `In2` e `In3` controlan la dirección de giro:
  - `In2 = LOW` y `In3 = HIGH` → Avanzar
  - `In2 = HIGH` y `In3 = LOW` → Reversa
  - Ambos LOW → Parar
  
- El valor PWM (en el ejemplo se llama `velocidad`) se puede modificar en tiempo real mediante el endpoint `/velocidad`.

### Interfaz Web

La interfaz se describe en la variable `html` con HTML y CSS integrados. Destacan:

- **Slider de Velocidad**: Ajusta el rango de 85 a 230 (valores de PWM).
- **Slider de Dirección**: Ajusta el ángulo de 20° a 160°.
- **Botones**: 
  - **Avanzar** → Llama a `fetch("/command?cmd=avanzar")`
  - **Reversa** → Llama a `fetch("/command?cmd=reversa")`
  - **Parar**   → Llama a `fetch("/command?cmd=parar")`

El uso de `fetch()` permite enviar peticiones HTTP sin recargar la página, haciendo el control más fluido.

---

## Uso del Carrito

1. **Encender el ESP32 y el resto del circuito**.  
2. Con un dispositivo (celular, laptop, etc.) busca la red WiFi llamada `"COCHERC"` y conéctate con la contraseña `"87654321"`.  
3. En el navegador, escribe la dirección `192.168.4.4` (o la que hayas configurado en `local_IP`).
4. Verás la interfaz web con los sliders y botones.
5. Ajusta la **velocidad** y la **dirección** con los sliders.
6. Utiliza los botones **Avanzar**, **Parar** o **Reversa** para controlar el movimiento.

---

## Captura de Pantalla de la Interfaz Web


![Imagen de WhatsApp 2025-03-30 a las 22 18 59_ce3c7df9](https://github.com/user-attachments/assets/71d51f00-0720-4f02-8b0a-30d837deadfe)


---

## Consideraciones Adicionales

- **Alimentación**: Verifica que tu fuente o baterías suministren la corriente suficiente para el servo y los motores sin que el voltaje se caiga demasiado.  
- **Calibración de Ángulos**: Dependiendo del modelo de servomotor y de la mecánica de dirección, puede ser necesario ajustar los rangos de `20` a `160` (o los pulsos de `500, 2400`) para que no fuerce el servo.  
- **Seguridad**: Cualquier dispositivo que se conecte a la red `COCHERC` podrá controlar el carrito. Considera cambiar SSID y contraseña si lo usas en entornos donde no quieres que se conecten otras personas.  
- **Bibliotecas Requeridas**:
  - `WiFi.h` (para ESP32)
  - `WebServer.h` (servidor web para ESP32)
  - `ESP32Servo.h` (control de servo con timers en ESP32)

---

## Créditos y Licencia

Este código fue creado por Daniel Alejandro Cangrejo López y Camila Andrea Cangrejo López, su esctructura está basada en ejemplos y bibliotecas oficiales de [Arduino](https://www.arduino.cc/) y [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32). El estilo de la página web está escrito en HTML5, CSS y JavaScript puro.

Siente la libertad de modificarlo y mejorarlo según tus necesidades. Se sugiere mantener la atribución original si lo distribuyes o publicas en un repositorio público.

**¡Disfruta construyendo y mejorando tu Carrito RC con ESP32!**
