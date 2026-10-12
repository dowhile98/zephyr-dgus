/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <dgus2/dgus2_protocol.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Test building a valid VP write command (0x82).
 *
 * Verifies that VP 0x1000 with payload [0x01, 0x02] produces:
 * 5A A5 05 82 10 00 01 02 (total 8 bytes).
 */
ZTEST(protocol_builder, test_build_write_vp_valid)
{
	uint8_t buf[32];
	size_t out_len = 0;
	const uint8_t payload[] = {0x01, 0x02};
	const uint8_t expected[] = {0x5A, 0xA5, 0x05, 0x82, 0x10, 0x00, 0x01, 0x02};

	int ret = dgus2_protocol_build_write_vp(buf, sizeof(buf), &out_len, 0x1000, payload,
						sizeof(payload));

	zassert_equal(ret, 0, "Expected return 0, got %d", ret);
	zassert_equal(out_len, sizeof(expected), "Expected len %zu, got %zu", sizeof(expected),
		      out_len);
	zassert_mem_equal(buf, expected, sizeof(expected), "Buffer content mismatch");
}

/**
 * @brief Test buffer overflow prevention for VP write command.
 *
 * Verifies that -ENOBUFS is returned when the destination buffer is smaller
 * than the required frame size.
 */
ZTEST(protocol_builder, test_build_write_vp_buffer_overflow)
{
	uint8_t buf[7]; /* Needs 8 bytes for 2-byte payload */
	size_t out_len = 0;
	const uint8_t payload[] = {0x01, 0x02};

	int ret = dgus2_protocol_build_write_vp(buf, sizeof(buf), &out_len, 0x1000, payload,
						sizeof(payload));

	zassert_equal(ret, -ENOBUFS, "Expected -ENOBUFS (%d), got %d", -ENOBUFS, ret);
}

/**
 * @brief Test input validation for VP write command.
 *
 * Verifies that -EINVAL is returned when NULL pointers are passed.
 */
ZTEST(protocol_builder, test_build_write_vp_invalid_params)
{
	uint8_t buf[32];
	size_t out_len = 0;
	const uint8_t payload[] = {0x01, 0x02};

	/* NULL output buffer */
	int ret = dgus2_protocol_build_write_vp(NULL, sizeof(buf), &out_len, 0x1000, payload,
						sizeof(payload));
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL buf, got %d", ret);

	/* NULL out_len pointer */
	ret = dgus2_protocol_build_write_vp(buf, sizeof(buf), NULL, 0x1000, payload,
					    sizeof(payload));
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL out_len, got %d", ret);

	/* NULL data pointer with data_len > 0 */
	ret = dgus2_protocol_build_write_vp(buf, sizeof(buf), &out_len, 0x1000, NULL,
					    sizeof(payload));
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL data with data_len > 0, got %d",
		      ret);

	/* NULL data with data_len == 0 is valid (0-length payload) */
	ret = dgus2_protocol_build_write_vp(buf, sizeof(buf), &out_len, 0x1000, NULL, 0);
	zassert_equal(ret, 0, "Expected 0 for data_len == 0, got %d", ret);
	zassert_equal(out_len, 6, "Expected frame length 6 for 0 payload, got %zu", out_len);
}

/**
 * @brief Test building a valid VP read command (0x83).
 *
 * Verifies that VP 0x2000 with 2 words produces:
 * 5A A5 04 83 20 00 02 (total 7 bytes).
 */
ZTEST(protocol_builder, test_build_read_vp_valid)
{
	uint8_t buf[32];
	size_t out_len = 0;
	const uint8_t expected[] = {0x5A, 0xA5, 0x04, 0x83, 0x20, 0x00, 0x02};

	int ret = dgus2_protocol_build_read_vp(buf, sizeof(buf), &out_len, 0x2000, 2);

	zassert_equal(ret, 0, "Expected return 0, got %d", ret);
	zassert_equal(out_len, sizeof(expected), "Expected len %zu, got %zu", sizeof(expected),
		      out_len);
	zassert_mem_equal(buf, expected, sizeof(expected), "Buffer content mismatch");
}

/**
 * @brief Test zero word count rejection for VP read command.
 *
 * Verifies that requesting 0 words returns -EINVAL.
 */
ZTEST(protocol_builder, test_build_read_vp_zero_words)
{
	uint8_t buf[32];
	size_t out_len = 0;

	int ret = dgus2_protocol_build_read_vp(buf, sizeof(buf), &out_len, 0x2000, 0);

	zassert_equal(ret, -EINVAL, "Expected -EINVAL for word_count == 0, got %d", ret);
}

/**
 * @brief Test buffer overflow prevention for VP read command.
 */
ZTEST(protocol_builder, test_build_read_vp_buffer_overflow)
{
	uint8_t buf[6]; /* Needs 7 bytes */
	size_t out_len = 0;

	int ret = dgus2_protocol_build_read_vp(buf, sizeof(buf), &out_len, 0x2000, 1);

	zassert_equal(ret, -ENOBUFS, "Expected -ENOBUFS (%d), got %d", -ENOBUFS, ret);
}

/**
 * @brief Test input validation for VP read command.
 */
ZTEST(protocol_builder, test_build_read_vp_invalid_params)
{
	uint8_t buf[32];
	size_t out_len = 0;

	int ret = dgus2_protocol_build_read_vp(NULL, sizeof(buf), &out_len, 0x2000, 1);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL buf, got %d", ret);

	ret = dgus2_protocol_build_read_vp(buf, sizeof(buf), NULL, 0x2000, 1);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL out_len, got %d", ret);
}

/**
 * @brief Test building a valid Register write command (0x80).
 *
 * Verifies that writing to register page 0x00, address 0x82 with payload [0x55] produces:
 * 5A A5 04 80 00 82 55 (total 7 bytes).
 */
ZTEST(protocol_builder, test_build_write_reg_valid)
{
	uint8_t buf[32];
	size_t out_len = 0;
	const uint8_t data[] = {0x55};
	const uint8_t expected[] = {0x5A, 0xA5, 0x04, 0x80, 0x00, 0x82, 0x55};

	int ret = dgus2_protocol_build_write_reg(buf, sizeof(buf), &out_len, 0x00, 0x82, data,
						 sizeof(data));

	zassert_equal(ret, 0, "Expected return 0, got %d", ret);
	zassert_equal(out_len, sizeof(expected), "Expected len %zu, got %zu", sizeof(expected),
		      out_len);
	zassert_mem_equal(buf, expected, sizeof(expected), "Buffer content mismatch");
}

/**
 * @brief Test buffer overflow prevention for Register write command.
 */
ZTEST(protocol_builder, test_build_write_reg_buffer_overflow)
{
	uint8_t buf[6]; /* Needs 7 bytes for 1-byte payload */
	size_t out_len = 0;
	const uint8_t data[] = {0x55};

	int ret = dgus2_protocol_build_write_reg(buf, sizeof(buf), &out_len, 0x00, 0x82, data,
						 sizeof(data));

	zassert_equal(ret, -ENOBUFS, "Expected -ENOBUFS (%d), got %d", -ENOBUFS, ret);
}

/**
 * @brief Test input validation for Register write command.
 */
ZTEST(protocol_builder, test_build_write_reg_invalid_params)
{
	uint8_t buf[32];
	size_t out_len = 0;
	const uint8_t data[] = {0x55};

	int ret = dgus2_protocol_build_write_reg(NULL, sizeof(buf), &out_len, 0x00, 0x82, data,
						 sizeof(data));
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL buf, got %d", ret);

	ret = dgus2_protocol_build_write_reg(buf, sizeof(buf), NULL, 0x00, 0x82, data,
					     sizeof(data));
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL out_len, got %d", ret);

	ret = dgus2_protocol_build_write_reg(buf, sizeof(buf), &out_len, 0x00, 0x82, NULL,
					     sizeof(data));
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL data with data_len > 0, got %d",
		      ret);
}

/**
 * @brief Test building register write command with explicit page (0x80).
 *
 * Format: 5A A5 [LEN] 80 [REG_PAGE] [REG_ADDR] [DATA...]
 */
ZTEST(protocol_builder, test_build_write_reg_page)
{
	uint8_t buf[32];
	size_t out_len = 0;
	const uint8_t data[] = {0x55, 0xAA};
	const uint8_t expected[] = {0x5A, 0xA5, 0x05, 0x80, 0x01, 0x20, 0x55, 0xAA};

	int ret = dgus2_protocol_build_write_reg(buf, sizeof(buf), &out_len, 0x01, 0x20, data,
						 sizeof(data));

	zassert_equal(ret, 0, "Expected return 0, got %d", ret);
	zassert_equal(out_len, sizeof(expected), "Expected len %zu, got %zu", sizeof(expected),
		      out_len);
	zassert_mem_equal(buf, expected, sizeof(expected), "Buffer content mismatch");
}

/**
 * @brief Test building register read command (0x81).
 *
 * Format: 5A A5 04 81 [REG_PAGE] [REG_ADDR] [LEN] (total 7 bytes).
 */
ZTEST(protocol_builder, test_build_read_reg)
{
	uint8_t buf[32];
	size_t out_len = 0;
	const uint8_t expected[] = {0x5A, 0xA5, 0x04, 0x81, 0x00, 0x0A, 0x04};

	int ret = dgus2_protocol_build_read_reg(buf, sizeof(buf), &out_len, 0x00, 0x0A, 0x04);

	zassert_equal(ret, 0, "Expected return 0, got %d", ret);
	zassert_equal(out_len, sizeof(expected), "Expected len %zu, got %zu", sizeof(expected),
		      out_len);
	zassert_mem_equal(buf, expected, sizeof(expected), "Buffer content mismatch");

	/* Buffer overflow check: 7 bytes needed */
	ret = dgus2_protocol_build_read_reg(buf, 6, &out_len, 0x00, 0x0A, 0x04);
	zassert_equal(ret, -ENOBUFS, "Expected -ENOBUFS, got %d", ret);

	/* Invalid parameters */
	ret = dgus2_protocol_build_read_reg(NULL, sizeof(buf), &out_len, 0x00, 0x0A, 0x04);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL buf, got %d", ret);

	ret = dgus2_protocol_build_read_reg(buf, sizeof(buf), NULL, 0x00, 0x0A, 0x04);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL out_len, got %d", ret);

	ret = dgus2_protocol_build_read_reg(buf, sizeof(buf), &out_len, 0x00, 0x0A, 0);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for len 0, got %d", ret);
}

/**
 * @brief Test building trend curve write command (0x84).
 *
 * Format: 5A A5 [LEN] 84 [CHANNEL] [POINT0_MSB] [POINT0_LSB] ...
 * LEN = CMD(1) + CHANNEL(1) + 2*COUNT
 */
ZTEST(protocol_builder, test_build_write_curve)
{
	uint8_t buf[32];
	size_t out_len = 0;
	const uint16_t points[] = {0x0010, 0x03E8};
	const uint8_t expected[] = {0x5A, 0xA5, 0x06, 0x84, 0x01, 0x00, 0x10, 0x03, 0xE8};

	int ret = dgus2_protocol_build_write_curve(buf, sizeof(buf), &out_len, 0x01, points, 2);

	zassert_equal(ret, 0, "Expected return 0, got %d", ret);
	zassert_equal(out_len, sizeof(expected), "Expected len %zu, got %zu", sizeof(expected),
		      out_len);
	zassert_mem_equal(buf, expected, sizeof(expected), "Buffer content mismatch");

	/* Buffer overflow check: 9 bytes needed */
	ret = dgus2_protocol_build_write_curve(buf, 8, &out_len, 0x01, points, 2);
	zassert_equal(ret, -ENOBUFS, "Expected -ENOBUFS, got %d", ret);

	/* Invalid parameters */
	ret = dgus2_protocol_build_write_curve(NULL, sizeof(buf), &out_len, 0x01, points, 2);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL buf, got %d", ret);

	ret = dgus2_protocol_build_write_curve(buf, sizeof(buf), NULL, 0x01, points, 2);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL out_len, got %d", ret);

	ret = dgus2_protocol_build_write_curve(buf, sizeof(buf), &out_len, 0x01, NULL, 2);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL points, got %d", ret);

	ret = dgus2_protocol_build_write_curve(buf, sizeof(buf), &out_len, 0x01, points, 0);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for count 0, got %d", ret);
}

/**
 * @brief Test CRC-16 Modbus computation function.
 *
 * Polynomial 0xA001, initial value 0xFFFF.
 */
ZTEST(protocol_builder, test_protocol_crc16)
{
	/* Standard Modbus test vector: "123456789" -> 0x4B37 */
	const uint8_t modbus_vec[] = "123456789";
	uint16_t crc = dgus2_protocol_crc16(modbus_vec, 9);
	zassert_equal(crc, 0x4B37, "Expected CRC 0x4B37 for 123456789, got 0x%04X", crc);

	/* DGUS II Command vector 1: [83 00 0F 01] -> 0x90ED */
	const uint8_t dgus_cmd1[] = {0x83, 0x00, 0x0F, 0x01};
	crc = dgus2_protocol_crc16(dgus_cmd1, sizeof(dgus_cmd1));
	zassert_equal(crc, 0x90ED, "Expected CRC 0x90ED for read version cmd, got 0x%04X", crc);

	/* DGUS II Command vector 2: [82 00 82 0A] -> 0xFBC8 */
	const uint8_t dgus_cmd2[] = {0x82, 0x00, 0x82, 0x0A};
	crc = dgus2_protocol_crc16(dgus_cmd2, sizeof(dgus_cmd2));
	zassert_equal(crc, 0xFBC8, "Expected CRC 0xFBC8 for backlight cmd, got 0x%04X", crc);

	/* Edge cases */
	zassert_equal(dgus2_protocol_crc16(NULL, 10), 0, "Expected 0 for NULL data");
	zassert_equal(dgus2_protocol_crc16(dgus_cmd1, 0), 0, "Expected 0 for length 0");
}
