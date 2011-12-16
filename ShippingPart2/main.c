#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "/dcs/dap/NAVIGATION/navigation.h"
#include "ship.h"
#include "update.h"
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

    update *updates = malloc(sizeof(update));
    updates = NULL;

    while((inputfr = fopen(infilename, "r")) == NULL){
        printf("Input Filename: ");
        scanf("%s", &infilename);
    }
    
    processInput(inputfr, &updates);
    if(updates == NULL){
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
    runSimulation(outputfr, &updates, duration, tick);
    
    //Print and log summary.
    listShips(outputfr, updates->head);

    return (EXIT_SUCCESS);
}

/**
 * Processes the user input file, creating ships and updates.
 * 
 * @param fr File pointer to specified input file
 * @param updates Pointer to pointer of LinkedList containing each ship update
 */
void processInput(FILE *fr, update** updates){
    char line[80];
   
    int mode = 0;
    int newUpdate = 1;
    update *u;
    while(fgets(line, 80, fr) != NULL){
        if(strcmp(line, "++++++++\n") != 0){
            if(newUpdate == 1){
                u = createUpdate(line);
                newUpdate = 0;
            }
            else{
                insertShip(&u->head, createShip(mode, line));
            }
        }
        else{
            insertUpdate(updates, u);
            u = malloc(sizeof(update));
            newUpdate = 1;
            mode = 1;
        }
    }
    
    //Ensures that an update is created for files with no updates.
    if(mode == 0){
        insertUpdate(updates, u);
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
 * @param updates Pointer to pointer of LinkedList containing each ship update
 * @param duration Minutes for which to run the simulation
 * @param tick Minutes between simulation steps
 */
void runSimulation(FILE *outputfr, update** updates, double duration, double tick){

    //Set simulation to begin with first "update".
    ship** head = &(*updates)->head;
    time_t ts = (*updates)->timestamp;
    
    //Set end of simulation for time comparison.
    time_t end = ts + (duration * 60);
    
    //Initialise doubles used to store time differences between simulation updates.
    double secondsToNextUpdate = 0;
    double secondsResetTickAfterUpdate = 0;
    
    while(difftime(ts, end) != 0){
        printf("\n\n++++++++");
        printf("\nSIMULATION TIME: %s", ctime(&ts));
        printf("++++++++");
        fprintf(outputfr, "\n\n++++++++");
        fprintf(outputfr, "\nSIMULATION TIME: %s", ctime(&ts));
        fprintf(outputfr, "++++++++");
        
        secondsToNextUpdate = checkUpdates(outputfr, updates, head, tick, ts);
        checkMap(outputfr, head, tick);
        
        //If secondsToNextUpdate is 0, there is an update on this tick, or no updates.
        if(secondsToNextUpdate == 0){
            if(secondsResetTickAfterUpdate == 0){
                advanceMap(outputfr, head, tick); //Tick as minutes.
                ts = ts+(tick*60); //Tick converted to seconds to update clock.
            }
            else{
                //Correct simulation time after an update if required.
                advanceMap(outputfr, head, tick - (secondsResetTickAfterUpdate / 60));
                ts = ts+((tick*60)-secondsResetTickAfterUpdate);
                secondsResetTickAfterUpdate = 0;
            }
        }
        //Otherwise there is an update before the next simulation tick.
        else{
            advanceMap(outputfr, head, (secondsToNextUpdate / 60));
            ts = ts+(secondsToNextUpdate);
            secondsResetTickAfterUpdate = secondsToNextUpdate;
        } 
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
