#ifndef __SPORT_UI_H_
#define __SPORT_UI_H_

/*
 * CONSTANTS
 *******************************************************************************
 */
#define ENABLE                      1
#define DISABLE                     0
#define SPORTS_ITEM_NUM             4


/*********************************************************************
 * TYPEDEFS
 */
typedef enum {
    SPORT_ITEM_BASE = 1,
    SPORT_OUTDOOR_RUNNING = 1,
    SPORT_INDOOR_RUNNING,
    SPORT_OUTDOOR_BIKE,
//    SPORT_INDOOR_BIKE,
    SPORT_FREETRAINNING,
    SPORT_INDOOR_SWIMMING,
    SPORT_OUTDOOR_WALK,
}sports_id_t;

/*
 * VARIABLES
 *******************************************************************************
 */
extern activity_t activity_sports;

#endif  //#ifndef __SPORT_UI_H_
