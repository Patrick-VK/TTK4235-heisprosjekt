#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "driver/elevio.h"


int orders[4][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}; //første i lista er om heisen skal stoppe, andre er om retningen har noe å si - knapper på panel utenfor vs inni
//orders[i][1] er begge, orders[i][2] er opp, orders[i][3] er ned 
int orderChanged = 1;
int heading = 1; //settes til 1 fordi heisen skal være i 1. etg ved oppstart
int doorOpen = 0;
int lastFloor = 0;

int ordersEmpty(){ //sjekker om ordrene er tomme
    for(int i = 0; i < N_FLOORS; i++){
        if((orders[i][0])){
            return 0;
        }
    } return 1;
}

void clearOrderRow(int row){ //tømmer en rad
    orders[row][0] = 0;
    orders[row][1] = 0;
    orders[row][2] = 0;
    orders[row][3] = 0;
    orderChanged = 1;
}

void panelLights(){ //fikser lysene til panelene
    if (orderChanged){ 
        for (int i = 0; i < N_FLOORS; i++){
            for (int b = 0; b < N_BUTTONS; b++){
                if (orders[i][0]){
                    if ((orders[i][1] == 1) & (b == 2)){
                        elevio_buttonLamp(i, b, 1);
                    } else if ((orders[i][3] == 1) & (b == 1)){
                        elevio_buttonLamp(i, b, 1);
                    } else if ((orders[i][2] == 1) & (b == 0)){
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

void settBestillinger(){ //sjekker om knappene er trykket inn, og endrer på orders hvis de er det
    for (int i = 0; i < N_FLOORS; i++){ 
        for (int b = 0; b < N_BUTTONS; b++){
            int btnPressed = elevio_callButton(i, b);
            if (btnPressed & b == 2){ //sjekker knappene inni heisen
                orders[i][0] = 1;
                orders[i][1] = 1;
                orderChanged = 1;
            } else if (btnPressed & b == 0){ //sjekker knappene på vei opp
                orders[i][0] = 1;
                orders[i][2] = 1;
                orderChanged = 1;
            } else if (btnPressed & b == 1){ //sjekker knappene på vei ned
                orders[i][0] = 1;
                orders[i][3] = 1;
                orderChanged = 1;
            }
        }         
    }
}

void settHeading(int etg){ //finner ut hviken retning heisen skal bevege seg i
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

void bevegelse(){ // gjør at heisen beveger seg.
    if (ordersEmpty()){
        elevio_motorDirection(DIRN_STOP);
    } else{
        elevio_motorDirection(heading);
    }
    doorOpen = 0;
}

void openDoor(){ //åpner dør
    doorOpen = 1;
    elevio_doorOpenLamp(1);
}

void closeDoor(){ //lukker dør
    doorOpen = 0;
    elevio_doorOpenLamp(0);
}

void waitDoor(){ //venter i tre sekunder, sjekker om det legges inn bestillinger og oppdaterer lys
    time_t start = time(NULL);
    elevio_motorDirection(DIRN_STOP);
    while (time(NULL) - start < 3){
        settBestillinger();
        panelLights();
        //stoppknapp(-1);
    }
}

void obstruksjon(){ //håndterer obstruksjonsknappen
    int obstruksjon = 0;
    elevio_stopLamp(0);
    while((elevio_obstruction()) & (doorOpen == 1)){
        obstruksjon = 1;
        openDoor();
    }
    if(obstruksjon){
        waitDoor();
        closeDoor();
    }
    obstruksjon = 0;
    
    
}

void stoppKnapp(int etg){ //hådterer stoppknappen.
    
    if (elevio_stopButton()){
        elevio_stopLamp(1);
        elevio_motorDirection(DIRN_STOP);
        heading = 0;
        for (int i = 0; i < N_FLOORS; i++){
            clearOrderRow(i);
        }
        panelLights();
        if (etg > -1){
            openDoor();
            while (elevio_stopButton()){
                elevio_motorDirection(DIRN_STOP);
            }
            time_t start = time(NULL);
            elevio_motorDirection(DIRN_STOP);
            while (time(NULL) - start < 3){
                obstruksjon();
            }
            closeDoor();
            }
        else {
            while (elevio_stopButton()){
                elevio_motorDirection(DIRN_STOP);
            }
        }
        
    } else {
        elevio_stopLamp(0);
    }
}

void stopInFloor(int etg){ //sjekker om heisen skal toppe i en etasje
    int orderAmount = 0;
    for (int i = 0; i < N_FLOORS; i++){
        if (orders[i][0]){
            orderAmount += 1;
        }
    }
    if (orderAmount == 1){
        if (orders[etg][0] == 1){
            openDoor();
            waitDoor();
            obstruksjon();
            closeDoor();
            clearOrderRow(etg);
        }
        
    }
    else if (heading == -1){
        if ((orders[etg][0] == 1) & ((orders[etg][1] == 1) || (orders[etg][3] == 1) )){
            openDoor();
            waitDoor();
            obstruksjon();
            closeDoor();
            clearOrderRow(etg);
        }
    }
    else if (heading == 1){
        if ((orders[etg][0] == 1) & ((orders[etg][1] == 1) || (orders[etg][2] == 1))){
            openDoor();
            waitDoor();
            obstruksjon();
            closeDoor();
            clearOrderRow(etg);
        }
    }
}

void floorLights(){ //håndterer lysene som viser hvilken etasje heisen er i
    if (elevio_floorSensor() >= 0){
            elevio_floorIndicator(elevio_floorSensor());
    }
}

int main(){
    elevio_init();
    elevio_motorDirection(DIRN_DOWN);
    int oppstart_bool = 0;

    while(1){
        if (!oppstart_bool){ // oppstart
            panelLights();
            floorLights();
            if(elevio_floorSensor() == 0){
                oppstart_bool = 1;
                elevio_motorDirection(DIRN_STOP);
            }
        }
        //Program-loop
        else {
            int floor = elevio_floorSensor();
    
            settBestillinger();
            settHeading(lastFloor);
            bevegelse();
            floorLights(); //etasjelys
            panelLights(); //panellys
            stoppKnapp(floor);
            if (floor > -1){
                stopInFloor(floor);
                obstruksjon();
                lastFloor = floor;
            }
        }
        nanosleep(&(struct timespec){0, 20*1000*1000}, NULL);
    }

    return 0;
}