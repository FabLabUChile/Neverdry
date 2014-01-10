/*
  Garden Bot
  
  Setup:
          -Divisor de voltaje, conectado a 5V, 1000 ohm, A0 sensor, GND.
          -Led rojo a pin 12. No olvidar controlar corriente con resistencia.
          -Led verde a pin 11.
 */

/************ INCLUDES ************/
//manejo de ahorro de energía
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

/************ GPIOs ************/
char pin_sensor=0;
char led_rojo=12;
char led_verde=11;
//para debug:
char led_awake=13;

/************ CONSTANTES (definidas como macros) ************/
#define DELAY 500 //en ms

/************ VARIABLES ************/

int sensor_read;
long t0, t1;
int HUMEDAD_MIN = 600; //Calibrable

/************ MÉTODOS ************/
void alertaRiego(char b){
  digitalWrite(led_rojo, b);
}

void rutinaLectura(){
  //Serial.print("lectura sensor: ");
  sensor_read=analogRead(pin_sensor);
  Serial.println(sensor_read);
  if (sensor_read>HUMEDAD_MIN)
    alertaRiego(1);
  else
    alertaRiego(0);
}

/* pone al arduino a dormir */
void dormir(){
  //debug:
  //Serial.println("durmiendo...");
  digitalWrite(led_awake,LOW);
  //delay(50);
  
  /* setear tipo de sleep */
  set_sleep_mode(SLEEP_MODE_PWR_SAVE); //el de más ahorro de energía
  /* habilitar sleep */
  sleep_enable();
  /* ponerse a dormir */
  sleep_mode();
  /* Desde acá despierta. Deshabilitar sleep */
  sleep_disable();
  
  // debug:
  //Serial.println("despierto!");
  digitalWrite(led_awake,HIGH);
}
/* handler del WatchDog Timer */
ISR(WDT_vect)
{
}

void initWDT(){
  /* limpiar el flag de reset */
  MCUSR &= ~(1<<WDRF);
  /* Para poder cambiar WDE o el prescaler, es necesario poner en 1 WDCE. Esto habilita las configuraciones del watchdog timer */
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* setear prescaler */
  WDTCSR = 1<<WDP2 | 1<<WDP1; /* interrupt cada 1s */
  /* Habilitar interrupt del watchdog. SIN RESET */
  WDTCSR |= (1 << WDIE);      
}

/************ MAIN ************/

void setup() {
  Serial.begin(9600);
  /* inicializar pines*/
  pinMode(pin_sensor, INPUT);
  pinMode(led_rojo, OUTPUT);
  pinMode(led_verde, OUTPUT); 
  Serial.print("Inicializando...     ");
  /*** Inicializar WatchDog Timer ***/
  //initWDT();
  
  // para debug:
  pinMode(led_awake, OUTPUT);
  digitalWrite(led_awake,HIGH);
  
  //setear valor predeterminado de humedad 
  Serial.println("Listo\n");
}

void loop() {
  delay(100);
  rutinaLectura();
  //dormir();
}

/*
Idea general: Sensor de humedad puesto en la tierra. Bajo cierto umbral de humedad, alertar para que se riegue.

Sensor: Dos clavos entre los cuales pasa corriente variable, dependiendo de la humedad en el medio. 
        Arduino lee 1024 cuando los clavos están aislados y 0 cuando están conectados.
        
        Los clavos resistirán el paso de la corriente entre ellos por los siguientes factores:
            -tipo de tierra
            -cantidad de humedad
            -resistencia del material de los clavos
            -óxido generado con el tiempo (incontrolable por software)
            -separación entre los clavos
            -profundidad del sensor en la tierra (mientras menos resistivo es el material de los clavos, menos importa este factor)
            
        
            
Sleep:  Cuando el arduino realizó una lectura y tomó una decisión sobre si alertar o no de una falta de agua, es necesario 
        ponerlo a descansar por un tiempo para ahorrar energía.
        
        -Los pins flotantes consumen pequeñas cantidades de energía. Resistencias pullup o pulldown solucionan este problema.
            Ya que arduino trae pullups internos, es necesario activarlas en todos los pines no usados. Para activar pin, setear como INPUT
            y escribir HIGH.
        -Sleep se setea de la siguiente forma:
                  - Setear el tipo de sleep que se usará
                  - Habilitar el bit de sleep
                  - poner en modo sleep. Acá se duerme realmente.
                  - Al despertar, deshabilitar el bit de sleep. Sólo puede despertar por un interrupt que se debe definir previamente.
                  
        Formas de despertar al arduino: 
                  - Interrupt externo
                  - UART, interface serial
                  - Timer interno o Watchdog
        
        Para este proyecto conviene usar el Watchdog Timer.
        El Watchdog Timer es un timer con oscilador propio. Genera un interrupt luego de una cantidad de ticks. El timer corre a 128 KHz.
        Puede configurarse para que en overflow: haga Reset, mande un interrupt, o haga ambos al mismo tiempo. Al igual que los otros timers
        el watchdog timer necesita de un prescaler por setear para saber cada cuantos ticks aumentar su contador. El prescaler va de 1 a 9.
        Los bits que setean el prescaler son WDPx con x de 3 a 0. Para despertar luego de 1s, setear prescaler a 0110.
        
        
Calibración:
        Discusión: Si se busca lograr un real sensor de humedad, se deben tener en cuenta unas cuantas variables principales que podrían afectar la lectura del sensor de manera no
          despreciable. Estos son:
              1-Tipo de tierra
              2-profundidad del sensor en la tierra
              3-temperatura en la tierra
              4-efecto transiente del medidor
              5-Cuánta humedad necesita cada planta
              
        4-  El efecto transiente es inevitable y tiene que ver con la velocidad a la que el agua moja la tierra. Una manera de minorizar su impacto puede ser
        anticipando el valor que se quiere alcanzar; esto es, dar alerta de humedad buena cuando aún no se tiene la humedad óptima. Un problema con esto
        es que el agua, de manera natural, abandona la tierra lentamente, por lo tanto este efecto estaría ausente (las mediciones serían relativamente estables)
        por lo que al intentar anticiparse al valor, leería algo incorrecto.
        Otra posibilidad es asumir una diferencia en los estados del arduino entre que se está regando y no se está regando. Cuando se está regando, tomar en cuenta
        el efecto transiente, y cuando no, ignorarlo, y hasta se pueden prolongar los períodos de sleep.
        
        1- El tipo de tierra influye en su conductividad. Una tierra muy conductiva podría hacer creer que la planta tiene la humedad suficiente, cuando en realidad
        está seca y es la tierra la que conduce mucha corriente.
        Se podría arreglar este problema pidiendo al usuario que calibre el medidor, con una muestra de la tierra que quiere usar. Esto implicaría medir la conductividad
        de la tierra cuando está en su valor óptimo y quizás cuando esta saturada y seca, para establecer un buen rango de valores de humedad.
        
        2- Nada más que hacer que decir al usuario que ponga el medidor lo más profundo que pueda
        
        3- Si se deja al sol o a la sombra la planta, la resistencia de los clavos varía. Una forma de lidiar con esto es agregar algún thermistor o algo por el estilo
        que contrarreste este efecto y calibre los valores ideales de humedad según la temperatura que lo rodea.
        
        5- La solución de (1) debería solucionar esto.
        
        
Propuesta final:
       El usuario riega la planta hasta un nivel óptimo de humedad. Pone el sensor en la tierra lo más enterrado posible.
       Presiona un botón, el sensor indica que está esperando que se estabilice el valor de humedad para recordarlo, quizás parpadeando 
       un led en el proceso, y apagándolo cuando está listo, o prendiendo un led verde que indica un estado OK (Quizás un led RGB es 
       más elegante para estos propósitos). Finalmente el sensor está activo y atento a una baja o subida excesivas de humedad. Quizás sea buen feedback 
       hacer lo que muchos artefactos electrónicos hacen para mostrar que están funcionando: parpadear un led cada X segundos (5, 10 segundos)
       así como las alarmas de casa o los aromatizadores automáticos.






Versión 1.2

Añadiduras:
    -Led RGB indicador
    -Botón de calibración
*/
