

extern "C" {
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
}

static constexpr gpio_num_t ONEWIRE_PIN = GPIO_NUM_4;
#define MAX_SENSORS 5
#define POLL_INTERVAL_MS 30000  // 30 seconds
#define DS18B20_FAMILY_CODE 0x28

typedef struct {
    uint8_t rom[8];          // 64-bit ROM code
    float temperature;       // Last temperature reading
    uint8_t valid;           // Is this sensor valid/initialized?
    char name[16];           // Optional name for display
} sensor_t;

typedef struct {
    gpio_num_t pin;
    sensor_t sensors[MAX_SENSORS];
    uint8_t sensor_count;
} onewire_t;

// Initialize OneWire bus
static void onewire_init(onewire_t *ow, gpio_num_t pin) {
    ow->pin = pin;
    ow->sensor_count = 0;
    memset(ow->sensors, 0, sizeof(ow->sensors));

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(pin, 1);  // Start high
}

// Reset the OneWire bus
static uint8_t onewire_reset(onewire_t *ow) {
    gpio_set_level(ow->pin, 0);
    esp_rom_delay_us(480);

    gpio_set_level(ow->pin, 1);
    esp_rom_delay_us(70);

    uint8_t presence = !gpio_get_level(ow->pin);
    esp_rom_delay_us(410);

    return presence;
}

// Write one bit
static void onewire_write_bit(onewire_t *ow, uint8_t v) {
    if (v & 1) {
        gpio_set_level(ow->pin, 0);
        esp_rom_delay_us(10);
        gpio_set_level(ow->pin, 1);
        esp_rom_delay_us(55);
    } else {
        gpio_set_level(ow->pin, 0);
        esp_rom_delay_us(65);
        gpio_set_level(ow->pin, 1);
        esp_rom_delay_us(5);
    }
}

// Read one bit
static uint8_t onewire_read_bit(onewire_t *ow) {
    uint8_t bit = 0;

    gpio_set_level(ow->pin, 0);
    esp_rom_delay_us(3);
    gpio_set_level(ow->pin, 1);
    esp_rom_delay_us(10);

    bit = gpio_get_level(ow->pin);
    esp_rom_delay_us(53);

    return bit;
}

// Write one byte
static void onewire_write_byte(onewire_t *ow, uint8_t data) {
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(ow, data & 0x01);
        data >>= 1;
    }
}

// Read one byte
static uint8_t onewire_read_byte(onewire_t *ow) {
    uint8_t data = 0;

    for (int i = 0; i < 8; i++) {
        data >>= 1;
        if (onewire_read_bit(ow)) {
            data |= 0x80;
        }
    }

    return data;
}

// Calculate CRC8 for OneWire
static uint8_t onewire_crc8(const uint8_t *addr, uint8_t len) {
    uint8_t crc = 0;

    for (uint8_t i = 0; i < len; i++) {
        uint8_t inbyte = addr[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

// Search for OneWire devices
static uint8_t onewire_search(onewire_t *ow) {
    uint8_t rom[8];
    uint8_t id_bit_number;
    uint8_t last_zero, rom_byte_number, search_result;
    uint8_t id_bit, cmp_id_bit;
    uint8_t rom_byte_mask, search_direction;

    // Initialize for search
    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    rom_byte_mask = 1;
    search_result = 0;

    // If the last call was not the last one
    if (!onewire_reset(ow)) {
        // Reset the search
        return 0;
    }

    // Issue the search command
    onewire_write_byte(ow, 0xF0);

    // Loop to do the search
    do {
        // Read a bit and its complement
        id_bit = onewire_read_bit(ow);
        cmp_id_bit = onewire_read_bit(ow);

        // Check for no devices on 1-wire
        if ((id_bit == 1) && (cmp_id_bit == 1)) {
            break;
        } else {
            // All devices coupled have 0 or 1
            if (id_bit != cmp_id_bit) {
                search_direction = id_bit;  // Bit write value for search
            } else {
                // If this discrepancy is before the Last Discrepancy
                // on a previous next then pick the same as last time
                if (id_bit_number < last_zero) {
                    search_direction = ((rom[rom_byte_number] & rom_byte_mask) > 0);
                } else {
                    // If equal to last zero pick 1, if not then pick 0
                    search_direction = (id_bit_number == last_zero);
                }

                // If 0 was picked then record its position in LastZero
                if (search_direction == 0) {
                    last_zero = id_bit_number;
                }
            }

            // Set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1) {
                rom[rom_byte_number] |= rom_byte_mask;
            } else {
                rom[rom_byte_number] &= ~rom_byte_mask;
            }

            // Serial number search direction write bit
            onewire_write_bit(ow, search_direction);

            // Increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // If the mask is 0 then go to new SerialNum byte rom_byte_number
            // and reset mask
            if (rom_byte_mask == 0) {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
        }
    } while (rom_byte_number < 8);  // Loop until through all ROM bytes

    // If the search was successful then
    if (id_bit_number >= 65) {
        // Search successful so set search_result equal to TRUE
        search_result = 1;
    }

    // If search was successful
    if (search_result == 1) {
        // Check CRC
        if (onewire_crc8(rom, 7) != rom[7]) {
            search_result = 0;
        } else if (rom[0] == DS18B20_FAMILY_CODE) {
            // It's a DS18B20, add to our list
            if (ow->sensor_count < MAX_SENSORS) {
                memcpy(ow->sensors[ow->sensor_count].rom, rom, 8);
                ow->sensors[ow->sensor_count].valid = 1;
                snprintf(ow->sensors[ow->sensor_count].name, sizeof(ow->sensors[0].name),
                        "Sensor%d", ow->sensor_count + 1);
                ow->sensor_count++;
                printf("Found DS18B20: ");
                for (int i = 0; i < 8; i++) {
                    printf("%02X", rom[i]);
                }
                printf("\n");
            }
        }
    }

    return search_result;
}

// Simple search (alternative to the complex one above)
static uint8_t onewire_simple_search(onewire_t *ow) {
    uint8_t rom[8];

    if (!onewire_reset(ow)) {
        return 0;
    }

    // Read ROM command (works with single device only)
    onewire_write_byte(ow, 0x33);

    // Read ROM
    for (int i = 0; i < 8; i++) {
        rom[i] = onewire_read_byte(ow);
    }

    // Check CRC
    if (onewire_crc8(rom, 7) != rom[7]) {
        printf("CRC error!\n");
        return 0;
    }

    // Check if it's DS18B20
    if (rom[0] == DS18B20_FAMILY_CODE) {
        if (ow->sensor_count < MAX_SENSORS) {
            memcpy(ow->sensors[ow->sensor_count].rom, rom, 8);
            ow->sensors[ow->sensor_count].valid = 1;
            snprintf(ow->sensors[ow->sensor_count].name, sizeof(ow->sensors[0].name),
                    "Sensor%d", ow->sensor_count + 1);
            ow->sensor_count++;
            return 1;
        }
    }

    return 0;
}

// Scan for all OneWire devices
static void scan_sensors(onewire_t *ow) {
    printf("Scanning for DS18B20 sensors...\n");

    // Reset sensor count
    ow->sensor_count = 0;
    memset(ow->sensors, 0, sizeof(ow->sensors));

    // Try simple search first (for single device)
    if (onewire_simple_search(ow)) {
        printf("Found %d sensor using simple search\n", ow->sensor_count);
    } else {
        // Try complex search for multiple devices
        printf("Trying complex search for multiple devices...\n");
       // uint8_t found = 0;
        for (int i = 0; i < 10; i++) {  // Try multiple times
            if (onewire_search(ow)) {
         //       found = 1;
            } else {
                break;
            }
        }
        printf("Found %d sensors\n", ow->sensor_count);
    }

    if (ow->sensor_count == 0) {
        printf("No DS18B20 sensors found!\n");
        printf("Check wiring:\n");
        printf("  VDD -> 3.3V\n");
        printf("  DQ  -> GPIO%d (with 4.7kΩ pull-up to 3.3V)\n", ONEWIRE_PIN);
        printf("  GND -> GND\n");
    }
}

// Convert ROM to string for display
static void rom_to_string(uint8_t *rom, char *buffer, size_t size) {
    snprintf(buffer, size, "%02X%02X%02X%02X%02X%02X%02X%02X",
             rom[0], rom[1], rom[2], rom[3], rom[4], rom[5], rom[6], rom[7]);
}

// Read temperature from specific sensor
static float read_sensor_temperature(onewire_t *ow, sensor_t *sensor) {
    if (!sensor->valid) {
        return -127.0;
    }

    if (!onewire_reset(ow)) {
        return -127.0;
    }

    // Match ROM command (address specific sensor)
    onewire_write_byte(ow, 0x55);
    for (int i = 0; i < 8; i++) {
        onewire_write_byte(ow, sensor->rom[i]);
    }

    // Start temperature conversion
    onewire_write_byte(ow, 0x44);

    // Wait for conversion (750ms for 12-bit resolution)
    vTaskDelay(pdMS_TO_TICKS(750));

    if (!onewire_reset(ow)) {
        return -127.0;
    }

    // Match ROM again
    onewire_write_byte(ow, 0x55);
    for (int i = 0; i < 8; i++) {
        onewire_write_byte(ow, sensor->rom[i]);
    }

    // Read scratchpad
    onewire_write_byte(ow, 0xBE);

    uint8_t data[9];
    for (int i = 0; i < 9; i++) {
        data[i] = onewire_read_byte(ow);
    }

    // Calculate temperature
    int16_t raw_temp = (data[1] << 8) | data[0];

    // Handle negative temperatures
    if (raw_temp & 0x8000) {
        raw_temp = -(~(raw_temp - 1));
    }

    float temperature = raw_temp / 16.0;

    // Verify CRC
    if (onewire_crc8(data, 8) != data[8]) {
        printf("CRC error for sensor!\n");
        return -127.0;
    }

    return temperature;
}

// Poll all sensors - SEPARATE POLLING FUNCTION
static void poll_sensors(onewire_t *ow) {
    printf("\n=== Polling Sensors ===\n");

    if (ow->sensor_count == 0) {
        printf("No sensors to poll. Rescanning...\n");
        scan_sensors(ow);
        if (ow->sensor_count == 0) {
            printf("Still no sensors found.\n");
            return;
        }
    }

    // Start conversion on all sensors simultaneously
    if (!onewire_reset(ow)) {
        printf("Bus reset failed\n");
        return;
    }

    // Skip ROM to address all devices
    onewire_write_byte(ow, 0xCC);
    onewire_write_byte(ow, 0x44);  // Start conversion

    // Wait for conversion
    vTaskDelay(pdMS_TO_TICKS(750));

    // Read each sensor
    for (int i = 0; i < ow->sensor_count; i++) {
        if (ow->sensors[i].valid) {
            float temp = read_sensor_temperature(ow, &ow->sensors[i]);
            ow->sensors[i].temperature = temp;
        }
    }
}

// Output function to print temperatures in Celsius
static void print_temperatures(onewire_t *ow) {
    printf("\n=== Temperature Readings ===\n");
    printf("Time: %lld\n", esp_timer_get_time() / 1000000);

    if (ow->sensor_count == 0) {
        printf("No active sensors.\n");
        return;
    }

    uint8_t valid_readings = 0;

    for (int i = 0; i < ow->sensor_count; i++) {
        if (ow->sensors[i].valid) {
            char rom_str[17];
            rom_to_string(ow->sensors[i].rom, rom_str, sizeof(rom_str));

            if (ow->sensors[i].temperature == -127.0) {
                printf("%s [%s]: ERROR\n",
                       ow->sensors[i].name, rom_str);
            } else {
                printf("%s [%s]: %.2f°C\n",
                       ow->sensors[i].name, rom_str,
                       ow->sensors[i].temperature);
                valid_readings++;
            }
        }
    }

    printf("Valid readings: %d/%d\n", valid_readings, ow->sensor_count);

    // Calculate average if we have multiple valid readings
    if (valid_readings > 1) {
        float sum = 0;
        uint8_t count = 0;

        for (int i = 0; i < ow->sensor_count; i++) {
            if (ow->sensors[i].valid && ow->sensors[i].temperature != -127.0) {
                sum += ow->sensors[i].temperature;
                count++;
            }
        }

        if (count > 0) {
            printf("Average temperature: %.2f°C\n", sum / count);
        }
    }
}

// Main application
void sensor_task(void*) {
    printf("\n=== ESP32 Multi-Sensor Thermometer ===\n");
    printf("Polling interval: %d seconds\n", POLL_INTERVAL_MS / 1000);
    printf("Max sensors: %d\n", MAX_SENSORS);
    printf("OneWire pin: GPIO%d\n", ONEWIRE_PIN);

    // Initialize OneWire
    onewire_t ow;
    onewire_init(&ow, ONEWIRE_PIN);

    // Initial scan for sensors
    scan_sensors(&ow);

    // Main loop with 30-second polling
    uint32_t poll_counter = 0;

    while (true) {
        printf("\n--- Cycle %lud ---\n", poll_counter + 1);

        // Poll all sensors (separate function)
        poll_sensors(&ow);

        // Print temperatures (separate output function)
        print_temperatures(&ow);

        // Wait for next poll
        printf("\nNext poll in %d seconds...\n", POLL_INTERVAL_MS / 1000);
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));

        poll_counter++;

        // Rescan every 10 polls (5 minutes) in case sensors are added/removed
        if (poll_counter % 10 == 0) {
            printf("\nPeriodic rescan...\n");
            scan_sensors(&ow);
        }
    }
}
