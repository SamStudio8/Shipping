#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "/dcs/dap/NAVIGATION/navigation.h"
#include "ship.h"
#include "simulation.h"

/*******************************************************************************
 * ship.c
 * Sam Nicholls (msn)
 * 
 * Defines methods to be performed on ship structs during the simulation.
 * Includes; creation and addition of ships to a LinkedList, ship collision
 * checking which mathematically solves the equation of lines to check crashes,
 * listing of ships and advancing of ship co-ordinates.
 ******************************************************************************/

/**
 * Creates a ship with location, speed and course from the user defined input file.
 * Defines visible and sunk to 0 ("false") and sets the next ship to NULL.
 * 
 * @param mode An int to denote whether the method is adding a ship or ship update.
 * @param line The line fetched from the user defined input file.
 * @return Pointer to ship
 */
ship* createShip(int mode, char line[]){
    ship *sh = (ship *) malloc(sizeof(ship));

    switch(mode){
        case 0:
            sscanf(line, "%s %lf %lf %lf %lf", 
                    &sh->ais_id, &sh->coordinates.lat, &sh->coordinates.lng,
                    &sh->course, &sh->speed);
            break;
            
        case 1:
            sscanf(line, "%s %lf %lf", &sh->ais_id, &sh->course, &sh->speed);
            break;
    }
    
    sh->next = NULL;
    sh->sunk = 0;
    sh->visible = 0;
    return sh;
}

/**
 * Inserts a ship to a LinkedList of ships.
 * If the node is not defined, the ship to insert is inserted.
 * Otherwise, the function is called recursively using the next node until the 
 * ship can be inserted.
 * 
 * @param node The node in which to check for the ability to insert.
 * @param toInsert The ship to be inserted.
 */
void insertShip(ship** node, ship* toInsert){
    if(*node == NULL){
        *node = toInsert;
    }
    else{
        insertShip(&(*node)->next, toInsert);
    }
}

/**
 * Check for a collision between two ships.
 * First the function provided by the specification is used to calculate the 
 * great circle distance between the two ships.
 * If the distance is within the defined collision distance, the ships are set
 * as sunk and a message is printed to the display and log.
 * 
 * If the ships are within risk range, the method solves a simultaneous linear
 * equation of two lines to check for the possibility of collision.
 * A message is printed and logged. 
 * 
 * @param fr Pointer to designated output file
 * @param firstShip The first ship to compare
 * @param otherShip The other ship to compare
 * @param tick Minutes between simulation steps
 */
void checkCollision(FILE *fr, ship* firstShip, ship* otherShip, double tick){
    
    double distance = great_circle(firstShip->coordinates, otherShip->coordinates);
    
    if(firstShip->sunk == 0 && otherShip->sunk == 0 && distance <= COLLISION_DISTANCE){
        printf("\n[SUNK] %s and %s collided and sunk: (%lf, %lf)", 
                firstShip->ais_id, otherShip->ais_id,
                firstShip->coordinates.lat, firstShip->coordinates.lng);
        fprintf(fr, "\n[SUNK] %s and %s collided and sunk: (%lf, %lf)", 
                firstShip->ais_id, otherShip->ais_id,
                firstShip->coordinates.lat, firstShip->coordinates.lng);
        
            firstShip->sunk = 1;
            otherShip->sunk = 1;
    }
    else if(firstShip->sunk == 0 && otherShip->sunk == 0 && distance <= WARNING_DISTANCE){
        
        ship *firstShipA1 = (ship *) malloc(sizeof(ship));
        ship *otherShipB1 = (ship *) malloc(sizeof(ship));

        float slopeA = 0;
        float slopeB = 0;
        float interceptA, interceptB, intersectX, intersectY;

        firstShipA1->coordinates = advanceShip(firstShip->coordinates, 
                firstShip->course, firstShip->speed, tick);
        otherShipB1->coordinates = advanceShip(otherShip->coordinates, 
                otherShip->course, otherShip->speed, tick);

        //If the boats are moving, find t(1) for each boat.
        if(firstShip->speed != 0){
            slopeA = ((firstShipA1->coordinates.lat - firstShip->coordinates.lat)
                    /(firstShipA1->coordinates.lng - firstShip->coordinates.lng));
        }
        if(otherShip->speed != 0){
            slopeB = (otherShipB1->coordinates.lat - otherShip->coordinates.lat)
                    /(otherShipB1->coordinates.lng - otherShip->coordinates.lng);
        }
        
        //Find the y intercepts for each line.
        interceptA = firstShip->coordinates.lat - (slopeA * firstShip->coordinates.lng);
        interceptB = otherShip->coordinates.lat - (slopeB * otherShip->coordinates.lng);

        //Equate the lines to each other to find x.
        //Then insert value for x into a line equation to find y.
        intersectX = (interceptB - interceptA) / (slopeA - slopeB);
        intersectY = (slopeA * intersectX) + interceptA;

        //If either ship's course is vertical, override the above calculations.
        if((firstShip->course == 0) || (firstShip->course == 180)){
            intersectX = firstShip->coordinates.lng;
            intersectY = (slopeB * intersectX) + interceptB;
        }
        if((otherShip->course == 0) || (otherShip->course == 180)){
            intersectX = otherShip->coordinates.lng;
            intersectY = (slopeA * intersectX) + interceptA;
        }
        
        //Ensure the collision is in the area in which we are interested.
        if(intersectX > WEST && intersectX < EAST 
                && intersectY < NORTH && intersectY > SOUTH){
            printf("\n[RISK] %s and %s are at risk of collision: (%lf, %lf)",
                    firstShip->ais_id, otherShip->ais_id,
                    intersectX, intersectY);
                    
            fprintf(fr, "\n%[RISK] %s and %s are at risk of collision: (%lf, %lf)",
                    firstShip->ais_id, otherShip->ais_id,
                    intersectX, intersectY);
        }
    }
}

/**
 * Print and log current summary of ships.
 * Traverses LinkedList calling the displayShipInformation method.
 * 
 * @param fr
 * @param node
 */
void listShips(FILE *fr, ship* node){
    printf("\n\nSHIP SUMMARY");
    fprintf(fr, "\n\nSHIP SUMMARY");
    
    while(node){
        displayShipInformation(fr, node);
        node = node->next;
    }
}

/**
 * Advances the location of a ship.
 * 
 * @param currl Ship's current location
 * @param course Ship's current course
 * @param speed Ship's current speed
 * @param timeDuration Minutes of time to advance the ship for
 * @return New location of ship
 */
location advanceShip(location currl, float course, float speed, double timeDuration){
    location l;
    
    float radian_course = (course * M_PI)/180;
    float radian_lat = (currl.lat * M_PI)/180;

    l.lat = currl.lat + (speed * cos(radian_course) * timeDuration) / 3600.0;
    l.lng = currl.lng + (speed * sin(radian_course) * timeDuration / cos(radian_lat)) / 3600.0;

    return l;
}

/**
 * Prints and logs a line summary for a ship.
 * Including status, name, location, speed and course - if available.
 * 
 * @param fr Pointer to designated output file
 * @param sh Ship for which to generate line summary
 */
void displayShipInformation(FILE *fr, ship* sh){
    
    if(sh->sunk == 1){
        printf("\n[SUNK] %s \t(%.2lf %.2lf)", 
            sh->ais_id, sh->coordinates.lat, sh->coordinates.lng);
        fprintf(fr, "\n[SUNK] %s \t(%.2lf %.2lf)", 
            sh->ais_id, sh->coordinates.lat, sh->coordinates.lng);
    }
    else if(sh->visible == 0){
        printf("\n[OUT ] %s \t Other Information Unavailable.", sh->ais_id);
        fprintf(fr, "\n[OUT ] %s \t Other Information Unavailable.", sh->ais_id);
    }
    else{
        printf("\n[OKAY] %s \t(%.2lf %.2lf)\tSpeed: %.2lf\tCourse: %.2lf", 
            sh->ais_id, sh->coordinates.lat, sh->coordinates.lng,
            sh->speed, sh->course);
        fprintf(fr, "\n[OKAY] %s \t(%.2lf %.2lf)\tSpeed: %.2lf\tCourse: %.2lf", 
            sh->ais_id, sh->coordinates.lat, sh->coordinates.lng,
            sh->speed, sh->course);
    }
}
