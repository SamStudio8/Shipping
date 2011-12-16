#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "/dcs/dap/NAVIGATION/navigation.h"
#include "ship.h"
#include "simulation.h"

/*******************************************************************************
 * main.c
 * Sam Nicholls (msn)
 * 
 * Processes user input files and executes the shipping simulation.
 ******************************************************************************/

/**
 * Requests user input and calls the simulator.
 * 
 * @param argc
 * @param argv
 * @return Exit Code
 */
int main(int argc, char** argv) {
    char infilename[50];
    char outfilename[50];
    double duration, tick;
    FILE *inputfr = NULL;
    FILE *outputfr = NULL;

    time_t ts = (time_t) malloc(sizeof(time_t));
    
    ship *head = malloc(sizeof(ship));
    head = NULL;

    while((inputfr = fopen(infilename, "r")) == NULL){
        printf("Input Filename: ");
        scanf("%s", &infilename);
    }
    
    processInput(inputfr, &head, &ts);
    
    if(head == NULL){
        printf("No ships were detected in your input file!");
        exit(1);
    }
    
    while((outputfr = fopen(outfilename, "w")) == NULL){
        printf("Output Filename: ");
        scanf("%s", &outfilename);
    }

    printf("Simulation Duration (Minutes): ");
    scanf("%lf", &duration);
        
    while(fmod(duration, tick) != 0){
        printf("Simulation Tick (Minutes, Factor of Duration): ");
        scanf("%lf", &tick);
    }
    
    fprintf(outputfr, "Ship Simulation Log (Sam Nicholls)\n"
            "Input File Path: %s\n"
            "Simulation Duration: %lf\n"
            "Simulation Timestep: %lf\n"
            "--------------------------------------", 
            infilename, duration, tick);
      
    //Execute simulation.
    runSimulation(outputfr, &head, ts, duration, tick);
    
    //Print and log summary.
    listShips(outputfr, head);

    return (EXIT_SUCCESS);
}

/**
 * Processes the user input file and creates a ship for each entry.
 * 
 * @param fr File pointer to specified input file
 * @param head Pointer to pointer of list containing each ship on map
 * @param t Pointer to time_t containing simulator clock
 */
void processInput(FILE *fr, ship** head, time_t *t){
    char line[80];

    //Get the date.
    if((fgets(line, 80, fr) != NULL)){
        struct tm timestamp;
        sscanf(line, "%d %d %d %d %d %d", 
                &timestamp.tm_mday, &timestamp.tm_mon, &timestamp.tm_year, 
                &timestamp.tm_hour, &timestamp.tm_min, &timestamp.tm_sec);

        timestamp.tm_year -= 1900;
        timestamp.tm_isdst = -1;
        *t = mktime(&timestamp);
    }
    
    //Process ship entries.
    while(fgets(line, 80, fr) != NULL){
        if(strcmp(line, "++++++++\n") != 0){
            insertShip(head, createShip(line));
        }
    }
    fclose(fr);
}

/**
 * Executes the shipping simulation.
 * Loop executes until the time of the simulation matches the calculated end time.
 * Each loop represents a simulation step.
 * The check and advance map methods are called during each step.
 * 
 * @param outputfr Pointer to designated output file
 * @param head Pointer to pointer of LinkedList containing each ship
 * @param ts time_t representing simulator clock
 * @param duration Minutes for which to run the simulation
 * @param tick Minutes between simulation steps
 */
void runSimulation(FILE *outputfr, ship** head, time_t ts, double duration, double tick){
   
    //Set end of simulation for time comparison.
    time_t end = ts + (duration * 60);
    
    while(difftime(ts, end) != 0){
        printf("\n\n++++++++");
        printf("\nSIMULATION TIME: %s", ctime(&ts));
        printf("++++++++");
        fprintf(outputfr, "\n\n++++++++");
        fprintf(outputfr, "\nSIMULATION TIME: %s", ctime(&ts));
        fprintf(outputfr, "++++++++");
        
        checkMap(outputfr, head, tick);
        advanceMap(outputfr, head, tick); //Tick in minutes.
        ts = ts+(tick*60); //Tick as seconds to update timestamp. 
    }
}

/**
 * Traverses list of all ships on the map, updating co-ordinates and checking region bounds.
 * Each ship is moved during each "step" of the simulation.
 * Following this move, the region bounds are checked for ships that may have crossed it.
 *  
 * @param fr Pointer to specified output file
 * @param head Pointer to pointer of LinkedList containing each ship on map
 * @param simulationStep Minutes between simulation steps
 */
void advanceMap(FILE *fr, ship** head, double simulationStep){
    
    ship *node = *head;
    while(node){
        node->coordinates = advanceShip(node->coordinates, node->course, node->speed, simulationStep);
        
        //Ship that was previously outside the area of interest sails in:
        if(node->visible == 0
                && (node->coordinates.lng > WEST
                && node->coordinates.lng < EAST
                && node->coordinates.lat < NORTH
                && node->coordinates.lat > SOUTH)){
            node->visible = 1;
            printf("\n[ IN ] %s entered designated area: (%lf, %lf)", 
                    node->ais_id, node->coordinates.lat, node->coordinates.lng);
            fprintf(fr, "\n[ IN ] %s entered designated area: (%lf, %lf)", 
                    node->ais_id, node->coordinates.lat, node->coordinates.lng);
            
        }
        //Ship that was previously visible moves outside of the area of interest:
        else if(node->visible == 1
                && (node->coordinates.lng < WEST
                || node->coordinates.lng > EAST
                || node->coordinates.lat > NORTH
                || node->coordinates.lat < SOUTH)){
            node->visible = 0;
            printf("\n[OUT ] %s left designated area: (%lf, %lf)", 
                    node->ais_id, node->coordinates.lat, node->coordinates.lng);
            fprintf(fr, "\n[OUT ] %s left designated area: (%lf, %lf)", 
                    node->ais_id, node->coordinates.lat, node->coordinates.lng);
        }
        node = node->next;
    }
    printf("\n[TIME] Simulation advanced by %lf minutes.", simulationStep);
    fprintf(fr, "\n[TIME] Simulation advanced by %lf minutes.", simulationStep);
}

/**
 * Checks for collisions between ships on the map.
 * The method traverses two LinkedLists; each containing all the ships,
 * the loops are written to ensur no two ships are compared to each other twice.
 * 
 * @param fr Pointer to specified output file
 * @param head Pointer to pointer of LinkedList containing each ship on map
 * @param tick Minutes between simulation steps
 */
void checkMap(FILE *fr, ship** head, double tick){
    ship *firstNode = *head;
    ship *otherNode = *head;

    while(firstNode){
        otherNode = firstNode->next;
        while(otherNode){
            checkCollision(fr, firstNode, otherNode, tick);
            otherNode = otherNode->next;
        }
        firstNode = firstNode->next;
    }
}
