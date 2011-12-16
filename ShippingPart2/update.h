#include "ship.h"

#ifndef UPDATE_H
#define	UPDATE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct update{
    time_t timestamp;
    ship *head;
    struct update *next;
}update;

update* createUpdate(char line[]);
void insertUpdate(update** updates, update* toInsert);
double checkUpdates(FILE *outputfr, update** updates, ship** head, double tick, time_t ts);

void processInput(FILE *input, update** updates);
void runSimulation(FILE *output, update** updates, double duration, double tick);
void advanceMap(FILE *output, ship** head, double simulationStep);
void checkMap(FILE *output, ship** head, double tick);

#ifdef	__cplusplus
}
#endif

#endif	/* UPDATE_H */
