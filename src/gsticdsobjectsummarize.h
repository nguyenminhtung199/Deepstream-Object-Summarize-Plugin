#ifndef __GST_ICDSOBJECTSUMMARIZE_H__
#define __GST_ICDSOBJECTSUMMARIZE_H__

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "gstnvdsmeta.h"
#include "icds_objectsummarize.h"

/* Package and library details required for plugin_init */
#define PACKAGE "icdsobjectsummarize"
#define VERSION "1.0"
#define LICENSE "Proprietary"
#define DESCRIPTION "IC dsobjectsummarize plugin for integration with DeepStream on DGPU/Jetson"
#define BINARY_PACKAGE "IC DeepStream dsobjectsummarize plugin"
#define URL "https://icomm.vn/"


G_BEGIN_DECLS
/* Standard boilerplate stuff */
typedef struct _GstIcDsObjectSummarize GstIcDsObjectSummarize;
typedef struct _GstIcDsObjectSummarizeClass GstIcDsObjectSummarizeClass;

/* Standard boilerplate stuff */
#define GST_TYPE_ICDSOBJECTSUMMARIZE (gst_icdsobjectsummarize_get_type())
#define GST_ICDSOBJECTSUMMARIZE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_ICDSOBJECTSUMMARIZE, GstIcDsObjectSummarize))
#define GST_ICDSOBJECTSUMMARIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_ICDSOBJECTSUMMARIZE, GstIcDsObjectSummarizeClass))
#define GST_ICDSOBJECTSUMMARIZE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_ICDSOBJECTSUMMARIZE, GstIcDsObjectSummarizeClass))
#define GST_IS_ICDSOBJECTSUMMARIZE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_ICDSOBJECTSUMMARIZE))
#define GST_IS_ICDSOBJECTSUMMARIZE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_ICDSOBJECTSUMMARIZE))
#define GST_ICDSOBJECTSUMMARIZE_CAST(obj)  ((GstIcDsObjectSummarize *)(obj))


struct _GstIcDsObjectSummarize
{
  GstBaseTransform base_trans;
  guint unique_id;
  gchar *config_file_path;
  gboolean config_file_parse_successful;
  ConfigInfo *config_info;
  GMutex objectsummarize_mutex;
  gint configuration_max_frame_track_per_object;
  gint configuration_max_frame_unseen_to_collect;
  gfloat configuration_min_percentage_top;
  gfloat configuration_min_percentage_best;
  gint configuration_min_time_value_appear;
};

// Boiler plate stuff
struct _GstIcDsObjectSummarizeClass
{
  GstBaseTransformClass parent_class;
};

GType gst_icdsobjectsummarize_get_type(void);

struct ObjectMeta {
    std::vector<int> list_frame_ids;
    std::vector<std::string> list_object_labels;
    std::unordered_map<std::string, int> attribute_summarized;
    std::pair<std::string, float> best_attributes; // Thêm thuộc tính này
    std::vector<std::pair<std::string, float>> top_attributes;
    int unseen_frame_count;
};



G_END_DECLS
#endif /* __GST_ICDSOBJECTSUMMARIZE_H__ */
