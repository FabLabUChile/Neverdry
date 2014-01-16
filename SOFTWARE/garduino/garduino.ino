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
char ledpins[]={9,10,11};
char pin_boton=2;
unsigned char color[]={255,255,255}; //NO TOCAR DIRECTAMENTE
char awake_cont=0;
char state=0; //0: ok. 1: seco. 2: muy mojado.

/************ CONSTANTES (definidas como macros) ************/
#define TOLERANCIA 200 //hay que calibrarla. NO tiene sentido que sea mayor a 512
#define AWAKE_FREQ 5

/************ VARIABLES ************/

int sensor_read;
long t0, t1;
int HUMEDAD_IDEAL = 100; //Calibrable por el usuario
unsigned char indice_color=0; //de 0 a 6
unsigned char colores[7][3]={{255,0,0},{0,255,0},{0,0,255},{255,255,0},{255,0,255},{0,255,255},{255,255,255}};

/************ MÉTODOS ************/
/* alertaRiego puede recibir tres valores:
      ·0 indica todo ok.
      ·1 indica falta agua.
      ·2 indica exceso de agua */
void alertaRiego(char b){
  //switch - case sería más elegante
  state=b;
  if (b==1) {
    Serial.println("poca agua!!!");
    setColor(255,0,0);
  }
  if (b==2) {
    Serial.println("mucha agua!!");
    setColor(255,0,255);
  }
  if (b==0) setColor(0,0,0);
}

void rutinaLectura(){
  //Serial.print("lectura sensor: ");
  sensor_read=analogRead(pin_sensor);
  Serial.println(sensor_read);
  if (sensor_read<HUMEDAD_IDEAL-TOLERANCIA/2.0)
    alertaRiego(2);
  else if (sensor_read>HUMEDAD_IDEAL+TOLERANCIA/2.0)
    alertaRiego(1);
  else alertaRiego(0);
}

/* pone al arduino a dormir */
void dormir(){
  attachInterrupt(0, buttonHandler, LOW);
  /* setear tipo de sleep */
  set_sleep_mode(SLEEP_MODE_PWR_SAVE); //el de más ahorro de energía
  /* habilitar sleep */
  sleep_enable();
  /* ponerse a dormir */
  
  //debug
  delay(50);
  
  sleep_mode();
  /* Desde acá despierta. Deshabilitar sleep */
  sleep_disable();
  /* Parpadear led que muestra corecto funcionamiento */
  awake_cont=(awake_cont + 1)%AWAKE_FREQ;
  if ((awake_cont==0)&&(state==0)){
    setColor(0,255,0);
    delay(50);
    setColor(0,0,0);
  }
}

/* Inicializa el WDT */
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
/*********** INTERRUPCIONES ***********/
/* handler de la interrupcion generada por el WatchDog Timer */
ISR(WDT_vect)
{
}
/* handler de la interrupción que envía el botón */
void buttonHandler(){
  detachInterrupt(0);
  
  //debug:
  Serial.println("boton");
  delay(50);
  
  //hay que hacer tiempo hasta que el sensor se estabilice un poco.
  char i;
  for (i=0;i<5;i++){
    fadeOnce();
  }
  awake_cont=0;
  // leer el sensor y fijar valor óptimo de humedad seteado por el usuario. 
  HUMEDAD_IDEAL=analogRead(pin_sensor)+TOLERANCIA/2.0-15; //totalmente experimental y puede ser modificado según se crea necesario.
}

/*******************/
/* Manejo de los leds. Posiblemente implementar con clases en C++ para transparencia del código. */

void setColor(unsigned char r, unsigned char g, unsigned char b){
  color[0]=map(r,0,255,255,0);
  color[1]=map(g,0,255,255,0);
  color[2]=map(b,0,255,255,0);
  analogWrite(ledpins[0],color[0]);
  analogWrite(ledpins[1],color[1]);
  analogWrite(ledpins[2],color[2]);
}
/* Efecto de FadeIn - FadeOut del led, con el color escogido.
Esta función gana tiempo para que el sensor se estabilice un poco.*/
void fadeOnce(unsigned char r, unsigned char g, unsigned char b){
  //velocidad de fade fijada internamente por la función.
  unsigned char f_color[]={r,g,b};
  rutinaFade(f_color);
}
void fadeOnce(){//versión sin argumentos
  rutinaFade(colores[indice_color]);
  indice_color=(indice_color+1)%7;
}
void rutinaFade(unsigned char f_color[]){ //NO usar externamente
  unsigned char i;
  unsigned char resolucion=100;
  int demora=4000; //microsegundos
  for (i=0;i<resolucion;i++){
    setColor((unsigned char)(f_color[0]*(i*1.0/resolucion)),(unsigned char)(f_color[1]*(i*1.0/resolucion)),(unsigned char)(f_color[2]*(i*1.0/resolucion)));
    delayMicroseconds(demora);
  }
  for (i=resolucion;i>0;i--){
    setColor((unsigned char)(f_color[0]*(i*1.0/resolucion)),(unsigned char)(f_color[1]*(i*1.0/resolucion)),(unsigned char)(f_color[2]*(i*1.0/resolucion)));
    delayMicroseconds(demora);
  }
}
/************ MAIN ************/

void setup() {
  Serial.begin(9600);
  Serial.print("Inicializando...      ");
  /* inicializar pines*/
  pinMode(pin_sensor, INPUT);
  char i;
  for (i=0;i<=2;i++){
    pinMode(ledpins[i],OUTPUT);
  }
  pinMode(pin_boton, INPUT);
  /*** Inicializar WatchDog Timer ***/
  initWDT();
  
  //setear valor predeterminado de humedad 
  Serial.println("Listo\n");
}

void loop() {
  //delay(100);
  rutinaLectura();
  dormir();
  //fadeOnce();
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
        Debe poder notificar: Sobrerriego de la planta, falta agua en la planta, en proceso de calibración, funcionamiento correcto.
              Falta agua: Led rojo. Sin animación, pues está durmiendo.
              Sobrerriego: Led amarillo.
              En proceso de calibración: led azul animado.
              Funcionamiento correcto: parpadeo de led verde cada vez que se despierta a hacer mediciones.
              
    -Botón de calibración: Ojo que lo más probable es que se presione cuando el arduino está dormido. Hay que poder escuchar esta interrupción y despertar.
    
    
   por corregir: 
       -cambiar if's por switch-case.
       -implementar boton y led con clase de C++ para no ensuciar código con métodos propios de estos artefactos.
       - el efecto de Fade de los leds ocurre el doble de veces de las que debería
       -Al estar muy húmedo, se alterna el color entre rojo y amarillo, y no se queda fijo en amarillo como se desea.
       -calibración del usuario debe tomar en cuenta que la humedad que establezca el usuario al regar, es CASI la cota
       que define el umbral entre que está bien regada y que está excesivamente regada. Definir humedad ideal tomando esto
       en cuenta.
       
Versión 1.3

Corregidos algunos errores anteriores
*/
