// Handwriting Recogniser with ToF Sensor & Edge Impulse (TFT Display Ver.)
// Adapted from Edge Impulse Static Buffer Example
// Author: Jonathan Tan
// Feb 2021
// Written for Seeed Wio Terminal


#include <wio_terminal_handwriting_recogniser_inference.h>
#include "Seeed_vl53l0x.h"
#include "TFT_eSPI.h"
Seeed_vl53l0x VL53L0X;
TFT_eSPI tft;

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define SERIAL SerialUSB
#else
    #define SERIAL Serial
#endif

static float features[25];


int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}


void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    Status = VL53L0X.VL53L0X_common_init();
    if (VL53L0X_ERROR_NONE != Status) {
        SERIAL.println("start vl53l0x mesurement failed!");
        VL53L0X.print_pal_error(Status);
        while (1);
    }

    VL53L0X.VL53L0X_high_accuracy_ranging_init();

    if (VL53L0X_ERROR_NONE != Status) {
        SERIAL.println("start vl53l0x mesurement failed!");
        VL53L0X.print_pal_error(Status);
        while (1);
    }
    
    tft.begin();
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(3);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(320);
    tft.drawString("Wio Terminal Handwriting Recogniser!", 160, 120);
    delay(1000);
    tft.setTextSize(2);
    tft.fillScreen(TFT_BLACK);
}


void loop() {
    tft.fillScreen(TFT_BLACK);
    tft.drawString(F("Perform gesture in 3"), 160, 120);
    delay(1000);
    tft.drawString(F("Perform gesture in 3 2"), 160, 120);
    delay(1000);
    tft.drawString(F("Perform gesture in 3 2 1"), 160, 120);
    delay(1000);
    tft.drawString(F("Start!"), 160, 120);
    
    for (int i = 0; i<25; i++) {
        VL53L0X_RangingMeasurementData_t RangingMeasurementData;
        VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    
        memset(&RangingMeasurementData, 0, sizeof(VL53L0X_RangingMeasurementData_t));
        Status = VL53L0X.PerformSingleRangingMeasurement(&RangingMeasurementData);
        if (VL53L0X_ERROR_NONE == Status) {
            if (RangingMeasurementData.RangeMilliMeter >= 2000) {
            } else {
                features[i] = RangingMeasurementData.RangeMilliMeter;
            }
        } else {
            tft.drawString("Measurement failed !! Status code =" + Status, 0, 0);
            delay(1000);
            tft.fillScreen(TFT_BLACK);
        }
        delay(200);
    }
    tft.drawString(F("End!"), 160, 120);
    delay(1000);

    ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

    if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        ei_printf("The size of your 'features' array is not correct. Expected %lu items, but had %lu\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
        delay(1000);
        return;
    }

    ei_impulse_result_t result = { 0 };

    // the features are stored into flash, and we don't want to load everything into RAM
    signal_t features_signal;
    features_signal.total_length = sizeof(features) / sizeof(features[0]);
    features_signal.get_data = &raw_feature_get_data;

    // invoke the impulse
    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
    ei_printf("run_classifier returned: %d\n", res);

    if (res != 0) return;

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    ei_printf("[");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("%.5f", result.classification[ix].value);
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf(", ");
#else
        if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
            ei_printf(", ");
        }
#endif
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("%.3f", result.anomaly);
#endif
    ei_printf("]\n");

    // human-readable predictions
    String output = "Null";
    float confidence = 0;
    
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result.classification[ix].value > confidence) {
            ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
            output = result.classification[ix].label;
            confidence = result.classification[ix].value;
        }
    }

    tft.drawString(output + ", " + confidence, 160, 120);
    
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

    delay(1000);
}

/**
 * @brief      Printf function uses vsnprintf and output using Arduino Serial
 *
 * @param[in]  format     Variable argument list
 */
void ei_printf(const char *format, ...) {
    static char print_buf[1024] = { 0 };

    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);

    if (r > 0) {
        Serial.write(print_buf);
    }
}
