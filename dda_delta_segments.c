
/** \file
  \function to create delta segments from cartesian movements

  Patterned off dda_split branch dda_split

  To allow for look-ahead, some sub-movements will have to be held back,
  until following movement(s) come in. Still, the functions here will
  never block.
*/

#include "dda_delta_segments.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "sersendf.h"
#include "dda_queue.h"
#include "dda_maths.h"
#include "dda.h"
#include "debug.h"


void delta_segments_create(TARGET *target) {
  uint32_t dist;
  int32_t s,seg_size;
  int32_t segment_total;

  enum axis_e i;
  TARGET orig_startpoint;
  TARGET segment;
  TARGET diff,diff_scaled,diff_frac;

  if (target == NULL) {
    // it's a wait for temp
    enqueue_home(NULL, 0, 0);
    return;
  }

  orig_startpoint = startpoint;

   seg_size = DELTA_SEGMENT_UM;

   for (i = X; i < AXIS_COUNT; i++){
      diff.axis[i] = target->axis[i] - orig_startpoint.axis[i];
      diff_scaled.axis[i] = diff.axis[i] >> 4;
   }

//   if (diff.axis[X] == 0 && diff.axis[Y] == 0 && diff.axis[Z] == 0)
//      return;

  /* distance whole movement */
  //dist = approx_distance_3((uint32_t)labs(target->axis[X] - orig_startpoint.axis[X]),
  //                         (uint32_t)labs(target->axis[Y] - orig_startpoint.axis[Y]),
  //                         (uint32_t)labs(target->axis[Z] - orig_startpoint.axis[Z]));


  //dist = sqrt((target->axis[X] - orig_startpoint.axis[X])*(target->axis[X]) - orig_startpoint.axis[X])
  //           +(target->axis[Y] - orig_startpoint.axis[Y])*(target->axis[Y]) - orig_startpoint.axis[Y])
  //           +(target->axis[Z] - orig_startpoint.axis[Z])*(target->axis[Z]) - orig_startpoint.axis[Z]));

  dist = SquareRoot32((diff_scaled.axis[X] * diff_scaled.axis[X]) + (diff_scaled.axis[Y] * diff_scaled.axis[Y]) + (diff_scaled.axis[Z] * diff_scaled.axis[Z]));
  dist = dist << 4;

  //dist = sqrt((diff_scaled.axis[X] * diff_scaled.axis[X]) + (diff_scaled.axis[Y] * diff_scaled.axis[Y]) + (diff_scaled.axis[Z] * diff_scaled.axis[Z]));
  //dist = dist << 4;

  if (DEBUG_DELTA && (debug_flags & DEBUG_DELTA)){
    sersendf_P(PSTR("\n\nSeg_Start (%ld,%ld,%ld,E:%ld F:%lu ER:%u)Seg_Target(%ld,%ld,%ld,E:%ld F:%lu ER:%u) dist:%lu\n"),
                orig_startpoint.axis[X], orig_startpoint.axis[Y], orig_startpoint.axis[Z],orig_startpoint.axis[E],orig_startpoint.F,orig_startpoint.e_relative,
                target->axis[X], target->axis[Y], target->axis[Z],target->axis[E],target->F,target->e_relative,dist);
  }

  /*
  //The Marlin/Repetier Approach
  cartesian_move_sec = (dist / target->F) * 60 * 0.001;   //distance(um) * 1mm/1000um * (Feedrate)1min/mm * 60sec/min

  if (target->F > 0)
    segment_total = DELTA_SEGMENTS_PER_SECOND * (dist / target->F) * 60 * 0.001;   //distance(um) * 1mm/1000um * (Feedrate)1min/mm * 60sec/min
  else
    segment_total = 0;
  */

  //Why not just do 1mm-ish segments if not a totally z-move?

  if ((diff.axis[X] == 0 && diff.axis[Y] == 0) || dist < (2 * seg_size))
  {
    segment_total = 1;

    if (DEBUG_DELTA && (debug_flags & DEBUG_DELTA)){
      sersendf_P(PSTR("SEG:Z or small: dist: %lu segs: %lu\n"),
                  dist,segment_total);
    }

    segment = *target;
    enqueue_home(&segment,0,0);
  } else {
    segment_total = dist / seg_size;

    for (i = X; i < AXIS_COUNT; i++)
      diff_frac.axis[i] = diff.axis[i] / segment_total;

    if (DEBUG_DELTA && (debug_flags & DEBUG_DELTA)){
      sersendf_P(PSTR("SEG: Frac(%ld,%ld,%ld,E:%ld) dist: %lu segs: %lu\n"),
                diff_frac.axis[X],diff_frac.axis[Y],diff_frac.axis[Z],diff_frac.axis[E],
                dist, segment_total);
    }
    //if you do all segments, rounding error reduces total - error will accumulate
    for (s=1; s<segment_total; s++){
      if (DEBUG_DELTA && (debug_flags & DEBUG_DELTA)){
        sersendf_P(PSTR("SEG: %d\n"),s);
      }
      for (i = X; i < AXIS_COUNT; i++)
        segment.axis[i] = (orig_startpoint.axis[i] + (s * diff_frac.axis[i]));
        segment.F = target->F;
        segment.e_relative = target->e_relative;
        enqueue_home(&segment,0,0);
    }
    //last bit to make sure we end up at the unsegmented target
    if (DEBUG_DELTA && (debug_flags & DEBUG_DELTA)){
      sersendf_P(PSTR("Seg: Final\n"));
    }
    segment = *target;
    enqueue_home(&segment,0,0);
  }
}