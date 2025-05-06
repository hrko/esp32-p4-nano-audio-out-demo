// SPDX-License-Identifier: MIT
#include <math.h>
#include <stdio.h>

#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "p4nano_audio.h"
#include "sdkconfig.h"

static const char *TAG = "audio_out_demo";
static const int SAMPLE_RATE = 48000;	 // Sample rate in Hz
static const int BIT_DEPTH = 16;			 // Bit depth
static const int CHANNEL_COUNT = 1;		 // Number of channels (1 for mono, 2 for stereo)

typedef struct {
	uint8_t *buffer;
	size_t size;
} audio_buffer_t;

static QueueHandle_t audio_queue;

typedef struct {
	// Type and channel must match the I2S configuration
	int16_t left;
	// int16_t right;
} i2s_sample_t;

static void sine_wave_generator(void *args) {
	const int sample_rate = SAMPLE_RATE;	// Sample rate in Hz
	const int frequency = 1000;						// Frequency of the sine wave
	const int amplitude = 10000;					// Amplitude of the sine wave (max 32767)
	const int buffer_count = 2;						// Number of buffers
	const int buffer_size = 256;					// Number of samples per buffer

	// Allocate buffers
	uint8_t *buffers[buffer_count];
	for (int i = 0; i < buffer_count; i++) {
		buffers[i] = malloc(buffer_size * sizeof(i2s_sample_t));
		if (!buffers[i]) {
			ESP_LOGE(TAG, "[generator] Failed to allocate memory for buffer %d", i);
			abort();
		}
	}

	int buffer_index = 0;
	int sample_index = 0;

	while (1) {
		i2s_sample_t *buf = (i2s_sample_t *)buffers[buffer_index];
		for (int i = 0; i < buffer_size; i++) {
			const float t = (float)sample_index / (float)sample_rate;
			buf[i].left = (int16_t)(amplitude * sinf(2.0f * M_PI * frequency * t));
			// buf[i].right = 0;	 // Mono output, right channel is silent
			sample_index = (sample_index + 1) % sample_rate;
		}

		audio_buffer_t audio_buf = {
				.buffer = buffers[buffer_index],
				.size = buffer_size * sizeof(i2s_sample_t),
		};

		if (xQueueSend(audio_queue, &audio_buf, portMAX_DELAY) != pdPASS) {
			ESP_LOGE(TAG, "[generator] Failed to send buffer to queue");
		}

		buffer_index = (buffer_index + 1) % buffer_count;
	}
}

static void i2s_writer(void *args) {
	audio_buffer_t audio_buf;
	i2s_chan_handle_t tx_handle = NULL;
	bsp_audio_get_i2s_handle(&tx_handle, NULL);
	if (!tx_handle) {
		ESP_LOGE(TAG, "[writer] Failed to get I2S handle");
		abort();
	}

	while (1) {
		if (xQueueReceive(audio_queue, &audio_buf, portMAX_DELAY) == pdPASS) {
			size_t bytes_written = 0;
			esp_err_t ret = i2s_channel_write(tx_handle, audio_buf.buffer, audio_buf.size, &bytes_written, portMAX_DELAY);
			if (ret != ESP_OK) {
				ESP_LOGE(TAG, "[writer] i2s write failed");
				abort();
			}
			if (bytes_written != audio_buf.size) {
				ESP_LOGW(TAG, "[writer] %d bytes should be written but only %d bytes are written", audio_buf.size, bytes_written);
			}
		}
	}
}

void app_main(void) {
	esp_err_t ret = ESP_OK;

	// I2S initialization
	i2s_std_config_t i2s_config = bsp_get_i2s_duplex_config(SAMPLE_RATE, BIT_DEPTH, CHANNEL_COUNT);
	ret = bsp_audio_init(&i2s_config);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize I2S");
		abort();
	}

	// Initialize audio device
	esp_codec_dev_handle_t speaker_handle = bsp_audio_codec_speaker_init();
	if (!speaker_handle) {
		ESP_LOGE(TAG, "Failed to initialize speaker codec");
		abort();
	}
	// Set volume
	ret = esp_codec_dev_set_out_vol(speaker_handle, 60);
	if (ret != ESP_CODEC_DEV_OK) {
		ESP_LOGE(TAG, "Failed to set speaker volume");
	}
	// Open device
	esp_codec_dev_sample_info_t fs = {
			.sample_rate = SAMPLE_RATE,
			.bits_per_sample = BIT_DEPTH,
			.channel = CHANNEL_COUNT,
	};
	ret = esp_codec_dev_open(speaker_handle, &fs);
	if (ret != ESP_CODEC_DEV_OK) {
		ESP_LOGE(TAG, "Failed to open speaker codec");
		abort();
	}

	// Create FreeRTOS queue
	audio_queue = xQueueCreate(2, sizeof(audio_buffer_t));
	if (!audio_queue) {
		ESP_LOGE(TAG, "Failed to create audio queue");
		abort();
	}

	// Create tasks
	xTaskCreate(sine_wave_generator, "sine_wave_generator", 4096, NULL, 5, NULL);
	xTaskCreate(i2s_writer, "i2s_writer", 4096, NULL, 5, NULL);
}