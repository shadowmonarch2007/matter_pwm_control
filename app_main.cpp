#include "esp_matter.h"
#include "nvs_flash.h"
#include "app_priv.h"
#include "app_driver.h"
#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <platform/CHIPDeviceLayer.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>

using namespace esp_matter;
using namespace chip::DeviceLayer;

static const char *TAG = "app_main";

static uint16_t pwm_endpoint_ids[PWM_NUM_CHANNELS] = {0, 0, 0};

static bool endpoint_on[PWM_NUM_CHANNELS] = {false, false, false};
static uint8_t endpoint_level[PWM_NUM_CHANNELS] = {254, 254, 254};

static void print_pairing_info()
{
    chip::SetupPayload payload;
    CHIP_ERROR err = CHIP_NO_ERROR;

    CommissionableDataProvider *commissionable_data_provider = GetCommissionableDataProvider();
    if (commissionable_data_provider == nullptr) {
        ESP_LOGE(TAG, "CommissionableDataProvider is null");
        return;
    }

    uint32_t setupPasscode;
    err = commissionable_data_provider->GetSetupPasscode(setupPasscode);
    if (err != CHIP_NO_ERROR) {
        ESP_LOGE(TAG, "Failed to get setup passcode: %s", chip::ErrorStr(err));
        return;
    }
    payload.setUpPINCode = setupPasscode;

    uint16_t discriminator;
    err = commissionable_data_provider->GetSetupDiscriminator(discriminator);
    if (err != CHIP_NO_ERROR) {
        ESP_LOGE(TAG, "Failed to get setup discriminator: %s", chip::ErrorStr(err));
        return;
    }
    payload.discriminator.SetLongValue(discriminator);

    DeviceInstanceInfoProvider *device_info_provider = GetDeviceInstanceInfoProvider();
    if (device_info_provider) {
        uint16_t vendorId, productId;
        if (device_info_provider->GetVendorId(vendorId) == CHIP_NO_ERROR) {
            payload.vendorID = vendorId;
        }
        if (device_info_provider->GetProductId(productId) == CHIP_NO_ERROR) {
            payload.productID = productId;
        }
    }

    payload.rendezvousInformation.SetValue(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kOnNetwork));

    std::string manual_code;
    chip::ManualSetupPayloadGenerator manualGenerator(payload);
    if (manualGenerator.payloadDecimalStringRepresentation(manual_code) == CHIP_NO_ERROR) {
        ESP_LOGI(TAG, "Manual pairing code: %s", manual_code.c_str());
    } else {
        ESP_LOGE(TAG, "Failed to generate manual code");
    }

    std::string qr_code;
    chip::QRCodeSetupPayloadGenerator qrGenerator(payload);
    if (qrGenerator.payloadBase38Representation(qr_code) == CHIP_NO_ERROR) {
        ESP_LOGI(TAG, "QR Code: MT:%s", qr_code.c_str());
    } else {
        ESP_LOGE(TAG, "Failed to generate QR code");
    }
}

static int get_pwm_channel_for_endpoint(uint16_t endpoint_id)
{
    for (int i = 0; i < PWM_NUM_CHANNELS; i++) {
        if (pwm_endpoint_ids[i] == endpoint_id) {
            return i;
        }
    }
    return -1;
}

static esp_err_t app_event_cb(attribute::callback_type_t type,
                              uint16_t endpoint_id,
                              uint32_t cluster_id,
                              uint32_t attribute_id,
                              esp_matter_attr_val_t *val,
                              void *priv_data)
{
    if (type == attribute::callback_type_t::POST_UPDATE) {
        int channel = get_pwm_channel_for_endpoint(endpoint_id);
        
        if (channel < 0) {
            return ESP_OK;
        }

        if (cluster_id == chip::app::Clusters::OnOff::Id &&
            attribute_id == chip::app::Clusters::OnOff::Attributes::OnOff::Id) {
            
            endpoint_on[channel] = val->val.b;
            
            ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            ESP_LOGI(TAG, " PWM Channel %d (GPIO %d): POWER %s", 
                     channel, 
                     channel == 0 ? PWM_CHANNEL_0_GPIO : 
                     channel == 1 ? PWM_CHANNEL_1_GPIO : PWM_CHANNEL_2_GPIO,
                     endpoint_on[channel] ? "ON" : "OFF");
            ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            
            app_driver_set_channel_power(channel, endpoint_on[channel], endpoint_level[channel]);
            
        } else if (cluster_id == chip::app::Clusters::LevelControl::Id &&
                   attribute_id == chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Id) {
            
            endpoint_level[channel] = val->val.u8;
            
            ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            ESP_LOGI(TAG, " PWM Channel %d (GPIO %d): LEVEL %d/254 (%.1f%%)", 
                     channel,
                     channel == 0 ? PWM_CHANNEL_0_GPIO : 
                     channel == 1 ? PWM_CHANNEL_1_GPIO : PWM_CHANNEL_2_GPIO,
                     endpoint_level[channel], 
                     (endpoint_level[channel] / 254.0f) * 100.0f);
            ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            
            app_driver_set_channel_power(channel, endpoint_on[channel], endpoint_level[channel]);
        }
    }
    return ESP_OK;
}

extern "C" void app_main()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║ ESP32-C6 Matter 3-Channel PWM Controller          ║");
    ESP_LOGI(TAG, "╠═══════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ PWM Configuration:                                ║");
    ESP_LOGI(TAG, "║   Channel 0 -> GPIO 4  (Dimmable Light 1)         ║");
    ESP_LOGI(TAG, "║   Channel 1 -> GPIO 5  (Dimmable Light 2)         ║");
    ESP_LOGI(TAG, "║   Channel 2 -> GPIO 6  (Dimmable Light 3)         ║");
    ESP_LOGI(TAG, "║   RGB LED   -> GPIO 8  (Status indicator)         ║");
    ESP_LOGI(TAG, "╠═══════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ Control via Matter:                               ║");
    ESP_LOGI(TAG, "║   ON/OFF   -> PWM 0%% / Current Level             ║");
    ESP_LOGI(TAG, "║   Level    -> PWM duty cycle (0-100%%)            ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    app_driver_init();
    
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_event_cb, nullptr, nullptr);

    const char* channel_names[] = {"PWM GPIO4", "PWM GPIO5", "PWM GPIO6"};
    
    for (int i = 0; i < PWM_NUM_CHANNELS; i++) {
        endpoint::dimmable_light::config_t light_cfg = {};
        endpoint_t *ep = endpoint::dimmable_light::create(node, &light_cfg, ENDPOINT_FLAG_NONE, nullptr);
        
        if (ep) {
            pwm_endpoint_ids[i] = endpoint::get_id(ep);
            ESP_LOGI(TAG, "✓ Created endpoint %d for %s", pwm_endpoint_ids[i], channel_names[i]);
        } else {
            ESP_LOGE(TAG, "✗ Failed to create endpoint for %s", channel_names[i]);
        }
    }
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║ Matter 3-Channel PWM Controller Ready             ║");
    ESP_LOGI(TAG, "║ Use Google Home or Apple Home to commission       ║");
    ESP_LOGI(TAG, "║ 3 dimmable lights will appear for control         ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    esp_matter::start(nullptr, (intptr_t)NULL);

    ESP_LOGI(TAG, "Waiting 5 seconds for BLE advertising to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    print_pairing_info();
}
