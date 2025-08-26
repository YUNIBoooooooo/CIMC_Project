#include "usart_app.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "usart.h"
#include "mydefine.h"

// UART相关缓冲区和索引
uint16_t uart_rx_index = 0;
uint32_t uart_rx_ticks = 0;
uint8_t uart_rx_buffer[128] = {0};
uint8_t uart_rx_dma_buffer[128] = {0};
uint8_t uart_dma_buffer[128] = {0};
uint8_t uart_flag = 0;
struct rt_ringbuffer uart_ringbuffer;
uint8_t ringbuffer_pool[128];

// 命令状态
static cmd_state_t g_cmd_state = CMD_STATE_IDLE;

// 采样输出相关变量
uint8_t g_sampling_output_enabled = 0;
uint32_t g_last_output_time = 0;

output_format_t g_output_format = OUTPUT_FORMAT_NORMAL;

// 内部函数声明
static void convert_voltage_to_hex_format(float voltage, uint16_t *integer_part, uint16_t *decimal_part);
static void test_unhide_conversion(const char *hex_data);
static void test_data_storage(void);

/// @brief RTC时间转UNIX时间戳
/// @param time RTC时间结构体指针
/// @param date RTC日期结构体指针
/// @return UNIX时间戳
uint32_t convert_rtc_to_unix_timestamp(RTC_TimeTypeDef *time, RTC_DateTypeDef *date)
{
	static const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	uint32_t year = date->Year + 2000;
	uint32_t month = date->Month;
	uint32_t day = date->Date;
	uint32_t hour = time->Hours;
	uint32_t minute = time->Minutes;
	uint32_t second = time->Seconds;

	uint32_t days = 0;
	for (uint32_t y = 1970; y < year; y++)
	{
		if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
		{
			days += 366;
		}
		else
		{
			days += 365;
		}
	}

	for (uint32_t m = 1; m < month; m++)
	{
		days += days_in_month[m - 1];
		if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
		{
			days += 1;
		}
	}

	days += (day - 1);

	uint32_t timestamp = days * 86400 + hour * 3600 + minute * 60 + second - 28800;

	return timestamp;
}

/// @brief 测试时间戳转换
static void test_unix_timestamp_conversion(void)
{

	RTC_TimeTypeDef test_time = {0};
	RTC_DateTypeDef test_date = {0};

	test_time.Hours = 8;
	test_time.Minutes = 0;
	test_time.Seconds = 21;

	test_date.Year = 0;
	test_date.Month = 1;
	test_date.Date = 2;
	uint32_t result = convert_rtc_to_unix_timestamp(&test_time, &test_date);

	my_printf(&huart1, "Test: 2000-01-02 8:00:21 -> %lu (expected: 946771221)\r\n", result);
	my_printf(&huart1, "Timestamp hex: %08X\r\n", result);

	char hex_output[32];
	format_hex_output(result, 3.2911376953125f, 0, hex_output);
	my_printf(&huart1, "Hide format test: %s (expected: 386E951500034A88)\r\n", hex_output);

	uint16_t int_part, dec_part;
	convert_voltage_to_hex_format(3.2911376953125f, &int_part, &dec_part);
	my_printf(&huart1, "Voltage 3.291V -> %04X%04X (expected: 00034A88)\r\n", int_part, dec_part);

	test_unhide_conversion("386E951500034A88");
}

/// @brief 测试隐藏数据解码
/// @param hex_data 待解码的十六进制数据
static void test_unhide_conversion(const char *hex_data)
{
	if (strlen(hex_data) < 16)
	{
		my_printf(&huart1, "Invalid hex data length\r\n");
		return;
	}

	uint32_t timestamp;
	sscanf(hex_data, "%8X", &timestamp);

	uint16_t voltage_int, voltage_dec;
	sscanf(hex_data + 8, "%4X%4X", &voltage_int, &voltage_dec);

	uint32_t local_timestamp = timestamp + 28800;
	uint32_t days = local_timestamp / 86400;
	uint32_t remaining = local_timestamp % 86400;
	uint32_t hours = remaining / 3600;
	uint32_t minutes = (remaining % 3600) / 60;
	uint32_t seconds = remaining % 60;

	uint32_t year = 1970;
	while (days >= 365)
	{
		if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
		{
			if (days >= 366)
			{
				days -= 366;
				year++;
			}
			else
			{
				break;
			}
		}
		else
		{
			days -= 365;
			year++;
		}
	}

	uint32_t month = 1;
	uint32_t day = days + 1;

	float voltage = (float)voltage_int + (float)voltage_dec / 65536.0f;

	my_printf(&huart1, "Unhide test: %s\r\n", hex_data);
	my_printf(&huart1, "Timestamp: %lu\r\n", timestamp);
	my_printf(&huart1, "(%04lu-%02lu-%02lu %02lu:%02lu:%02lu)\r\n", year, month, day, hours, minutes, seconds);
	my_printf(&huart1, "Voltage: %.6fV\r\n", voltage);
}

/// @brief 测试数据存储
static void test_data_storage(void)
{
	my_printf(&huart1, "Testing data storage...\r\n");

	my_printf(&huart1, "Testing sample storage...\r\n");
	data_storage_status_t result = data_storage_write_sample(3.3f);
	my_printf(&huart1, "Sample storage result: %d\r\n", result);

	my_printf(&huart1, "Testing overlimit storage...\r\n");
	result = data_storage_write_overlimit(5.0f, 4.5f);
	my_printf(&huart1, "Overlimit storage result: %d\r\n", result);

	my_printf(&huart1, "Testing hidedata storage...\r\n");
	result = data_storage_write_hidedata(3.8f, 1);
	my_printf(&huart1, "Hidedata storage result: %d\r\n", result);

	my_printf(&huart1, "Data storage test completed.\r\n");
}

/// @brief 电压转16进制格式
/// @param voltage 电压值
/// @param integer_part 整数部分
/// @param decimal_part 小数部分
static void convert_voltage_to_hex_format(float voltage, uint16_t *integer_part, uint16_t *decimal_part)
{

	*integer_part = (uint16_t)voltage;

	float fractional = voltage - (float)(*integer_part);
	*decimal_part = (uint16_t)(fractional * 65536.0f);
}


static void test_voltage_hex_encoding(void)
{

	float test_voltage = 12.5f;
	uint16_t int_part, dec_part;

	convert_voltage_to_hex_format(test_voltage, &int_part, &dec_part);

	my_printf(&huart1, "Test: %.1fV -> %04X%04X (expected: 000C8000)\r\n",
			  test_voltage, int_part, dec_part);
}


void format_hex_output(uint32_t timestamp, float voltage, uint8_t is_overlimit, char *output)
{
	uint16_t int_part, dec_part;

	convert_voltage_to_hex_format(voltage, &int_part, &dec_part);

	sprintf(output, "%08X%04X%04X%s",
			timestamp,
			int_part,
			dec_part,
			is_overlimit ? "*" : "");
}


static void test_hex_format_output(void)
{

	uint32_t test_timestamp = 1735705845;
	float test_voltage = 12.5f;
	uint8_t test_overlimit = 0;
	char output_buffer[32];

	format_hex_output(test_timestamp, test_voltage, test_overlimit, output_buffer);

	my_printf(&huart1, "Test HEX output: %s (expected: 6774C4F5000C8000)\r\n", output_buffer);

	test_overlimit = 1;
	format_hex_output(test_timestamp, test_voltage, test_overlimit, output_buffer);
	my_printf(&huart1, "Test HEX output (overlimit): %s (expected: 6774C4F5000C8000*)\r\n", output_buffer);
}


int my_printf(UART_HandleTypeDef *huart, const char *format, ...)
{
	char buffer[512];
	va_list arg;
	int len;
	va_start(arg, format);
	len = vsnprintf(buffer, sizeof(buffer), format, arg);
	va_end(arg);
	HAL_UART_Transmit(huart, (uint8_t *)buffer, (uint16_t)len, 0xFF);
	return len;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		uart_rx_ticks = uwTick;
		uart_rx_index++;
		HAL_UART_Receive_IT(&huart1, &uart_rx_buffer[uart_rx_index], 1);
	}
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART1)
	{
		HAL_UART_DMAStop(huart);

		rt_ringbuffer_put(&uart_ringbuffer, uart_rx_dma_buffer, Size);

		memset(uart_rx_dma_buffer, 0, sizeof(uart_rx_dma_buffer));

		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));

		__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
	}
}


void parse_uart_command(uint8_t *buffer, uint16_t length)
{

	if (strcmp((char *)buffer, "test") == 0)
	{
		system_self_check();
	}

	else if (strcmp((char *)buffer, "testtime") == 0)
	{
		test_unix_timestamp_conversion();
	}

	else if (strncmp((char *)buffer, "testhide ", 9) == 0)
	{
		if (length > 9)
		{
			char *hex_data = (char *)buffer + 9;
			test_unhide_conversion(hex_data);
		}
		else
		{
			my_printf(&huart1, "Usage: testhide <hex_data>\r\n");
		}
	}

	else if (strcmp((char *)buffer, "teststorage") == 0)
	{
		test_data_storage();
	}
	else if (strcmp((char *)buffer, "RTC Config") == 0)
	{
		handle_rtc_config_command();
	}
	else if (strcmp((char *)buffer, "RTC now") == 0)
	{
		rtc_print_current_time_now();
	}

	else if (strcmp((char *)buffer, "conf") == 0)
	{
		handle_conf_command();
	}
	else if (strcmp((char *)buffer, "ratio") == 0)
	{
		handle_ratio_command();
	}
	else if (strcmp((char *)buffer, "limit") == 0)
	{
		handle_limit_command();
	}

	else if (strcmp((char *)buffer, "config save") == 0)
	{
		handle_configsave_command();
	}
	else if (strcmp((char *)buffer, "config read") == 0)
	{
		handle_configread_command();
	}
	else if (strcmp((char *)buffer, "start") == 0)
	{
		handle_start_command();
	}
	else if (strcmp((char *)buffer, "stop") == 0)
	{
		handle_stop_command();
	}
	else if (strcmp((char *)buffer, "hide") == 0)
	{
		handle_hide_command();
	}
	else if (strcmp((char *)buffer, "unhide") == 0)
	{
		handle_unhide_command();
	}
	else if (g_cmd_state != CMD_STATE_IDLE)
	{
		handle_interactive_input((char *)buffer);
	}
}


void uart_task(void)
{
	uint16_t length;

	length = rt_ringbuffer_data_len(&uart_ringbuffer);

	if (length > 0)
	{
		rt_ringbuffer_get(&uart_ringbuffer, uart_dma_buffer, length);

		parse_uart_command(uart_dma_buffer, length);
		memset(uart_dma_buffer, 0, sizeof(uart_dma_buffer));
	}

	handle_sampling_output();
}


void handle_conf_command(void)
{
	ini_config_t ini_config;
	config_params_t config_params;
	data_storage_write_log("conf command");
	ini_status_t ini_status = ini_parse_file("config.ini", &ini_config);

	if (ini_status == INI_FILE_NOT_FOUND)
	{
		my_printf(&huart1, "config.ini file not found.\r\n");
		return;
	}
	if (ini_status != INI_OK)
	{
		my_printf(&huart1, "config.ini format error.\r\n");
		return;
	}
	if (!ini_config.ratio_found || !ini_config.limit_found)
	{
		my_printf(&huart1, "config.ini missing parameters.\r\n");
		return;
	}
	if (config_validate_ratio(ini_config.ratio) != CONFIG_OK)
	{
		my_printf(&huart1, "ratio parameter out of range (0-100).\r\n");
		return;
	}

	if (config_validate_limit(ini_config.limit) != CONFIG_OK)
	{
		my_printf(&huart1, "limit parameter out of range (0-500).\r\n");
		return;
	}
	if (config_get_params(&config_params) != CONFIG_OK)
	{
		my_printf(&huart1, "config system error.\r\n");
		return;
	}
	config_params.ratio = ini_config.ratio;
	config_params.limit = ini_config.limit;
	if (config_set_params(&config_params) != CONFIG_OK)
	{
		my_printf(&huart1, "config update failed.\r\n");
		return;
	}
	if (config_save_to_flash() != CONFIG_OK)
	{
		my_printf(&huart1, "config save to flash failed.\r\n");
		return;
	}
	my_printf(&huart1, "Ratio = %.1f\r\n", ini_config.ratio);
	my_printf(&huart1, "Limit = %.1f\r\n", ini_config.limit);
	my_printf(&huart1, "config read success\r\n");
	char log_msg[128];
	sprintf(log_msg, "config read success - ratio %.1f, limit %.1f", ini_config.ratio, ini_config.limit);
	data_storage_write_log(log_msg);
}


void handle_ratio_command(void)
{
	config_params_t config_params;
	data_storage_write_log("ratio command");
	if (config_get_params(&config_params) != CONFIG_OK)
	{
		my_printf(&huart1, "config system error.\r\n");
		return;
	}
	my_printf(&huart1, "Ratio=%.1f\r\n", config_params.ratio);
	my_printf(&huart1, "Input value(0~100):\r\n");
	g_cmd_state = CMD_STATE_WAIT_RATIO;
}


void handle_interactive_input(char *input)
{
	float value;
	config_params_t config_params;
	if (sscanf(input, "%f", &value) != 1)
	{
		my_printf(&huart1, "invalid input format.\r\n");
		g_cmd_state = CMD_STATE_IDLE;
		return;
	}
	if (config_get_params(&config_params) != CONFIG_OK)
	{
		my_printf(&huart1, "config system error.\r\n");
		g_cmd_state = CMD_STATE_IDLE;
		return;
	}

	if (g_cmd_state == CMD_STATE_WAIT_RATIO)
	{
		if (config_validate_ratio(value) != CONFIG_OK)
		{
			my_printf(&huart1, "ratio invalid\r\n");
			my_printf(&huart1, "Ratio = %.1f\r\n", config_params.ratio);
		}
		else
		{
			config_params.ratio = value;
			if (config_set_params(&config_params) == CONFIG_OK)
			{
				my_printf(&huart1, "ratio modified success\r\n");
				my_printf(&huart1, "Ratio = %.1f\r\n", value);
				char log_msg[64];
				sprintf(log_msg, "ratio config success to %.1f", value);
				data_storage_write_log(log_msg);
			}
			else
			{
				my_printf(&huart1, "config update failed.\r\n");
			}
		}
		g_cmd_state = CMD_STATE_IDLE;
	}
	else if (g_cmd_state == CMD_STATE_WAIT_LIMIT)
	{
		if (config_validate_limit(value) != CONFIG_OK)
		{
			my_printf(&huart1, "limit invalid\r\n");
			my_printf(&huart1, "limit = %.2f\r\n", config_params.limit);
		}
		else
		{
			config_params.limit = value;
			if (config_set_params(&config_params) == CONFIG_OK)
			{
				my_printf(&huart1, "limit modified success\r\n");
				my_printf(&huart1, "limit = %.2f\r\n", value);
				char log_msg[64];
				sprintf(log_msg, "limit config success to %.1f", value);
				data_storage_write_log(log_msg);
			}
			else
			{
				my_printf(&huart1, "config update failed.\r\n");
			}
		}
		g_cmd_state = CMD_STATE_IDLE;
	}
	else if (g_cmd_state == CMD_STATE_WAIT_RTC)
	{
		HAL_StatusTypeDef status = rtc_set_time_from_string(input);
		if (status == HAL_OK)
		{
			my_printf(&huart1, "RTC Config success\r\n");
			my_printf(&huart1, "Time: %s\r\n", input);
			char log_msg[128];
			sprintf(log_msg, "RTC config success to %s", input);
			data_storage_write_log(log_msg);
		}
		else
		{
			my_printf(&huart1, "RTC Config failed\r\n");
			my_printf(&huart1, "Invalid time format. Please use: 2025-01-01 15:00:10 or 2025��01��01��12:00:30\r\n");
		}
		g_cmd_state = CMD_STATE_IDLE;
	}
}


void handle_limit_command(void)
{
	config_params_t config_params;
	data_storage_write_log("limit command");
	if (config_get_params(&config_params) != CONFIG_OK)
	{
		my_printf(&huart1, "config system error.\r\n");
		return;
	}
	my_printf(&huart1, "limit=%.1f\r\n", config_params.limit);
	my_printf(&huart1, "Input value(0~200):\r\n");
	g_cmd_state = CMD_STATE_WAIT_LIMIT;
}

void handle_configsave_command(void)
{
	config_params_t config_params;
	if (config_get_params(&config_params) != CONFIG_OK)
	{
		my_printf(&huart1, "config system error.\r\n");
		return;
	}
	my_printf(&huart1, "ratio: %.2f\r\n", config_params.ratio);
	my_printf(&huart1, "limit: %.2f\r\n", config_params.limit);
	if (config_save_to_flash() != CONFIG_OK)
	{
		my_printf(&huart1, "save parameters to flash failed.\r\n");
		return;
	}
	my_printf(&huart1, "save parameters to flash\r\n");
}

void handle_configread_command(void)
{
	config_params_t config_params;
	config_status_t status = config_load_from_flash();
	if (status != CONFIG_OK)
	{
		my_printf(&huart1, "read parameters from flash failed.\r\n");
		return;
	}
	my_printf(&huart1, "read parameters from flash\r\n");
	if (config_get_params(&config_params) != CONFIG_OK)
	{
		my_printf(&huart1, "config system error.\r\n");
		return;
	}
	my_printf(&huart1, "ratio: %.2f\r\n", config_params.ratio);
	my_printf(&huart1, "limit: %.2f\r\n", config_params.limit);
}


void handle_start_command(void)
{
	sampling_init();
	if (sampling_start() != SAMPLING_OK)
	{
		my_printf(&huart1, "sampling start failed.\r\n");
		return;
	}
	my_printf(&huart1, "Periodic Sampling\r\n");
	sampling_cycle_t cycle = sampling_get_cycle();
	my_printf(&huart1, "sample cycle: %ds\r\n", (int)cycle);
	g_sampling_output_enabled = 1;
	g_last_output_time = HAL_GetTick();
	char log_msg[64];
	sprintf(log_msg, "sample start - cycle %ds (command)", (int)cycle);
	data_storage_write_log(log_msg);
}


void handle_stop_command(void)
{
	sampling_init();
	if (sampling_stop() != SAMPLING_OK)
	{
		my_printf(&huart1, "sampling stop failed.\r\n");
		return;
	}
	my_printf(&huart1, "Periodic Sampling STOP\r\n");
	g_sampling_output_enabled = 0;
	data_storage_write_log("sample stop (command)");
}


void handle_sampling_output(void)
{
	if (!g_sampling_output_enabled)
	{
		return;
	}
	if (sampling_get_state() != SAMPLING_ACTIVE)
	{
		return;
	}
	uint32_t current_time = HAL_GetTick();
	sampling_cycle_t cycle = sampling_get_cycle();
	uint32_t cycle_ms = cycle * 1000;

	if (current_time - g_last_output_time >= cycle_ms)
	{
		g_last_output_time = current_time;
		RTC_TimeTypeDef current_rtc_time = {0};
		RTC_DateTypeDef current_rtc_date = {0};
		HAL_RTC_GetTime(&hrtc, &current_rtc_time, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &current_rtc_date, RTC_FORMAT_BIN);
		float voltage = sampling_get_voltage();
		uint8_t is_overlimit = sampling_check_overlimit();
		if (g_output_format == OUTPUT_FORMAT_HIDDEN)
		{
			uint32_t timestamp = convert_rtc_to_unix_timestamp(&current_rtc_time, &current_rtc_date);
			char hex_output[32];

			format_hex_output(timestamp, voltage, is_overlimit, hex_output);
			my_printf(&huart1, "%s\r\n", hex_output);
		}
		else
		{
			if (is_overlimit)
			{
				config_params_t config_params;
				if (config_get_params(&config_params) == CONFIG_OK)
				{
					my_printf(&huart1, "%04d-%02d-%02d %02d:%02d:%02d ch0=%.2fV OverLimit(%.2f)!\r\n",
							  current_rtc_date.Year + 2000,
							  current_rtc_date.Month,
							  current_rtc_date.Date,
							  current_rtc_time.Hours,
							  current_rtc_time.Minutes,
							  current_rtc_time.Seconds,
							  voltage,
							  config_params.limit);
				}
				else
				{
					my_printf(&huart1, "%04d-%02d-%02d %02d:%02d:%02d ch0=%.2fV OverLimit!!\r\n",
							  current_rtc_date.Year + 2000,
							  current_rtc_date.Month,
							  current_rtc_date.Date,
							  current_rtc_time.Hours,
							  current_rtc_time.Minutes,
							  current_rtc_time.Seconds,
							  voltage);
				}
			}
			else
			{
				my_printf(&huart1, "%04d-%02d-%02d %02d:%02d:%02d ch0=%.2fV\r\n",
						  current_rtc_date.Year + 2000,
						  current_rtc_date.Month,
						  current_rtc_date.Date,
						  current_rtc_time.Hours,
						  current_rtc_time.Minutes,
						  current_rtc_time.Seconds,
						  voltage);
			}
		}
		if (g_output_format == OUTPUT_FORMAT_HIDDEN)
		{

			data_storage_status_t result = data_storage_write_hidedata(voltage, is_overlimit);
		}
		else
		{
			data_storage_status_t result = data_storage_write_sample(voltage);
		}

		if (is_overlimit)
		{
			config_params_t config_params;
			float limit_value = 0.0f;
			if (config_get_params(&config_params) == CONFIG_OK)
			{
				limit_value = config_params.limit;
			}

			data_storage_status_t result = data_storage_write_overlimit(voltage, limit_value);
		}
	}
}


void handle_hide_command(void)
{
	g_output_format = OUTPUT_FORMAT_HIDDEN;
	data_storage_write_log("hide data");
}


void handle_unhide_command(void)
{
	g_output_format = OUTPUT_FORMAT_NORMAL;
	data_storage_write_log("unhide data");
}


void handle_rtc_config_command(void)
{
	data_storage_write_log("RTC Config command");
	my_printf(&huart1, "Input Datetime\r\n");
	g_cmd_state = CMD_STATE_WAIT_RTC;
}
