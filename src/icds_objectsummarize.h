#ifndef _ICDS_OBJECTSUMMARIZE_H_
#define _ICDS_OBJECTSUMMARIZE_H_
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
  int max_frame_unseen_to_collect;
  int max_frame_track_per_object;
  float min_percentage_top;
  float min_percentage_best;
  int min_time_value_appear;

}ConfigInfo;

#ifdef __cplusplus
}
#endif

#endif
