#ifndef ONE_ST_CONSTANTS_UTILITIES
#define ONE_ST_CONSTANTS_UTILITIES

#define MY_VERBOSE                  (1)

//magic numbers
#define MIN_NUM_INPUT_POINTS        (3)
#define MAX_NUM_INPUT_POINTS        (10)
#define DEFAULT_NUM_INPUT_POINTS    (MIN_NUM_INPUT_POINTS)

#define GRID_LENGTH_DEFAULT  (10)

#define RAND_SEED_DEFAULT    (10)
#define TRUE_RANDOM_DEFAULT  (false)

#define ONLY_POINTS_DEFAULT  (false)

bool intInRangeInclusive(int x, int lowEnd, int highEnd){
    return ((x >= lowEnd) && (x <= highEnd));
}


#endif