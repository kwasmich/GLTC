//
//  ParallelWorker.h
//  PThreadWorker
//
//  Created by Michael Kwasnicki on 13.07.15.
//  Copyright (c) 2015 Michael Kwasnicki. All rights reserved.
//

#ifndef __PThreadWorker__ParallelWorker__
#define __PThreadWorker__ParallelWorker__


typedef struct {
    void *args;
    void (*func)(void*);
} WorkItem_s;



void pwForEach( WorkItem_s *in_out_queue, int const NUM_BLOCKS, int const NUM_THREADS );



#endif /* defined(__PThreadWorker__ParallelWorker__) */
