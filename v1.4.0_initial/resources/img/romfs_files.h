/**
 * This file contains the list of files for the ROMFS.
 *
 * The files have been converted using...
 * 		file2c -dbcs infile outfile
 */
#include "app_config.h"

#ifndef GUI_DEBUG_DEFAULT_SURFACE_EXTERNAL
#define GUI_DEBUG_DEFAULT_SURFACE_EXTERNAL      0
#endif

#if 1
#if (IMG_INTERNAL_ONLY)
#include "menu_01_card.h"
#include "menu_02_hrm.h"
#include "menu_03_sport.h"
#include "menu_09_setting.h"
#include "menu_04_mijia.h"
#include "menu_08_data.h"
#include "menu_05_weather.h"
#include "m_setting_00_doing.h"     //using as upgrade menu

#include "m_upgrade_00_enter.h"
#include "m_upgrade_01_doing.h"

#include "m_weather_00_sunny.h"
#include "m_weather_01_rainning.h"
#include "m_weather_04_cloud.h"

#include "m_notice_02_info.h"

#include "m_card_00_guangzhou.h"
#include "m_card_01_shenzhen.h"
#include "m_card_02_beijing.h"
#include "m_card_04_shanghai.h"
#include "m_card_03_xiaomi.h"
#include "m_card_05_gate.h"
#include "icon_03_check.h"

#include "m_data_00_step.h"
#include "m_data_01_calorie.h"
#include "m_data_02_km.h"
#include "m_data_03_hrm.h"
#include "m_data_04_sitting.h"
#include "m_data_05_sleeping.h"

#include "g_widget_07_start.h"

#endif


#if (!GUI_DEBUG_DEFAULT_SURFACE_EXTERNAL)
#include "surface_01_red_arron.h"
#endif
//#include "dial_earth.h"
//#include "surface_06_xiaoai.h"
//#include "surface_02_red_circle.h"


//#include "hrm_animate_0.h"
//#include "hrm_animate_1.h"
//#include "hrm_animate_2.h"
//#include "hrm_animate_3.h"

//#include "hrm_animate_4.h"
//#include "hrm_animate_5.h"
//#include "hrm_animate_6.h"
//#include "hrm_animate_7.h"

#else

#if 0
#include "surface_0319_01_xiaoai1.h"
#include "surface_0319_02_xiaoai2.h"
#include "surface_0319_03_xiaoai3.h"
#include "surface_0319_04_mitu.h"
#include "surface_0319_05_tuxing.h"
#include "surface_0319_06_green.h"
#include "surface_0319_07_redcircle.h"
#include "surface_0319_08_colorful.h"
#include "surface_0319_09_multinfo.h"
#include "surface_0319_10_bigfont.h"
#include "surface_0319_11_clock.h"
#endif

#if 0
#include "g_status_00_succ.h"
#include "g_status_01_fail.h"
#include "g_widget_00_enter.h"
#include "g_widget_01_pending.h"
#include "g_widget_03_stop.h"
#include "g_widget_05_restore.h"
#include "g_widget_06_cancel.h"
#include "g_widget_07_start.h"
#include "hrm_detect_01.h"
#include "hrm_detect_02.h"
#include "hrm_detect_03.h"
#include "hrm_detect_04.h"
#include "hrm_detect_05.h"
#include "hrm_detect_06.h"
#include "hrm_detect_07.h"
#include "hrm_detect_08.h"
#include "icon_01_as.h"
#include "icon_02_shoes.h"
#include "icon_03_check.h"
#include "menu_00_home.h"
#include "menu_01_card.h"
#include "menu_02_hrm.h"
#include "menu_03_sport.h"
#include "menu_04_mijia.h"
#include "menu_05_weather.h"
#include "menu_06_clock.h"
#include "menu_07_calendar.h"
#include "menu_08_data.h"
#include "menu_09_setting.h"
#endif

#if 0
#include "m_binding_01_doing.h"
#include "m_card_01_shenzhen.h"
#include "m_card_02_beijing.h"
#include "m_card_03_xiaomi.h"
#include "m_card_05_gate.h"
#include "m_data_00_step.h"
#include "m_data_01_calorie.h"
#include "m_data_03_hrm.h"
#include "m_data_04_sitting.h"
#include "m_data_05_sleeping.h"
#include "m_notice_00_phone.h"
#include "m_notice_01_mail.h"
#include "m_notice_02_info.h"
#include "m_setting_00_doing.h"
#include "m_sport_00_running.h"
#include "m_sport_00_running_s.h"
#include "m_sport_swimming_03.h"
#include "m_status_00_msg.h"
#include "m_status_01_batterylow.h"
#include "m_status_02_charging.h"
#include "m_status_offline.h"
#include "m_upgrade_00_enter.h"
#include "m_upgrade_01_doing.h"
#include "m_weather_00_sunny.h"
#include "m_weather_01_rainning.h"
#include "m_weather_02_overcast.h"
#include "m_weather_03_snow.h"
#include "m_weather_04_cloud.h"
#include "m_weather_09_thunderstorm.h"
#include "m_weather_10_cloud_night.h"
#include "m_weather_10_rsvd.h"
#include "m_weather_10_sunny_night.h"
#include "sport_animate_01.h"
#include "sport_animate_02.h"
#include "sport_animate_03.h"
#include "surface_colorful_font.h"
#include "surface_earth.h"
#include "surface_galaxy.h"
#endif

#if 0
#include "g_scrollbar.h"
#include "g_widget_00_enter0.h"
#include "g_widget_01_pending0.h"
#include "g_widget_03_stop.h"
#include "g_widget_07_start.h"
#include "icon_phone.h"
#include "m_card_00_guangzhou.h"
#include "m_card_04_shanghai.h"
#include "m_card_05_gate.h"
#include "m_card_06_building.h"
#include "m_card_06_gongsi.h"
#include "m_card_07_home.h"
#include "m_msg_04_calendar.h"
#include "m_notice_00_phone.h"
#include "m_notice_douyin.h"
#include "m_notice_qq.h"
#include "m_notice_toutiao.h"
#include "m_notice_wechat.h"
#include "m_notice_weibo.h"
#include "m_notice_zhifubao.h"
#include "m_notice_zhihu.h"
#include "m_relax_00_enter.h"
#include "m_sport_01_bike.h"
#endif

#if 0
#include "m_data_02_km.h"
#include "m_msg_01_milestone.h"
#include "m_msg_02_relax.h"
#include "m_sport_02_jumping.h"
#include "m_status_02_charging2.h"
#endif

#endif