#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "/dcs/dap/NAVIGATION/navigation.h"
#include "update.h"

/*******************************************************************************
 * update.c
 * Sam Nicholls (msn)
 * 
 * Defines methods to be performed on update structs during the simulation.
 * Methods allow creation of updates, insertion to a Linked List structure and
 * a check updates method called during the simulation to find whether any
 * updates need to be applied to ships on the map as defined by the input file. * 
 ******************************************************************************/

/**
 * Creates an update using data from the user's input file.
 * 
 * @param line Line of input from user's input file.
 * @return Pointer to an update struct.
 */
update* createUpdate(char line[]){
    update *u = malloc(sizeof(update));
    
    struct tm timestamp;
    time_t t = (time_t) malloc(sizeof(time_t));
    
    sscanf(line, "%d %d %d %d %d %d", 
            &timestamp.tm_mday, &timestamp.tm_mon, &timestamp.tm_year, 
            &timestamp.tm_hour, &timestamp.tm_min, &timestamp.tm_sec);
    
    timestamp.tm_year -= 1900;
    timestamp.tm_isdst = -1;
    
    t = mktime(&timestamp);
    u->timestamp = t;
    u->head = NULL;
    u->next = NULL;
    return u;
}

/**
 * Inserts an update to a LinkedList of updates.
 * If the node is not defined, the update to insert is inserted.
 * Otherwise, the function is called recursively using the next node until the 
 * update can be inserted.
 * 
 * @param node The node in which to check for the ability to insert.
 * @param toInsert The update to be inserted.
 */
void insertUpdate(update** node, update* toInsert){
    if(*node == NULL){
        *node = toInsert;
    }
    else{
        insertUpdate(&(*node)->next, toInsert);
    }
}

/**
 * Using the current simulation timestamp, checks whether any updates need to be
 * applied to the map now, or between now and the next simulation step.
 * 
 * If there is an update to be applied, the function returns 0 as the two times
 * are exactly the same.
 * 
 * The function also returns 0 if there are no updates to be applied between now
 * and the next time step.
 * 
 * The function returns the difference between now and the next update to be 
 * applied if there is an update before the next simulation step.
 * 
 * @param outputfr Pointer to designated output file
 * @param updates Pointer to pointer of LinkedList containing ship updates
 * @param head Point to pointer of LinkedList containing all ships on the map
 * @param tick Minutes between each simulation step
 * @param ts Simulation's current timestamp as a time_t
 * @return The difference between the current time and the next update
 */
double checkUpdates(FILE *outputfr, update** updates, ship** head, double tick, time_t ts){
    update *node = *updates;
    
    while(node){
        double diff = (difftime(node->timestamp, ts));
        if(difftime(ts, node->timestamp) == 0){
            ship *firstNode = *head;
            ship *updateNode = node->head;

            while(firstNode){
                while(updateNode){
                    if(strcmp(firstNode->ais_id, updateNode->ais_id) == 0){
                        firstNode->course = updateNode->course;
                        firstNode->speed = updateNode->speed;
                        printf("\n%s updated; Course: %lf, Speed: %lf", 
                                firstNode->ais_id, firstNode->course, firstNode->speed);
                        fprintf(outputfr, "\n%s updated; Course: %lf, Speed: %lf", 
                                firstNode->ais_id, firstNode->course, firstNode->speed);
                    }
                    updateNode = updateNode->next;
                }
                updateNode = node->head;
                firstNode = firstNode->next;
            }
            return 0;
        }
        //Else, seconds between now and the next update is less than tick in seconds.
        else if((diff > 0) && (diff < (tick * 60))){
            return diff;
        }
        node = node->next;
    }
    return 0;
}
