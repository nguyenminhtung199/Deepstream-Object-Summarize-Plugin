#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "icdsobjectsummarize_property_parser.h"

GST_DEBUG_CATEGORY (ICDSOBJECTSUMMARIZE_CFG_PARSER_CAT);

#define PARSE_ERROR(details_fmt,...) \
  G_STMT_START { \
    GST_CAT_ERROR (ICDSOBJECTSUMMARIZE_CFG_PARSER_CAT, \
        "Failed to parse config file %s: " details_fmt, \
        cfg_file_path, ##__VA_ARGS__); \
    GST_ELEMENT_ERROR (icdsobjectsummarize, LIBRARY, SETTINGS, \
        ("Failed to parse config file:%s", cfg_file_path), \
        (details_fmt, ##__VA_ARGS__)); \
    goto done; \
  } G_STMT_END

#define CHECK_IF_PRESENT(error, custom_err) \
  G_STMT_START { \
    if (error && error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) { \
      std::string errvalue = "Error while setting property, in group ";  \
      errvalue.append(custom_err); \
      PARSE_ERROR ("%s %s", errvalue.c_str(), error->message); \
    } \
  } G_STMT_END

#define CHECK_ERROR(error, custom_err) \
  G_STMT_START { \
    if (error) { \
      std::string errvalue = "Error while setting property, in group ";  \
      errvalue.append(custom_err); \
      PARSE_ERROR ("%s %s", errvalue.c_str(), error->message); \
    } \
  } G_STMT_END

#define CHECK_INT_VALUE_NON_NEGATIVE(prop_name,value, group) \
  G_STMT_START { \
    if ((gint) value < 0) { \
      PARSE_ERROR ("Integer property '%s' in group '%s' can have value >=0", prop_name, group); \
    } \
  } G_STMT_END

#define CHECK_FLOAT_VALUE_NON_NEGATIVE(prop_name,value, group) \
  G_STMT_START { \
    if ((gfloat) value < 0) { \
      PARSE_ERROR ("Float property '%s' in group '%s' can have value >=0", prop_name, group); \
    } \
  } G_STMT_END
  
#define GET_BOOLEAN_PROPERTY(group, property, field) {\
  field = g_key_file_get_boolean(key_file, group, property, &error); \
  CHECK_ERROR(error, group); \
}

#define GET_UINT_PROPERTY(group, property, field) {\
  field = g_key_file_get_integer(key_file, group, property, &error); \
  CHECK_ERROR(error, group); \
  CHECK_INT_VALUE_NON_NEGATIVE(property, field, group);\
}

#define GET_UDOUBLE_PROPERTY(group, property, field) {\
  field = g_key_file_get_double(key_file, group, property, &error); \
  CHECK_ERROR(error, group); \
  CHECK_FLOAT_VALUE_NON_NEGATIVE(property, field, group);\
}

#define ICDSOBJECTSUMMARIZE_PROPERTY "property"
#define ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MAX_FRAME_TRACK "max_frame_track_per_object"
#define ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MAX_FRAME_UNSEEN "max_frame_unseen_to_collect"
#define ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_BEST "min_percentage_best"
#define ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_TOP "min_percentage_top"
#define ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_TIME_APPEAR "min_time_value_appear"


static gboolean
icdsobjectsummarize_parse_format_output(GstIcDsObjectSummarize * icdsobjectsummarize, gchar * cfg_file_path, GKeyFile * key_file, gchar * group);

static gboolean
icdsobjectsummarize_parse_property_group(GstIcDsObjectSummarize * icdsobjectsummarize, gchar * cfg_file_path, GKeyFile * key_file)
{
  g_autoptr (GError) error = nullptr;
  gboolean ret = FALSE;
  ConfigInfo *config_info = (icdsobjectsummarize->config_info);

  GET_UINT_PROPERTY(ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MAX_FRAME_TRACK, icdsobjectsummarize->configuration_max_frame_track_per_object)
  GET_UINT_PROPERTY(ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MAX_FRAME_UNSEEN, icdsobjectsummarize->configuration_max_frame_unseen_to_collect)
  GET_UDOUBLE_PROPERTY(ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_BEST, icdsobjectsummarize->configuration_min_percentage_best)
  GET_UDOUBLE_PROPERTY(ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_TOP, icdsobjectsummarize->configuration_min_percentage_top)
  GET_UINT_PROPERTY(ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_TIME_APPEAR, icdsobjectsummarize->configuration_min_time_value_appear)

  CHECK_IF_PRESENT (error, ICDSOBJECTSUMMARIZE_PROPERTY);

  if (error) {
    g_error_free (error);
    error = nullptr;
  }

  GST_CAT_INFO (ICDSOBJECTSUMMARIZE_CFG_PARSER_CAT,
      "Parsed %s=%d, %s=%d, %s=%f, %s=%f, %s=%d in group '%s'\n", 
      ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MAX_FRAME_TRACK, icdsobjectsummarize->configuration_max_frame_track_per_object, 
      ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MAX_FRAME_UNSEEN, icdsobjectsummarize->configuration_max_frame_unseen_to_collect, 
      ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_BEST, icdsobjectsummarize->configuration_min_percentage_best,
      ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_TOP, icdsobjectsummarize->configuration_min_percentage_top,
      ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_TIME_APPEAR, icdsobjectsummarize->configuration_min_time_value_appear, 
      ICDSOBJECTSUMMARIZE_PROPERTY);

  config_info->max_frame_track_per_object = g_key_file_get_integer(key_file, ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MAX_FRAME_TRACK, &error);
  config_info->max_frame_unseen_to_collect = g_key_file_get_integer(key_file, ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MAX_FRAME_UNSEEN, &error);
  config_info->min_percentage_best = g_key_file_get_double(key_file, ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_BEST, &error);
  config_info->min_percentage_top = g_key_file_get_double(key_file, ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_TOP, &error);
  config_info->min_time_value_appear = g_key_file_get_integer(key_file, ICDSOBJECTSUMMARIZE_PROPERTY, ICDSOBJECTSUMMARIZE_PROPERTY_CONFIG_MIN_TIME_APPEAR, &error);

  ret = TRUE;

done:
  return ret;
}

static gboolean
icdsobjectsummarize_parse_format_output (GstIcDsObjectSummarize * icdsobjectsummarize,
    gchar * cfg_file_path, GKeyFile * key_file, gchar * group)
{
  g_autoptr (GError) error = nullptr;
  g_auto (GStrv) keys = nullptr;
  GStrv key = nullptr;
  gchar *template_check = nullptr;
  std::vector <std::string> template_check_vec;
  ConfigInfo *config_info = (icdsobjectsummarize->config_info);

  keys = g_key_file_get_keys (key_file, group, nullptr, &error);
  CHECK_ERROR (error, group);

  for (key = keys; *key; key++){
    template_check = g_key_file_get_value(key_file, group, *key, &error);
    if (template_check == nullptr) {
        CHECK_ERROR (error, group);
    }
    std::string std_string_text(g_strdup(template_check));
    template_check_vec.push_back(std_string_text);

    g_free(template_check);
    template_check = nullptr;
  }

done:
  g_free(template_check);
  return TRUE;
}

gboolean
icdsobjectsummarize_parse_config_file (GstIcDsObjectSummarize * icdsobjectsummarize, gchar * cfg_file_path)
{
  g_autoptr (GError) error = nullptr;
  gboolean ret = FALSE;
  g_auto (GStrv) groups = nullptr;
  gboolean property_present = FALSE;
  GStrv group;
  g_autoptr (GKeyFile) cfg_file = g_key_file_new ();

  if (!ICDSOBJECTSUMMARIZE_CFG_PARSER_CAT) {
    GstDebugLevel level;
    GST_DEBUG_CATEGORY_INIT(ICDSOBJECTSUMMARIZE_CFG_PARSER_CAT, "icdsobjectsummarize_cfg_parser", 0, NULL);
    level = gst_debug_category_get_threshold(ICDSOBJECTSUMMARIZE_CFG_PARSER_CAT);
    if (level < GST_LEVEL_ERROR)
      gst_debug_category_set_threshold (ICDSOBJECTSUMMARIZE_CFG_PARSER_CAT, GST_LEVEL_ERROR);
  }

  if (!g_key_file_load_from_file(cfg_file, cfg_file_path, G_KEY_FILE_NONE, &error)){
    PARSE_ERROR ("%s", error->message);
  }
  
  if (!g_key_file_has_group(cfg_file, ICDSOBJECTSUMMARIZE_PROPERTY)) {
    PARSE_ERROR ("Group 'property' not specified");
  }

  g_key_file_set_list_separator(cfg_file, ';');
  groups = g_key_file_get_groups(cfg_file, nullptr);

  for (group = groups; *group; group++) {
    GST_CAT_INFO (ICDSOBJECTSUMMARIZE_CFG_PARSER_CAT, "Group found %s \n", *group);
    if (!strcmp (*group, ICDSOBJECTSUMMARIZE_PROPERTY)) {
      property_present = icdsobjectsummarize_parse_property_group(icdsobjectsummarize, cfg_file_path, cfg_file);
    }  else {
      g_print ("ICDSOBJECTSUMMARIZE_CFG_PARSER: Group '%s' ignored\n", *group);
    }
  }

  if (FALSE == property_present) {
    ret = FALSE;
  } else {
    ret = TRUE;
  }

done:
  return ret;
}