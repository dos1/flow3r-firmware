#include "badge23/scope.h"
#include <string.h>

scope_t * scope;

void init_scope(uint16_t size){
    scope_t * scp = malloc(sizeof(scope_t));
    if(scp == NULL) scope = NULL;
    scp->buffer_size = size;
    scp->buffer = malloc(sizeof(int16_t) * scp->buffer_size);
    if(scp->buffer == NULL){
        free(scp);
        scope = NULL;
    } else {
        memset(scp->buffer, 0, sizeof(int16_t) * scp->buffer_size);
        scope = scp;
    }
}

void write_to_scope(int16_t value){
    if(scope->is_being_read) return;
    if(scope == NULL) return;
    scope->write_head_position++;
    if(scope->write_head_position >= scope->buffer_size) scope->write_head_position = 0;
    scope->buffer[scope->write_head_position] = value;
}

void begin_scope_read(){
    if(scope == NULL) return;
    scope->is_being_read = 1; //best mutex
}

void read_line_from_scope(uint16_t * line, int16_t point){
    memset(line, 0, 480);
    int16_t startpoint = scope->write_head_position - point;
    while(startpoint < 0){
        startpoint += scope->buffer_size;
    }
    int16_t stoppoint = startpoint - 1;
    if(stoppoint < 0){
        stoppoint += scope->buffer_size;
    }
    int16_t start = (scope->buffer[point]/32 + 120);
    int16_t stop = (scope->buffer[point+1]/32 + 120);
    if(start>240)   start = 240;
    if(start<0)     start = 0;
    if(stop>240)    stop = 240;
    if(stop<0)     stop = 0;

    if(start > stop){
        int16_t inter = start;
        start = stop;
        stop = inter;
    }
    for(int16_t i = start; i <= stop; i++){
        line[i] = 255;
    }
}

void end_scope_read(){
    if(scope == NULL) return;
    scope->is_being_read = 0; //best mutex
}
