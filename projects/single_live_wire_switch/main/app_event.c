/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>

#include "app_wifi.h"
#include <esp_timer.h>

#include "app_priv.h"

#if 1
#define PHASE_1_DURATION 3600
#define PHASE_1_TIMEOUT 300
#define PHASE_2_TIMEOUT 80
#else
#define PHASE_1_DURATION 300
#define PHASE_1_TIMEOUT 60
#define PHASE_2_TIMEOUT 30

#endif

enum {
	WIFI_CONNECT_PHASE_0,
	WIFI_CONNECT_PHASE_1,
	WIFI_CONNECT_PHASE_2,
	WIFI_CONNECT_PHASE_MAX,
};

typedef struct {
    int err;
    const char *reason;
} wifi_reason_t;

typedef struct {
	int connect_cnt;
	int connect_max;
	int connect_timeout; //seconds
} connect_strategy_context_t;

typedef struct {
	int is_connected;   //network is ok
	int cur_phase;

	esp_timer_handle_t wifi_reconnect_timer;
	connect_strategy_context_t phase[WIFI_CONNECT_PHASE_MAX];
} wifi_custom_connect_strategy_t;

static const char* TAG = "app_event";
static wifi_custom_connect_strategy_t connect_strategy;

static const wifi_reason_t wifi_reason[] =
{
    {0,                                    "other reason"},
    {WIFI_REASON_UNSPECIFIED,              "unspecified"},
    {WIFI_REASON_AUTH_EXPIRE,              "auth expire"},
    {WIFI_REASON_AUTH_LEAVE,               "auth leave"},
    {WIFI_REASON_ASSOC_EXPIRE,             "assoc expire"},
    {WIFI_REASON_ASSOC_TOOMANY,            "assoc too many"},
    {WIFI_REASON_NOT_AUTHED,               "not authed"},
    {WIFI_REASON_NOT_ASSOCED,              "not assoced"},
    {WIFI_REASON_ASSOC_LEAVE,              "assoc leave"},
    {WIFI_REASON_ASSOC_NOT_AUTHED,         "assoc not authed"},
    {WIFI_REASON_BEACON_TIMEOUT,           "beacon timeout"},
    {WIFI_REASON_NO_AP_FOUND,              "no ap found"},
    {WIFI_REASON_AUTH_FAIL,                "auth fail"},
    {WIFI_REASON_ASSOC_FAIL,               "assoc fail"},
    {WIFI_REASON_HANDSHAKE_TIMEOUT,        "hanshake timeout"},
    {WIFI_REASON_DISASSOC_PWRCAP_BAD,      "bad Power Capability, disassoc"},
    {WIFI_REASON_DISASSOC_SUPCHAN_BAD,     "bad Supported Channels, disassoc"},
    {WIFI_REASON_IE_INVALID,               "invalid IE"},
    {WIFI_REASON_MIC_FAILURE,              "MIC failure"},
    {WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT,   "4-way keying handshake timeout"},
    {WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT, "Group key handshake"},
    {WIFI_REASON_IE_IN_4WAY_DIFFERS,       "IE in 4-way differs"},
    {WIFI_REASON_GROUP_CIPHER_INVALID,     "invalid group cipher"},
    {WIFI_REASON_PAIRWISE_CIPHER_INVALID,  "invalid pairwise cipher"},
    {WIFI_REASON_AKMP_INVALID,             "invalid AKMP"},
    {WIFI_REASON_UNSUPP_RSN_IE_VERSION,    "unsupported RSN IE version"},
    {WIFI_REASON_INVALID_RSN_IE_CAP,       "invalid RSN IE capability"},
    {WIFI_REASON_802_1X_AUTH_FAILED,       "802.1x auth failed"},
    {WIFI_REASON_CIPHER_SUITE_REJECTED,    "cipher suite rejected"}
};

static const char* wifi_disconnect_reason_to_str(int err)
{
    for (int i=0; i< sizeof(wifi_reason)/sizeof(wifi_reason[0]); i++){
        if (err == wifi_reason[i].err){
            return wifi_reason[i].reason;
        }
    }
    return wifi_reason[0].reason;
}


static wifi_custom_connect_strategy_t *get_strategy_ref(void)
{
	return &connect_strategy;
}


static void wifi_reconnect_timer_callback(void* arg)
{
	//ESP_LOGI(TAG, "---------------wifi_reconnect_timer_callback---------------------");
	esp_wifi_connect();
}

static void wifi_custom_connect_strategy_init(wifi_custom_connect_strategy_t *p)
{
	const esp_timer_create_args_t wifi_reconnect_timer_args = {
            .callback = &wifi_reconnect_timer_callback,
            .name = "wifi_reconnect"
    };
    
	p->is_connected = 0;
	p->cur_phase = WIFI_CONNECT_PHASE_0;

	ESP_ERROR_CHECK(esp_timer_create(&wifi_reconnect_timer_args, &(p->wifi_reconnect_timer)));
	//phase 0
	p->phase[WIFI_CONNECT_PHASE_0].connect_cnt = 0;
	p->phase[WIFI_CONNECT_PHASE_0].connect_timeout = 0;
	p->phase[WIFI_CONNECT_PHASE_0].connect_max = 3;

	//phase 1
	p->phase[WIFI_CONNECT_PHASE_1].connect_cnt = 0;
	p->phase[WIFI_CONNECT_PHASE_1].connect_timeout = PHASE_1_TIMEOUT;
	p->phase[WIFI_CONNECT_PHASE_1].connect_max = (PHASE_1_DURATION / PHASE_1_TIMEOUT);

	//phase 2
	p->phase[WIFI_CONNECT_PHASE_2].connect_cnt = 0;
	p->phase[WIFI_CONNECT_PHASE_2].connect_timeout = PHASE_2_TIMEOUT;
	p->phase[WIFI_CONNECT_PHASE_2].connect_max = 3;
	
}


static void wifi_custom_connect_fail(wifi_custom_connect_strategy_t *p)
{
	p->is_connected = 0;

	switch (p->cur_phase) {
		case WIFI_CONNECT_PHASE_0:
			{
				p->phase[p->cur_phase].connect_cnt += 1;
				if (p->phase[p->cur_phase].connect_cnt >= p->phase[p->cur_phase].connect_max) {
					//reset
					p->phase[p->cur_phase].connect_cnt = 0;
					//move to phase 1
					p->cur_phase = WIFI_CONNECT_PHASE_1;
				}
			}
			break;
		case WIFI_CONNECT_PHASE_1:
			{
				p->phase[p->cur_phase].connect_cnt += 1;
				if (p->phase[p->cur_phase].connect_cnt >= p->phase[p->cur_phase].connect_max) {
					//reset
					p->phase[p->cur_phase].connect_cnt = 0;
					//move to phase 2
					p->cur_phase = WIFI_CONNECT_PHASE_2;
				}
			}
			break;
		case WIFI_CONNECT_PHASE_2:
			{
				p->phase[p->cur_phase].connect_cnt += 1;
				if (p->phase[p->cur_phase].connect_cnt >= p->phase[p->cur_phase].connect_max) {
					//reset
					p->phase[p->cur_phase].connect_cnt = 0;
					//move to phase 1
					p->cur_phase = WIFI_CONNECT_PHASE_1;
				}
			}
			break;
	}
	//
	app_rainmaker_reconnect_timer_stop();
	//start timer
	ESP_LOGI(TAG, "wifi reconnect in %d seconds",p->phase[p->cur_phase].connect_timeout);
	esp_timer_start_once(p->wifi_reconnect_timer, p->phase[p->cur_phase].connect_timeout * 1000 * 1000);
}

static void wifi_custom_connect_success(wifi_custom_connect_strategy_t *p)
{
	p->is_connected = 1;
	//reset cnts
	p->cur_phase = WIFI_CONNECT_PHASE_0;
	for (int i = WIFI_CONNECT_PHASE_0;i < WIFI_CONNECT_PHASE_MAX; i++) {
		p->phase[i].connect_cnt = 0;
	}

	esp_timer_stop(p->wifi_reconnect_timer);
	app_rainmaker_reconnect_timer_start();
}


static void handle_wifi_events(int32_t event_id, void* event_data)
{
    switch (event_id) {
        case WIFI_EVENT_STA_START:
        {
            int8_t power = 0;
            esp_wifi_set_max_tx_power(44);
            esp_wifi_get_max_tx_power(&power);
            ESP_LOGW(TAG, "max_tx_power: %f", (float)power * 0.25);
            esp_wifi_connect();
        }
        break;
        case WIFI_EVENT_STA_DISCONNECTED:
        {
			wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
			ESP_LOGI(TAG, "Wifi Disconnected :%s",wifi_disconnect_reason_to_str(disconnected->reason));
            wifi_custom_connect_fail(get_strategy_ref());
        }
        break;
		case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(TAG, "Wifi Connected");
        }
        break;
        default:
        ESP_LOGW(TAG, "wifi unhandled: event %d", event_id);
        break;
    }
}

static void handle_ip_events(int32_t event_id, void* event_data)
{
    switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
            /* Signal main application to continue execution */
			wifi_custom_connect_success(get_strategy_ref());
        }
        break;
        default:
        ESP_LOGW(TAG, "ip unhandled: event %d", event_id);
        break;
    }
}

/* Event handler for catching system events */
static void
network_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        handle_wifi_events(event_id,event_data);
    } else if (event_base == IP_EVENT) {
        handle_ip_events(event_id,event_data);
    }
}


void app_event_register_init(void)
{
	wifi_custom_connect_strategy_init(get_strategy_ref());
	esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &network_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &network_event_handler, NULL);
}


int wifi_is_connected(void)
{
	return get_strategy_ref()->is_connected;
}

