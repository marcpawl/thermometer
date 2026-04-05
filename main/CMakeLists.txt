# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(
		SRCS
			"rtc_clock.cpp"
		    "sensor_task.cpp" "poster_task.cpp" "ota_task.cpp"
		    "model.cpp" "main.cpp"
		INCLUDE_DIRS "."
		PRIV_REQUIRES
			esp_http_client app_update esp_https_ota
		    esp_netif mbedtls nvs_flash esp_wifi esp_psram
		    esp_event
		    espressif__onewire_bus driver esp_timer
		EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ESP-IDF often defaults to GNU extensions; turn off if you want strict C++20
set(CMAKE_CXX_EXTENSIONS OFF)
