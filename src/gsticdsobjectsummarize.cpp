#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <ostream>
#include <fstream>
#include <cstring>
#include <map>
#include <cmath>
#include "gsticdsobjectsummarize.h"
#include "icdsobjectsummarize_property_parser.h"
#include <sys/time.h>
#include <regex>
GST_DEBUG_CATEGORY_STATIC (gst_icdsobjectsummarize_debug);
#define GST_CAT_DEFAULT gst_icdsobjectsummarize_debug

static GQuark _dsmeta_quark = 0;

/* Enum to identify properties */
enum
{
  PROP_0,
  PROP_CONFIG_FILE
};


/* By default NVIDIA Hardware allocated memory flows through the pipeline. We
 * will be processing on this type of memory only. */
#define GST_CAPS_FEATURE_MEMORY_NVMM "memory:NVMM"
static GstStaticPadTemplate gst_icdsobjectsummarize_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_NVMM,
            "{ NV12, RGBA }")));

static GstStaticPadTemplate gst_icdsobjectsummarize_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_NVMM,
            "{ NV12, RGBA }")));

/* Define our element type. Standard GObject/GStreamer boilerplate stuff */
#define gst_icdsobjectsummarize_parent_class parent_class
G_DEFINE_TYPE (GstIcDsObjectSummarize, gst_icdsobjectsummarize, GST_TYPE_BASE_TRANSFORM);

static void gst_icdsobjectsummarize_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_icdsobjectsummarize_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_icdsobjectsummarize_finalize (GObject * object);
static gboolean gst_icdsobjectsummarize_start (GstBaseTransform * btrans);
static gboolean gst_icdsobjectsummarize_stop (GstBaseTransform * btrans);
static GstFlowReturn gst_icdsobjectsummarize_transform_ip (GstBaseTransform * btrans, GstBuffer * inbuf);


/* Install properties, set sink and src pad capabilities, override the required
 * functions of the base class, These are common to all instances of the
 * element.
 */
static void
gst_icdsobjectsummarize_class_init (GstIcDsObjectSummarizeClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseTransformClass *gstbasetransform_class;

  // Indicates we want to use DS buf api
  g_setenv ("DS_NEW_BUFAPI", "1", TRUE);

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasetransform_class = (GstBaseTransformClass *) klass;

  /* Overide base class functions */
  gobject_class->set_property = GST_DEBUG_FUNCPTR(gst_icdsobjectsummarize_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR(gst_icdsobjectsummarize_get_property);
  gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_icdsobjectsummarize_finalize);
  gstbasetransform_class->start = GST_DEBUG_FUNCPTR(gst_icdsobjectsummarize_start);
  gstbasetransform_class->stop = GST_DEBUG_FUNCPTR(gst_icdsobjectsummarize_stop);
  gstbasetransform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_icdsobjectsummarize_transform_ip);

  g_object_class_install_property (gobject_class, PROP_CONFIG_FILE,
    g_param_spec_string ("config-file-path", "DsObjectinfo Config File",
      "DsObjectinfo Config File", NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  /* Set sink and src pad capabilities */
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get(&gst_icdsobjectsummarize_src_template));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get(&gst_icdsobjectsummarize_sink_template));

  /* Set metadata describing the element */
  gst_element_class_set_details_simple (gstelement_class,
      "DsObjectSummary plugin",
      "DsObjectSummary Plugin",
      "Process Summary Object Info ",
      "NVIDIA Corporation. Post on Deepstream forum for any queries "
      "@ https://devtalk.nvidia.com/default/board/209/");
}

gint max_frame_unseen_to_collect = 0;
gint max_frame_track_per_object = 0;
gfloat min_percentage_best = 0;
gfloat min_percentage_top = 0;
gint min_time_value_appear = 0;

static void get_template_config (GstIcDsObjectSummarize * icdsobjectsummarize)
{
  ConfigInfo &config_info = *(icdsobjectsummarize->config_info);
  max_frame_unseen_to_collect = config_info.max_frame_unseen_to_collect;
  max_frame_track_per_object = config_info.max_frame_track_per_object;
  min_percentage_best = config_info.min_percentage_best;
  min_percentage_top = config_info.min_percentage_top;
  min_time_value_appear = config_info.min_time_value_appear;
}

static void 
reset_config_info(ConfigInfo* config_info) {
    if (config_info) {
        config_info->max_frame_unseen_to_collect = INT_MIN;
        config_info->max_frame_track_per_object = INT_MIN;
        config_info->min_percentage_best = INT_MIN;
        config_info->min_percentage_top = INT_MIN;
        config_info->min_time_value_appear = INT_MIN;
    }
}

static void
gst_icdsobjectsummarize_finalize (GObject * object)
{
  GstIcDsObjectSummarize *icdsobjectsummarize = GST_ICDSOBJECTSUMMARIZE (object);
  icdsobjectsummarize->config_file_path = NULL;
  icdsobjectsummarize->config_file_parse_successful = FALSE;
  reset_config_info(icdsobjectsummarize->config_info);
  g_mutex_clear (&icdsobjectsummarize->objectsummarize_mutex);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_icdsobjectsummarize_init (GstIcDsObjectSummarize * icdsobjectsummarize)
{
  GstBaseTransform *btrans = GST_BASE_TRANSFORM (icdsobjectsummarize);

  icdsobjectsummarize->configuration_max_frame_track_per_object = 0;
  icdsobjectsummarize->configuration_max_frame_unseen_to_collect = 0;
  icdsobjectsummarize->configuration_min_percentage_best = 0;
  icdsobjectsummarize->configuration_min_percentage_top = 0;
  icdsobjectsummarize->configuration_min_time_value_appear = 0;
  icdsobjectsummarize->config_file_path = NULL;
  icdsobjectsummarize->config_file_parse_successful = FALSE;
  icdsobjectsummarize->config_info = new ConfigInfo;

  g_mutex_init (&icdsobjectsummarize->objectsummarize_mutex);

  /* This quark is required to identify NvDsMeta when iterating through
   * the buffer metadatas */
  if (!_dsmeta_quark)
    _dsmeta_quark = g_quark_from_static_string (NVDS_META_STRING);
}

/* Function called when a property of the element is set. Standard boilerplate.
 */
static void
gst_icdsobjectsummarize_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstIcDsObjectSummarize *icdsobjectsummarize = GST_ICDSOBJECTSUMMARIZE (object);
  switch (prop_id) {
    case PROP_CONFIG_FILE:
    {
      // Get config for custom plugin
      g_mutex_lock (&icdsobjectsummarize->objectsummarize_mutex);
      g_free (icdsobjectsummarize->config_file_path);
      icdsobjectsummarize->config_file_path = g_value_dup_string (value);
      icdsobjectsummarize->config_file_parse_successful = icdsobjectsummarize_parse_config_file (icdsobjectsummarize, icdsobjectsummarize->config_file_path);
      get_template_config(icdsobjectsummarize);


      g_mutex_unlock (&icdsobjectsummarize->objectsummarize_mutex);
    }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* Function called when a property of the element is requested. Standard
 * boilerplate.
 */
static void
gst_icdsobjectsummarize_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstIcDsObjectSummarize *icdsobjectsummarize = GST_ICDSOBJECTSUMMARIZE (object);
  switch (prop_id) {
    case PROP_CONFIG_FILE:
      g_value_set_string (value, icdsobjectsummarize->config_file_path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/**
 * Initialize all resources and start the output thread
 */
static gboolean
gst_icdsobjectsummarize_start (GstBaseTransform * btrans)
{
  GstIcDsObjectSummarize *icdsobjectsummarize = GST_ICDSOBJECTSUMMARIZE (btrans);
  // std::cout << "Config object info plugin: " << max_frame_unseen_to_collect << max_frame_track_per_object << min_percentage_best << min_percentage_top << min_time_value_appear << std::endl;
  g_print("Config object info plugin: %d %d %f %f %d \n", max_frame_unseen_to_collect, max_frame_track_per_object, min_percentage_best, min_percentage_top, min_time_value_appear);
    if (!icdsobjectsummarize->config_file_path || strlen (icdsobjectsummarize->config_file_path) == 0) {
    GST_ELEMENT_ERROR (icdsobjectsummarize, LIBRARY, SETTINGS, ("Configuration file not provided"), (nullptr));
    return FALSE;
  }
  if (icdsobjectsummarize->config_file_parse_successful == FALSE) {
    GST_ELEMENT_ERROR (icdsobjectsummarize, LIBRARY, SETTINGS,
        ("Configuration file parsing failed"),
        ("Config file path: %s", icdsobjectsummarize->config_file_path));
    return FALSE;
  }
  return TRUE;
}

/**
 * Stop the output thread and free up all the resources
 */
static gboolean
gst_icdsobjectsummarize_stop (GstBaseTransform * btrans)
{
  GstIcDsObjectSummarize *icdsobjectsummarize = GST_ICDSOBJECTSUMMARIZE (btrans);
  GST_DEBUG_OBJECT(icdsobjectsummarize, "ctx lib released \n");
  return TRUE;
}
// Khai báo cấu trúc dữ liệu cache_objects_tracking
std::unordered_map<std::string, std::map<guint64, std::map<int32_t, ObjectMeta>>> cache_objects_tracking;


static GstFlowReturn
gst_icdsobjectsummarize_transform_ip (GstBaseTransform * btrans, GstBuffer * inbuf)
{
  GstIcDsObjectSummarize *icdsobjectsummarize = GST_ICDSOBJECTSUMMARIZE (btrans);
  GstFlowReturn flow_ret = GST_FLOW_ERROR;
  NvDsBatchMeta *batch_meta = NULL;
  NvDsFrameMeta *frame_meta = NULL;
  NvDsMetaList *l_frame = NULL;
  // unique_id = icdsobjectsummarize->unique_id
  
  ConfigInfo &config_info = *(icdsobjectsummarize->config_info);
  batch_meta = gst_buffer_get_nvds_batch_meta (inbuf);
  if (nullptr == batch_meta) {
    GST_ELEMENT_ERROR (icdsobjectsummarize, STREAM, FAILED,
        ("NvDsBatchMeta not found for input buffer."), (NULL));
    return flow_ret;
  }
  NvDsMetaList *l_obj = nullptr;
  NvDsMetaList *l_class = nullptr;
  NvDsMetaList *l_label = nullptr;
  NvDsObjectMeta *obj_meta = nullptr;
  NvDsClassifierMeta *class_meta = nullptr;
  NvDsLabelInfo *label_info = nullptr;
  int frame_id;
  // g_print("Config: %d %d %f %f %d", max_frame_unseen_to_collect, max_frame_track_per_object, min_percentage_best, min_percentage_top, min_time_value_appear);
  // g_mutex_lock (&icdsobjectsummarize->objectsummarize_mutex);
  for (l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next) {
    frame_meta = (NvDsFrameMeta *) (l_frame->data);
    frame_id = frame_meta->frame_num;
    for (l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next) {
      obj_meta = (NvDsObjectMeta *) (l_obj->data);
      // std::cout << obj_meta->obj_label << std::endl;
      for (l_class = obj_meta->classifier_meta_list; l_class != NULL; l_class = l_class->next){
        class_meta = (NvDsClassifierMeta *) (l_class->data);
        int32_t unique_id = class_meta->unique_component_id;
        for (l_label = class_meta->label_info_list; l_label != NULL; l_label = l_label->next){
          label_info = (NvDsLabelInfo *) (l_label->data); 
          cache_objects_tracking[obj_meta->obj_label][obj_meta->object_id][unique_id].list_frame_ids.push_back(frame_meta->frame_num);
          cache_objects_tracking[obj_meta->obj_label][obj_meta->object_id][unique_id].list_object_labels.push_back(label_info->result_label);

          if (cache_objects_tracking[obj_meta->obj_label][obj_meta->object_id][unique_id].list_frame_ids.size() > max_frame_track_per_object){
            auto &frame_ids = cache_objects_tracking[obj_meta->obj_label][obj_meta->object_id][unique_id].list_frame_ids;
            auto &label = cache_objects_tracking[obj_meta->obj_label][obj_meta->object_id][unique_id].list_object_labels;
          
              // Xóa các phần tử phía trước sao cho kích thước của vector bằng max_frame_track_per_object
            
            frame_ids.erase(frame_ids.begin(), frame_ids.begin() + (frame_ids.size() - max_frame_track_per_object));   
            label.erase(label.begin(), label.begin() + (label.size() - max_frame_track_per_object));
          }
        
        // Clear previous summary
        cache_objects_tracking[obj_meta->obj_label][obj_meta->object_id][unique_id].attribute_summarized.clear();     
        for (const std::string& label : cache_objects_tracking[obj_meta->obj_label][obj_meta->object_id][unique_id].list_object_labels) {
          cache_objects_tracking[obj_meta->obj_label][obj_meta->object_id][unique_id].attribute_summarized[label]++;
        }
              std::string vehicle_type = label_info->result_label;
        }
      }
      
    }
  }
  // Kiểm tra và xóa các đối tượng không còn thấy sau nhiều khung hình
  // std::cout << "Frame: " << frame_id << std::endl;
  std::ofstream outfile("Object_summarize");
  outfile << "Frame: " << frame_id << std::endl;
  for (auto& obj_detec : cache_objects_tracking){
    outfile << "Object Category: " << obj_detec.first << std::endl;
    for (auto& obj_entry : obj_detec.second) {
    outfile << std::endl;
    outfile << "  Object_id: " << obj_entry.first << std::endl;
      for (auto& unique_id_entry : obj_entry.second) {
        outfile << "    Object Type: " << unique_id_entry.first << std:: endl;
        auto& meta = unique_id_entry.second;
        gfloat percentage_max = INT_MIN;

        // Tăng số khung hình không thấy đối tượng
        if (!meta.list_frame_ids.empty()) {
            int current_frame = frame_meta->frame_num;
            int last_seen_frame = meta.list_frame_ids.back();
            outfile << "      Labels: ";
            for (auto& labels : meta.list_object_labels)
            {
              outfile << labels << " ";
            }
            outfile << std::endl;
            if (current_frame - last_seen_frame > max_frame_unseen_to_collect) {

              for (auto& label : meta.attribute_summarized){
                gfloat percentage = label.second * 1.0 / meta.list_frame_ids.size();
                if (percentage > min_percentage_top && label.second >= min_time_value_appear)
                {
                  meta.top_attributes.push_back(std::make_pair(label.first, percentage));
                  if (outfile.is_open()) {
                  }
                    outfile << "      Top attribute: " << label.first 
                            << "\n        Count: " << label.second 
                            << "\n        Total: " << meta.list_frame_ids.size() 
                            << "\n        Percentage: " << percentage << std::endl;
                }
                if (percentage > percentage_max){
                  percentage_max = percentage; 
                  meta.best_attributes.first = label.first;
                  meta.best_attributes.second = percentage;
                  
                }
    
              }
              if (meta.best_attributes.second > min_percentage_best && meta.attribute_summarized[meta.best_attributes.first] >= min_time_value_appear){
                outfile << "      Best attribute2: " << meta.best_attributes.first << std::endl;
              }
            } else {
                // Cập nhật số khung hình không thấy
                meta.unseen_frame_count++;
            }
        }

      }
    }
    outfile.close();
  }

  flow_ret = GST_FLOW_OK;

  return flow_ret;
}

/**
 * Boiler plate for registering a plugin and an element.
 */
static gboolean
icdsobjectsummarize_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT(gst_icdsobjectsummarize_debug, "icdsobjectsummarize", 1, "icdsobjectsummarize plugin");
  return gst_element_register(plugin, "icdsobjectsummarize", GST_RANK_PRIMARY, GST_TYPE_ICDSOBJECTSUMMARIZE);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    icdsgst_dsobjectsummarize,
    DESCRIPTION, icdsobjectsummarize_plugin_init, "6.1", LICENSE, BINARY_PACKAGE,
    URL)