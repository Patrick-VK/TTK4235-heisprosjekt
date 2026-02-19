#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "driver/elevio.h"

//må sannsynligvis ha tilgang på lysinputs, bruke de til å toggle lista
//none bestillinger(){}:
//bruker en liste for å sjekke hvor den stopper, sjekker dette opp mot motorretning og nåværende posisjon (skal ikke snu hvis det finnes fler bestillinger i retningen den beveger seg i) 
//hvis heisen stopper i en etasje skal etasjen fjærnes/settes til false i lista
//hvis liste tom => motorspeed = 0
//kommer til å kunne fjerne true fra lys når bestillinger utføres

//none lys(etasjer?){}:
//kjøres kontinuerlig
//må kunne vite om heisen stopper i en etasje, for å slukke lyset til knappen utenfor (when stopped light toggle off).

//none door_open(etg){}: kalles fra bestillinger, muligens obst eller stopp
//åpne (lampe og var), timer 3 sek, lukke
//sjekke at heis står stille, sjekke at heis er i en etasje


//bool? stop_btn(){}: kalles når stoppknappen trykkes, (veldig) mulig denne må kalles i door open
//her må man finne en lur løsning på hvordan man sjekker at heisen er i en etasje, og ikke mellom, sjekke sensorverdier
//skal være gyldig så lenge stoppknappen er true
//skal få lyset til å lyse, må sjekkes opp mot door open etter eller annet sted
//må sette motorspeed til 0 
//sette alle orders til false
//skal ikke være mulig å gjøre bestillinger, mulig en konsekvens av å sette orders til false

//bool? obs(){}: enklere stopp, mulig det er overflødig med egen funksjon, er bare en iftest om bryer og dør er true
//etter bryter blir false, kalle timern fra dør, gå videre
//MÅ ligge under dørene
int main(){
    elevio_init();
    
    printf("=== Example Program ===\n");
    printf("Press the stop button on the elevator panel to exit\n");

    elevio_motorDirection(DIRN_UP);
    //int oppstart_bool = false

    while(1){
        //if (!oppstart_bool)
        //oppstartskode, typ send ned til 1. etg
        //sett retning, sjekk etg, ikke bruk dører eller obs (tror jeg), 
        //husk at heisknappene må funke, lage btnlights()?

        //else{}, resten av koden
        //mulig btnlights()?
        //legge inn stoppknapp
        int floor = elevio_floorSensor(); //sjekke verdier her  0-3 eller 1-4?
        if (floor >= 0){
            elevio_floorIndicator(floor);
        }

        if(floor == 0){
            elevio_motorDirection(DIRN_UP);
        }

        if(floor == N_FLOORS-1){
            elevio_motorDirection(DIRN_DOWN);
        }

        //sjekker at knappene lyser når de blir trykket på, mulig en liknende variant for å håndtere bestillinger
        for(int f = 0; f < N_FLOORS; f++){
            for(int b = 0; b < N_BUTTONS; b++){
                int btnPressed = elevio_callButton(f, b);
                elevio_buttonLamp(f, b, btnPressed);
            }
        }

        if(elevio_obstruction()){
            elevio_stopLamp(1);
        } else {
            elevio_stopLamp(0);
        }
        
        if(elevio_stopButton()){
            elevio_motorDirection(DIRN_STOP);
            break;
        }
        
        nanosleep(&(struct timespec){0, 20*1000*1000}, NULL);
    }

    return 0;
}
