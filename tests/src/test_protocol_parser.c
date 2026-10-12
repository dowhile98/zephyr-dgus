/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <dgus2/dgus2_protocol.h>
#include <errno.h>
#include <string.h>

/**
 * @brief Test initialization of the DGUS II protocol parser.
 *
 * Verifies that the parser struct is cleanly initialized with state
 * WAIT_HEADER1 and all counters set to zero.
 */
ZTEST(protocol_parser, test_parser_init) {
	struct dgus2_parser parser;

	/* Safe NULL handling */
	dgus2_protocol_parser_init(NULL);

	dgus2_protocol_parser_init(&parser);
	zassert_equal(parser.state, DGUS2_STATE_WAIT_HEADER1, "Initial state must be WAIT_HEADER1");
	zassert_equal(parser.rx_len, 0, "Initial rx_len must be 0");
	zassert_equal(parser.bytes_read, 0, "Initial bytes_read must be 0");
	zassert_equal(parser.parse_errors, 0, "Initial parse_errors must be 0");
}

/**
 * @brief Test parser safety with NULL parameters.
 */
ZTEST(protocol_parser, test_parse_null_params) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;

	dgus2_protocol_parser_init(&parser);

	zassert_false(dgus2_protocol_parse_byte(NULL, 0x5A, &frame),
	    "Should return false when parser is NULL");
	zassert_false(dgus2_protocol_parse_byte(&parser, 0x5A, NULL),
	    "Should return false when out_frame is NULL");
}

/**
 * @brief Test parsing a single valid complete frame.
 *
 * Feed frame 5A A5 05 82 10 00 AA BB byte-by-byte.
 * Verifies that the parser emits a complete frame only on the final byte.
 */
ZTEST(protocol_parser, test_parse_single_valid_frame) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t raw[] = {0x5A, 0xA5, 0x05, 0x82, 0x10, 0x00, 0xAA, 0xBB};

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(raw) - 1; i++) {
		bool completed = dgus2_protocol_parse_byte(&parser, raw[i], &frame);

		zassert_false(completed, "Frame prematurely emitted at index %zu", i);
	}

	bool completed = dgus2_protocol_parse_byte(&parser, raw[sizeof(raw) - 1], &frame);

	zassert_true(completed, "Frame should be completed on final byte");
	zassert_equal(frame.cmd, 0x82, "Command mismatch: got 0x%02X", frame.cmd);
	zassert_equal(frame.vp_addr, 0x1000, "VP mismatch: got 0x%04X", frame.vp_addr);
	zassert_equal(frame.data_len, 2, "Data length mismatch: got %u", frame.data_len);
	zassert_equal(frame.data[0], 0xAA, "Data[0] mismatch: got 0x%02X", frame.data[0]);
	zassert_equal(frame.data[1], 0xBB, "Data[1] mismatch: got 0x%02X", frame.data[1]);
	zassert_equal(
	    parser.state, DGUS2_STATE_WAIT_HEADER1, "FSM state must reset to WAIT_HEADER1");
	zassert_equal(parser.parse_errors, 0, "No parse errors expected");
}

/**
 * @brief Test recovery from noise/garbage bytes prior to valid frame.
 *
 * Injects arbitrary noise bytes before a valid header and checks that the
 * parser locks onto the frame once 0x5A 0xA5 is received.
 */
ZTEST(protocol_parser, test_parse_noise_recovery) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t stream[] = {
	    0xFF, 0x00, 0x7E, 0x12, 0x34, 0xAA, 0x5A, 0xA5, 0x03, 0x82, 0x20, 0x00};

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(stream) - 1; i++) {
		bool completed = dgus2_protocol_parse_byte(&parser, stream[i], &frame);

		zassert_false(completed, "Frame prematurely emitted at byte %zu", i);
	}

	bool completed = dgus2_protocol_parse_byte(&parser, stream[sizeof(stream) - 1], &frame);

	zassert_true(completed, "Frame should complete on final byte despite leading noise");
	zassert_equal(frame.cmd, 0x82, "Command mismatch");
	zassert_equal(frame.vp_addr, 0x2000, "VP mismatch");
	zassert_equal(frame.data_len, 0, "Data length should be 0");
}

/**
 * @brief Test consecutive repeated header1 bytes (5A 5A A5).
 *
 * Verifies that the sequence 5A 5A A5 does not get discarded as an invalid
 * header but correctly synchronizes to the second 5A as the true start.
 */
ZTEST(protocol_parser, test_parse_repeated_header1) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t stream[] = {0x5A, 0x5A, 0xA5, 0x03, 0x82, 0x10, 0x00};

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(stream) - 1; i++) {
		bool completed = dgus2_protocol_parse_byte(&parser, stream[i], &frame);

		zassert_false(completed, "Premature completion at byte %zu", i);
	}

	bool completed = dgus2_protocol_parse_byte(&parser, stream[sizeof(stream) - 1], &frame);

	zassert_true(completed, "Frame should complete after 5A 5A A5 header sequence");
	zassert_equal(frame.cmd, 0x82, "Command mismatch");
	zassert_equal(frame.vp_addr, 0x1000, "VP mismatch");
	zassert_equal(frame.data_len, 0, "Data length should be 0");
}

/**
 * @brief Test rejection of invalid length (LEN < 3).
 *
 * DGUS frames require at least 3 bytes in payload (CMD, VP_MSB, VP_LSB).
 * If LEN < 3 is received, FSM must discard and increment parse_errors.
 */
ZTEST(protocol_parser, test_parse_invalid_length_too_short) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t raw[] = {0x5A, 0xA5, 0x02};

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(raw); i++) {
		bool completed = dgus2_protocol_parse_byte(&parser, raw[i], &frame);

		zassert_false(completed, "Should not complete on invalid length byte");
	}

	zassert_equal(parser.state, DGUS2_STATE_WAIT_HEADER1, "FSM should reset to WAIT_HEADER1");
	zassert_equal(parser.parse_errors, 1, "parse_errors should be 1");
}

/**
 * @brief Test rejection of invalid length (LEN > MAX_PAYLOAD + 3).
 *
 * If LEN exceeds the maximum buffer capacity (250 + 3 = 253), FSM must discard
 * and increment parse_errors.
 */
ZTEST(protocol_parser, test_parse_invalid_length_too_long) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t raw[] = {0x5A, 0xA5, 0xFE}; /* LEN = 254 */

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(raw); i++) {
		bool completed = dgus2_protocol_parse_byte(&parser, raw[i], &frame);

		zassert_false(completed, "Should not complete on excessive length byte");
	}

	zassert_equal(parser.state, DGUS2_STATE_WAIT_HEADER1, "FSM should reset to WAIT_HEADER1");
	zassert_equal(parser.parse_errors, 1, "parse_errors should be 1");
}

/**
 * @brief Test sequential reception of multiple fragmented frames.
 *
 * Simulates real UART byte-by-byte streaming across multiple back-to-back frames.
 */
ZTEST(protocol_parser, test_parse_fragmented_stream) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t frame1[] = {0x5A, 0xA5, 0x04, 0x82, 0x10, 0x00, 0x11};
	const uint8_t frame2[] = {0x5A, 0xA5, 0x05, 0x82, 0x20, 0x00, 0x22, 0x33};

	dgus2_protocol_parser_init(&parser);

	/* Stream frame 1 */
	for (size_t i = 0; i < sizeof(frame1) - 1; i++) {
		zassert_false(dgus2_protocol_parse_byte(&parser, frame1[i], &frame),
		    "Frame 1 premature at %zu", i);
	}
	zassert_true(dgus2_protocol_parse_byte(&parser, frame1[sizeof(frame1) - 1], &frame),
	    "Frame 1 should complete");
	zassert_equal(frame.cmd, 0x82, "Frame 1 CMD mismatch");
	zassert_equal(frame.vp_addr, 0x1000, "Frame 1 VP mismatch");
	zassert_equal(frame.data_len, 1, "Frame 1 data_len mismatch");
	zassert_equal(frame.data[0], 0x11, "Frame 1 data[0] mismatch");

	/* Stream frame 2 immediately following */
	for (size_t i = 0; i < sizeof(frame2) - 1; i++) {
		zassert_false(dgus2_protocol_parse_byte(&parser, frame2[i], &frame),
		    "Frame 2 premature at %zu", i);
	}
	zassert_true(dgus2_protocol_parse_byte(&parser, frame2[sizeof(frame2) - 1], &frame),
	    "Frame 2 should complete");
	zassert_equal(frame.cmd, 0x82, "Frame 2 CMD mismatch");
	zassert_equal(frame.vp_addr, 0x2000, "Frame 2 VP mismatch");
	zassert_equal(frame.data_len, 2, "Frame 2 data_len mismatch");
	zassert_equal(frame.data[0], 0x22, "Frame 2 data[0] mismatch");
	zassert_equal(frame.data[1], 0x33, "Frame 2 data[1] mismatch");
}

/**
 * @brief Test parsing DWIN 0x83 Read VP response with word_count.
 *
 * Official DWIN T5L specification format for 0x83 response:
 * 5A A5 [LEN=06] [83] [VP_H=10] [VP_L=00] [WORD_COUNT=01] [DATA_H=12] [DATA_L=34]
 * - LEN = 6
 * - CMD = 0x83
 * - VP = 0x1000
 * - WORD_COUNT = 0x01 (1 word = 2 bytes)
 * - DATA = 0x12, 0x34
 */
ZTEST(protocol_parser, test_parse_0x83_word_count) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t raw[] = {0x5A, 0xA5, 0x06, 0x83, 0x10, 0x00, 0x01, 0x12, 0x34};

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(raw) - 1; i++) {
		zassert_false(dgus2_protocol_parse_byte(&parser, raw[i], &frame),
		    "Premature completion at index %zu", i);
	}

	bool completed = dgus2_protocol_parse_byte(&parser, raw[sizeof(raw) - 1], &frame);

	zassert_true(completed, "Frame should complete on final byte");
	zassert_equal(frame.cmd, 0x83, "Command should be 0x83");
	zassert_equal(frame.vp_addr, 0x1000, "VP address should be 0x1000");
	zassert_equal(frame.word_count, 1, "Word count mismatch: got %u", frame.word_count);
	zassert_equal(frame.data_len, 2, "Data length mismatch: got %u", frame.data_len);
	zassert_equal(frame.data[0], 0x12, "Data[0] mismatch: got 0x%02X", frame.data[0]);
	zassert_equal(frame.data[1], 0x34, "Data[1] mismatch: got 0x%02X", frame.data[1]);
}

/**
 * @brief Test parsing Write VP confirmation ACK ('OK').
 *
 * Feed 5A A5 03 82 4F 4B ('O'=0x4F, 'K'=0x4B).
 * Verified: cmd == 0x82, is_ack == true, data_len == 0.
 */
ZTEST(protocol_parser, test_parse_write_ack_ok) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t raw[] = {0x5A, 0xA5, 0x03, 0x82, 0x4F, 0x4B};

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(raw) - 1; i++) {
		zassert_false(dgus2_protocol_parse_byte(&parser, raw[i], &frame),
		    "Premature completion at index %zu", i);
	}

	bool completed = dgus2_protocol_parse_byte(&parser, raw[sizeof(raw) - 1], &frame);

	zassert_true(completed, "Frame should complete on final byte");
	zassert_equal(frame.cmd, 0x82, "Command should be 0x82");
	zassert_true(frame.is_ack, "is_ack should be true");
	zassert_equal(frame.data_len, 0, "Data length should be 0");
}

/**
 * @brief Test parsing Write Register confirmation ACK ('OK').
 *
 * Feed 5A A5 03 80 4F 4B.
 * Verified: cmd == 0x80, is_ack == true, data_len == 0.
 */
ZTEST(protocol_parser, test_parse_write_reg_ack_ok) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t raw[] = {0x5A, 0xA5, 0x03, 0x80, 0x4F, 0x4B};

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(raw) - 1; i++) {
		zassert_false(dgus2_protocol_parse_byte(&parser, raw[i], &frame),
		    "Premature completion at index %zu", i);
	}

	bool completed = dgus2_protocol_parse_byte(&parser, raw[sizeof(raw) - 1], &frame);

	zassert_true(completed, "Frame should complete on final byte");
	zassert_equal(frame.cmd, 0x80, "Command should be 0x80");
	zassert_true(frame.is_ack, "is_ack should be true");
	zassert_equal(frame.data_len, 0, "Data length should be 0");
}

/**
 * @brief Test parsing Write Curve confirmation ACK ('OK').
 *
 * Feed 5A A5 03 84 4F 4B.
 * Verified: cmd == 0x84, is_ack == true, data_len == 0.
 */
ZTEST(protocol_parser, test_parse_write_curve_ack_ok) {
	struct dgus2_parser parser;
	struct dgus2_frame frame;
	const uint8_t raw[] = {0x5A, 0xA5, 0x03, 0x84, 0x4F, 0x4B};

	dgus2_protocol_parser_init(&parser);

	for (size_t i = 0; i < sizeof(raw) - 1; i++) {
		zassert_false(dgus2_protocol_parse_byte(&parser, raw[i], &frame),
		    "Premature completion at index %zu", i);
	}

	bool completed = dgus2_protocol_parse_byte(&parser, raw[sizeof(raw) - 1], &frame);

	zassert_true(completed, "Frame should complete on final byte");
	zassert_equal(frame.cmd, 0x84, "Command should be 0x84");
	zassert_true(frame.is_ack, "is_ack should be true");
	zassert_equal(frame.data_len, 0, "Data length should be 0");
}
