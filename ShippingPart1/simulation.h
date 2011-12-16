#include <time.h>

#ifndef SIMULATION_H
#define	SIMULATION_H

#ifdef	__cplusplus
extern "C" {
#endif

#define NORTH 52.833
#define SOUTH 51.667
#define EAST -3.833
#define WEST -6.667
#define WARNING_DISTANCE 2
#define COLLISION_DISTANCE 0.25

void processInput(FILE *input, ship** head, time_t *ts);
void runSimulation(FILE *output, ship** head, time_t ts, double duration, double tick);
void advanceMap(FILE *output, ship** head, double simulationStep);
void checkMap(FILE *output, ship** head, double tick);
    
#ifdef	__cplusplus
}
#endif

#endif	/* SIMULATION_H */

