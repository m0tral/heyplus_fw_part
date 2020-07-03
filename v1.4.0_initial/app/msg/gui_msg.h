#ifndef __GUI_MSG_H_

#define __GUI_MSG_H_

#include "msg_management_service.h"
#include "window_management_service.h"


#define DELETE_ALL_MSG_BUTTON_TOP               1
#define DELETE_ALL_MSG_BUTTON_BOTTOM            2

#define DELETE_ALL_MSG_BUTTON_POSITION          DELETE_ALL_MSG_BUTTON_BOTTOM




#define MSG_CONTENT_FONT_HEIGHT             22
#define MSG_TITLE_FONT_HEIGHT               22

#define DROP_START_COVER_OFFSET             240

#if 0
#define APP_WEIXIN                          "wx"
#define APP_QQ                              "qq"
#define APP_WEIBO                           "weibo"
#define APP_TAOBAO                          "taobao"
#define APP_TOUTIAO                         "toutiao"
#define APP_NETEASE                         "netease"
#define APP_ZHIHU                           "zhihu"
#define APP_ZHIFUBAO                        "alipay"
#define APP_DOUYIN                          "douyin"
#define APP_DINGDING                        "dingding"
#define APP_MIJIA                           "mijia"
#define APP_MOMO                            "momo"
#else

#define APP_TELE                            "system.phone"
#define APP_SMS                             "system.sms"
#define APP_WEIXIN                          "app.wx"
#define APP_QQ                              "app.qq"
#define APP_ZHIFUBAO                        "app.alipay"
#define APP_WEIBO                           "app.weibo"
#define APP_TAOBAO                          "app.taobao"
#define APP_TOUTIAO                         "app.toutiao"
#define APP_NETEASE                         "app.netease.news"
#define APP_ZHIHU                           "app.zhihu"
#define APP_DOUYIN                          "app.douyin"
#define APP_DINGDING                        "app.dingding"
#define APP_MIJIA                           "app.mijia"
#define APP_MOMO                            "app.momo"
#define APP_TIM                             "app.tim"

#define APP_WHATSAPP                        "app.whatsapp"
#define APP_TELEGRAM                        "app.telegram"
#define APP_VIBER                        	"app.viber"
#define APP_FACEBOOK                        "app.facebook"
#define APP_VKONTAKTE	                    "app.vkontakte"
#define APP_TWITTER                       	"app.twitter"
#define APP_GMAIL                       	"app.gmail"
#define APP_YANDEXMAIL                      "app.yandexmail"
#define APP_INSTAGRAM                       "app.instagram"
#define APP_4PDA                            "app.4pda"
#define APP_SKYPE                           "app.skype"

#define APP_OTHERS                          "Others"

#endif

#define APP_TELE_IMG                            "m_notice_00_phone.bmp"
#define APP_SMS_IMG                             "m_notice_02_info.bmp"
#define APP_WEIXIN_IMG                          "m_notice_wechat.bmp"
#define APP_QQ_IMG                              "m_notice_qq.bmp"
#define APP_WEIBO_IMG                           "m_notice_weibo.bmp"
#define APP_TAOBAO_IMG                          "g_notification.bmp"
#define APP_TOUTIAO_IMG                         "m_notice_toutiao.bmp"
#define APP_NETEASE_IMG                         "m_notice_02_info.bmp"
#define APP_ZHIHU_IMG                           "m_notice_zhihu.bmp"
#define APP_ZHIFUBAO_IMG                        "m_notice_zhifubao.bmp"
#define APP_DOUYIN_IMG                          "m_notice_douyin.bmp"
#define APP_DINGDING_IMG                        "m_notice_dingding.bmp"
#define APP_MIJIA_IMG                           "m_notice_mijia.bmp"
#define APP_MOMO_IMG                            "m_notice_momo.bmp"

#define APP_WHATSAPP_IMG                        "m_notice_whatsapp.bmp"
#define APP_TELEGRAM_IMG                        "m_notice_telegram.bmp"
#define APP_VIBER_IMG                        	"m_notice_viber.bmp"
#define APP_FACEBOOK_IMG                        "m_notice_facebook.bmp"
#define APP_VKONTAKTE_IMG	                    "m_notice_vkontakte.bmp"
#define APP_TWITTER_IMG                       	"m_notice_twitter.bmp"
#define APP_GMAIL_IMG                       	"m_notice_gmail.bmp"
#define APP_YANDEXMAIL_IMG                      "m_notice_yandexmail.bmp"
#define APP_INSTAGRAM_IMG                       "m_notice_instagram.bmp"
#define APP_4PDA_IMG                            "m_notice_4pda.bmp"
#define APP_SKYPE_IMG                           "m_notice_skype.bmp"

#define APP_DEFAULT_IMG                         "g_notification.bmp"

#define TELE_CONTENT_STR                        "Пропущенный вызов"

#define TITLE_COLOR                             0x989898
#define ON_CALL_STR_COLOR                       0x80d015

#define ON_CALLING                              0xAA

#define COMPARE_APP_ID(appid, key)              ((strstr(appid, key) == NULL)?(0):(1))

#define TITLE_MAX_LEN                           5

#define CONTENT_MAX_LINE                        6


#define SLIDE_TYPE_TRANSLATION                  1
#define SLIDE_TYPE_COVER                        0


#define DROP_SHOW_STATUS_GET                    (1)
#define DROP_SHOW_STATUS_SAVE                   (1<<1)
#define DROP_SHOW_STATUS_SHOW                   (1<<2)
#define DROP_SHOW_STATUS_SHOW_SAVE              (DROP_SHOW_STATUS_SAVE | DROP_SHOW_STATUS_SHOW )

#define MSG_TITLE_MAX_LEN                       105

#define MSG_FRIST_SCREEN_LEN                    (6*120)

#define MSG_DEFAULT_SCREEN_LEN                  (8*120)

#define MSG_DISABLE_MOTAR                       0xF0001


extern activity_t msg_activity ;
extern activity_t drop_activity ;



#endif

