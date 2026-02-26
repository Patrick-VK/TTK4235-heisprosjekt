#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "driver/elevio.h"


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
//

int orders[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; //første i lista er om heisen skal stoppe, andre er om retningen har noe å si - knapper på panel utenfor vs inni
//-1 er ned, 0 er likegyldig, 1 er opp
int orderChanged = 1;
int heading = 1; //samme som orders, settes til 1 fordi heisen skal være i 1. etg ved oppstart

//må sannsynligvis ha tilgang på lysinputs, bruke de til å toggle lista
//none bestillinger(){}:
//bruker en liste for å sjekke hvor den stopper, sjekker dette opp mot motorretning og nåværende posisjon (skal ikke snu hvis det finnes fler bestillinger i retningen den beveger seg i) 
//hvis heisen stopper i en etasje skal etasjen fjærnes/settes til false i lista
//hvis liste tom => motorspeed = 0
//kommer til å kunne fjerne true fra lys når bestillinger utføres


int ordersEmpty(){
    for(int i = 0; i < N_FLOORS; i++){
        if(orders[i][0]){
            return 0;
        }
    } return 1;
}

void clearOrderRow(int row){
    orders[row][0] = 0;
    orders[row][1] = 0;
    orderChanged = 1;
}

void settBestillinger(){ //tilsvarer getPressedButtons() i UML, bytte om til funksjon for å hente alle pressed buttons? 
    //sjekker panelknapper, må muligens skrives om
    for (int i = 0; i < N_FLOORS; i++){ 
        for (int b = 0; b < N_BUTTONS; b++){
            int btnPressed = elevio_callButton(i, b);
            if (btnPressed & b == 2){ //sjekker knappene inni heisen
                orders[i][0] = 1;
                orders[i][1] = 0;
                orderChanged = 1;
            } else if (btnPressed & b == 0){ //sjekker knappene på vei opp
                orders[i][0] = 1;
                orders[i][1] = 1;
                orderChanged = 1;
            } else if (btnPressed & b == 1){ //sjekker knappene på vei ned
                orders[i][0] = 1;
                orders[i][1] = -1;
                orderChanged = 1;
            }
        }         
    }
}

void settHeading(int etg){
    int ordersAbove = 0;
    int ordersBelow = 0;
    if (etg == 3){
        heading = -1;
    } else if (etg == 0){
        heading = 1;
    } else {
        //teller ordre over
        for (int i = etg; i < N_FLOORS; i++){
            if ((orders[i][0] == 1)){
                ordersAbove += 1;
            }
        }
        //teller ordre under
        for (int i = etg; i >= 0; i--){
            if ((orders[i][0] == 1)){
                ordersBelow += 1;
            }
        }
        if ((heading == 1) & (ordersAbove == 0)){
            heading = 0;
        } else if ((heading == -1) & (ordersBelow == 0)){
            heading = 0;
        }
        if ((heading == 0) & (ordersAbove > 0)){
            heading = 1;
        } else if ((heading == 0) & (ordersBelow > 0)){
            heading = -1;
        }

    }
}

void bevegelse(){ //kutte ut?
    if (ordersEmpty()){
        elevio_motorDirection(DIRN_STOP);
    } else{
        elevio_motorDirection(heading);
    }
}

void stopInFloor(int etg){
    if ((orders[etg][0] == 1) & ((orders[etg][1] == 0) || (orders[etg][1] == heading))){
        elevio_doorOpenLamp(1);
        time_t start = time(NULL);
        clearOrderRow(etg);
        elevio_motorDirection(DIRN_STOP);
        while (time(NULL) - start < 3);
        elevio_doorOpenLamp(0);
    }
}

void floorLights(){ //Bytte til setPanelLights() ?
    if (elevio_floorSensor() >= 0){
            elevio_floorIndicator(elevio_floorSensor());
    }
}

void panelLights(){ // mulig denne kan endres og kalles ved bestilling
    if (orderChanged){ 
        for (int i = 0; i < N_FLOORS; i++){
            for (int b = 0; b < N_BUTTONS; b++){
                if (orders[i][0]){
                    if ((orders[i][1] == 0) & (b == 2)){
                        elevio_buttonLamp(i, b, 1);
                    } else if ((orders[i][1] == -1) & (b == 1)){
                        elevio_buttonLamp(i, b, 1);
                    } else if ((orders[i][1] == 1) & (b == 0)){
                        elevio_buttonLamp(i, b, 1);
                    }   
                } else {
                    elevio_buttonLamp(i, b, 0);
                }
            }
        }
        orderChanged = 0;
    }
}


int main(){
    elevio_init();
    //printf("=== Example Program ===\n");
    //(printf("Press the stop button on the elevator panel to exit\n");
    elevio_motorDirection(DIRN_DOWN);
    int oppstart_bool = 0;

    while(1){
        if (!oppstart_bool){
            panelLights(); //mulig feilkilde
            floorLights();
            if(elevio_floorSensor() == 0){
                oppstart_bool = 1;
                elevio_motorDirection(DIRN_STOP);
            }
        }
        else {
            //mulig btnlirDirection(DIRN_UP);
            //if(floor == 0ghts()?
            int floor = elevio_floorSensor(); //sjekke verdier her, 0-3 eller 1-4?
    
            settBestillinger();
            bevegelse();
            if (floor > -1){
                settHeading(floor);
                stopInFloor(floor);
            }
            floorLights(); //etasjelys
            panelLights(); //panellys
            
            
            //muig dette gjøres om til funksjon
            /*if (ordersEmpty()){
                elevio_motorDirection(DIRN_STOP);
            }
            else {
                elevio_motorDirection(DIRN_UP);
            }*/
            //if(floor == 0){
            //    elevio_motorDirection(DIRN_UP);
            //}

            //if(floor == N_FLOORS-1){
            //    elevio_motorDirection(DIRN_DOWN);
            //}


            //sjekker at knappene lyser når de blir trykket på, mulig en liknende variant for å håndtere bestillinger
            //for(int f = 0; f < N_FLOORS; f++){
            //    for(int b = 0; b < N_BUTTONS; b++){
            //        int btnPressed = elevio_callButton(f, b);
            //        elevio_buttonLamp(f, b, btnPressed);
            //    }
            //}

            if(elevio_obstruction()){
                elevio_stopLamp(1);
            } else {
                elevio_stopLamp(0);
            }
        
        }
        //oppstartskode, typ send ned til 1. etg
        //sett retning, sjekk etg, ikke bruk dører eller obs (tror jeg), 
        //husk at heisknappene må funke, lage btnlights()?

       
        if(elevio_stopButton()){
            elevio_motorDirection(DIRN_STOP);
            break;
        }
        
        nanosleep(&(struct timespec){0, 20*1000*1000}, NULL);
    }

    return 0;
}
