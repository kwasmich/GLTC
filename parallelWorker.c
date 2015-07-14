//
//  ParallelWorker.c
//  PThreadWorker
//
//  Created by Michael Kwasnicki on 13.07.15.
//  Copyright (c) 2015 Michael Kwasnicki. All rights reserved.
//

#include "parallelWorker.h"

#include <assert.h>
#include <iso646.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>



typedef struct {
    int tid;
    uint32_t processCount;
} ThreadOutput_s;


typedef struct {
    WorkItem_s *queue;
    int queueFill;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    bool finalize;
} WorkQueue_s;


typedef struct {
    WorkQueue_s *q;
    int tid;
} ThreadArgs_s;




static WorkQueue_s* createWorkQueue( int const in_SIZE ) {
    WorkQueue_s *q = malloc( sizeof(WorkQueue_s) );
    q->queue = malloc( in_SIZE * sizeof(WorkItem_s) );
    q->queueFill = 0;
    pthread_mutex_init( &q->mutex, NULL );
    pthread_cond_init( &q->cond, NULL );
    q->finalize = false;
    return q;
}



static WorkQueue_s* createWorkQueue2( WorkItem_s *in_out_queue, int const in_SIZE ) {
    WorkQueue_s *q = malloc( sizeof(WorkQueue_s) );
    q->queue = in_out_queue;
    q->queueFill = in_SIZE;
    pthread_mutex_init( &q->mutex, NULL );
    pthread_cond_init( &q->cond, NULL );
    q->finalize = false;
    return q;
}



static void destroyWorkQueue( WorkQueue_s **in_out_q ) {
    pthread_mutex_destroy( &(*in_out_q)->mutex );
    pthread_cond_destroy( &(*in_out_q)->cond );
    free( (*in_out_q)->queue );
    (*in_out_q)->queue = NULL;
    (*in_out_q)->queueFill = 0;
    in_out_q = NULL;
}



static int numWork( WorkQueue_s *in_out_q ) {
    pthread_mutex_lock( &in_out_q->mutex );
    int num = in_out_q->queueFill;
    pthread_mutex_unlock( &in_out_q->mutex );
    return num;
}



static void pushWork( WorkQueue_s *in_out_q, WorkItem_s const in_DATA ) {
    pthread_mutex_lock( &in_out_q->mutex );
    in_out_q->queue[in_out_q->queueFill] = in_DATA;
    in_out_q->queueFill++;
    //printf( "%d\n", in_out_q->queueFill );
    //pthread_cond_signal( &in_out_q->cond ); // wake up one thread
    pthread_cond_broadcast( &in_out_q->cond ); // wake up all threads
    pthread_mutex_unlock( &in_out_q->mutex );
}



static WorkItem_s* popWork( WorkQueue_s *in_out_q ) {
    pthread_mutex_lock( &in_out_q->mutex );
    
    while ( in_out_q->queueFill == 0 and not in_out_q->finalize ) {
        pthread_cond_wait( &in_out_q->cond, &in_out_q->mutex );
    }
    
    WorkItem_s *item;
    
    if ( in_out_q->queueFill == 0 and in_out_q->finalize ) {
        item = NULL;
    } else {
        in_out_q->queueFill--;
        //printf( "%d\n", in_out_q->queueFill );
        item = &(in_out_q->queue[in_out_q->queueFill]);
    }
    
    pthread_mutex_unlock( &in_out_q->mutex );
    return item;
}



static void finalizeWork( WorkQueue_s *in_out_q ) {
    pthread_mutex_lock( &in_out_q->mutex );
    in_out_q->finalize = true;
    pthread_cond_broadcast( &in_out_q->cond ); // wake up all threads
    pthread_mutex_unlock( &in_out_q->mutex );
}



static void* workerThread( void *in_threadData ) {
    ThreadArgs_s *ta = (ThreadArgs_s*)in_threadData;
    uint32_t counter = 0;
    
    while ( true ) {
        WorkItem_s *data = popWork( ta->q );
        
        if ( data ) {
            data->func( data->args );
            counter++;
        } else {
            break;
        }
    }
    
    ThreadOutput_s *output = malloc( sizeof(ThreadOutput_s) );
    output->tid = ta->tid;
    output->processCount = counter;
    pthread_exit( (void*)output );
}




void pwForEach( WorkItem_s *in_out_queue, int const NUM_BLOCKS, int const NUM_THREADS ) {
//    for ( int i = 0; i < NUM_BLOCKS; i++ ) {
//        in_out_queue[i].func( in_out_queue[i].args );
//    }
//    return;
    
    ThreadArgs_s ta[NUM_THREADS];
    pthread_t th[NUM_THREADS];
    WorkQueue_s *q = createWorkQueue2( in_out_queue, NUM_BLOCKS );
    finalizeWork( q );
    
    for ( int t = 0; t < NUM_THREADS; t++ ) {
        ta[t].tid = t;
        ta[t].q = q;
    }
    
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setstacksize( &attr, 2 * 1024 * 1024 );
    
    for ( int t = 0; t < NUM_THREADS; t++ ) {
        int rc = pthread_create( &th[t], &attr, workerThread, (void*)&ta[t] );
        assert( rc == 0 );
    }
    
    for ( int t = 0; t < NUM_THREADS; t++ ) {
        ThreadOutput_s *result;
        int rc = pthread_join( th[t], (void**)&result );
        assert( rc == 0 );
        printf( "%d: %d\n", result->tid, result->processCount );
        free( result );
    }
    
    pthread_attr_destroy( &attr );
    destroyWorkQueue( &q );
}
