#ifndef SHIP_H
#define	SHIP_H

#ifdef	__cplusplus
extern "C" {
#endif
    
//C99 standard dropped M_PI macro.
//This ensures that it is definitely defined.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct ship{
    char ais_id[10];
    location coordinates;
    double course;
    double speed;
    struct ship *next;
    int sunk;
    int visible;
}ship;

ship* createShip(int mode, char line[]);
void insertShip(ship** head, ship* toInsert);
void displayShip(ship* ship);
void listShips(FILE *fr, ship* head);
void checkCollision(FILE *fr, ship* firstShip, ship* otherShip, double tick);
location advanceShip(location currl, float coarse, float speed, double timeDuration);
void displayShipInformation(FILE *fr, ship* sh);

#ifdef	__cplusplus
}
#endif

#endif	/* SHIP_H */


 
