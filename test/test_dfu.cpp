#include "CppUTest/TestHarness.h"
#include "voyager_host_message_generator.h"
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include "mock_dfu.h"
#include "mock_nvm.h"
#include "voyager.h"
#include "voyager_private.h"
}

// create a test group
TEST_GROUP(test_dfu){

    void setup(){mock().clear();
mock_nvm_data_t *nvm_data = mock_nvm_get_data();
nvm_data->app_reset_vector_address = (uintptr_t)&mock_reset_vector;

uint8_t *mock_app_start_address = mock_dfu_get_flash();
nvm_data->app_start_address = (uintptr_t)mock_app_start_address;
nvm_data->app_end_address = nvm_data->app_start_address + FAKE_FLASH_SIZE * 2;
nvm_data->app_size = FAKE_FLASH_SIZE;
mock_dfu_init();
}

void teardown() { mock().clear(); }
}
;

// Test the packet creating function

// Test that a start request without enter dfu request is rejected and sends an
// error packet to the host
TEST(test_dfu, start_request_without_enter_dfu_request) {
  // Clear the mock
  mock().clear();
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Create a start request packet
  uint8_t buffer[8];
  voyager_host_message_generator_generate_start_request(buffer, 8, 1, 1);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Check the pending data flag is set
  CHECK_EQUAL(true, voyager_private_get_data()->pending_data);

  // Check that the packet size is set
  CHECK_EQUAL(8U, voyager_private_get_data()->packet_size);

  // Generate the comparison ack packet
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
  voyager_private_generate_ack_message(
      VOYAGER_DFU_ERROR_ENTER_DFU_NOT_REQUESTED, NULL, ack_packet,
      VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // Run the bootloader
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check that the bootloader is still in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Check that the pending data flag is cleared
  CHECK_EQUAL(false, voyager_private_get_data()->pending_data);

  // Check the mock expectations
  mock().checkExpectations();
}

// Test that a start request with enter dfu request is accepted and sends an ack
// as well as moves to DFU receive state
TEST(test_dfu, start_request_with_enter_dfu_request) {
  // Clear the mock
  mock().clear();
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // get the mock nvm data and set the app size and crc to 0
  mock_nvm_data_t *nvm_data = mock_nvm_get_data();
  nvm_data->app_crc = 0;
  nvm_data->app_size = 0;
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Request to enter DFU mode
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_request(VOYAGER_REQUEST_ENTER_DFU));

  // Create a start request packet
  uint8_t buffer[8] = {0};
  voyager_host_message_generator_generate_start_request(buffer, 8, 0XDEBEAD,
                                                        0xBEEFDEAD);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Check the pending data flag is set
  CHECK_EQUAL(true, voyager_private_get_data()->pending_data);

  // Check that the packet size is set
  CHECK_EQUAL(8U, voyager_private_get_data()->packet_size);

  // Generate the comparison ack packet
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
  // compute the CRC of the last 7 bytes of the buffer sent by the host
  // and pass it as the payload to the generate ack message function
  uint32_t crc = voyager_host_calculate_crc(&buffer[1], 7);
  uint8_t crc_buffer[4] = {0};
  crc_buffer[0] = (crc >> 24) & 0xFF;
  crc_buffer[1] = (crc >> 16) & 0xFF;
  crc_buffer[2] = (crc >> 8) & 0xFF;
  crc_buffer[3] = crc & 0xFF;
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_NONE, crc_buffer,
                                       ack_packet,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // Run the bootloader
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check that the nvm app size and crc are set
  CHECK_EQUAL(0xBEEFDEAD, mock_nvm_get_data()->app_crc);
  CHECK_EQUAL(0xDEBEAD, mock_nvm_get_data()->app_size);

  // Check that the pending data flag is cleared
  CHECK_EQUAL(false, voyager_private_get_data()->pending_data);

  // Check the mock expectations
  mock().checkExpectations();

  // Check that the desired state is DFU receive
  CHECK_EQUAL(VOYAGER_STATE_DFU_RECEIVE, voyager_private_get_desired_state());

  // Expect the bootloader hal erase flash function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_hal_erase_flash")
      .withParameter("start_address", mock_nvm_get_data()->app_start_address)
      .withParameter("end_address", mock_nvm_get_data()->app_end_address)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check the mock expectations
  mock().checkExpectations();

  // check that the current state is DFU receive
  CHECK_EQUAL(VOYAGER_STATE_DFU_RECEIVE, voyager_bootloader_get_state());
}

// Test a packet null pointer
TEST(test_dfu, start_request_null_pointer) {
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_INVALID_ARGUMENT,
              voyager_bootloader_process_receieved_packet(NULL, 8));

  // Check that the bootloader is still in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
}

// Test a packet over the size
TEST(test_dfu, start_request_over_size) {
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Create a start request packet
  uint8_t buffer[8];
  voyager_host_message_generator_generate_start_request(buffer, 8, 1, 1);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_INVALID_ARGUMENT,
              voyager_bootloader_process_receieved_packet(
                  buffer, VOYAGER_BOOTLOADER_MAX_RECEIVE_PACKET_SIZE + 1));

  // Check that the bootloader is still in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
}

// Test a packet overrun (i.e. call process received data twice with valid data
// without calling run)
TEST(test_dfu, start_request_overrun) {
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Request to enter DFU mode
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_request(VOYAGER_REQUEST_ENTER_DFU));

  // Create a start request packet
  uint8_t buffer[8];
  voyager_host_message_generator_generate_start_request(buffer, 8, 1, 1);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Check the pending data flag is set
  CHECK_EQUAL(true, voyager_private_get_data()->pending_data);

  // Check that the packet size is set
  CHECK_EQUAL(8U, voyager_private_get_data()->packet_size);

  // Process the packet again
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Check that the bootloader is still in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Check that the packet overrun flag is set
  CHECK_EQUAL(true, voyager_private_get_data()->packet_overrun);

  // Generate a the ack packet to compare against
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE];
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_PACKET_OVERRUN, NULL,
                                       ack_packet,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // Run the bootloader
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check the mock expectations
  mock().checkExpectations();
}

// Test a full DFU transfer and jump to application
TEST(test_dfu, full_dfu_transfer_and_jump_to_application) {
  // Clear the mock
  mock().clear();
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // get the mock nvm data and set the app size and crc to 0
  mock_nvm_data_t *nvm_data = mock_nvm_get_data();
  nvm_data->app_crc = 0;
  nvm_data->app_size = 0;
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Request to enter DFU mode
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_request(VOYAGER_REQUEST_ENTER_DFU));

  // Create a start request packet
  uint8_t buffer[8] = {0};
  voyager_bootloader_app_crc_t app_crc = voyager_private_calculate_crc(
      (const void *)fake_flash_data_1, sizeof(fake_flash_data_1));
  voyager_bootloader_app_size_t app_size = sizeof(fake_flash_data_1);
  voyager_host_message_generator_generate_start_request(buffer, 8, app_size,
                                                        app_crc);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Generate the comparison ack packet
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
  // compute the CRC of the last 7 bytes of the buffer sent by the host
  // and pass it as the payload to the generate ack message function
  uint32_t crc = voyager_host_calculate_crc(&buffer[1], 7);
  uint8_t crc_buffer[4] = {0};
  crc_buffer[0] = (crc >> 24) & 0xFF;
  crc_buffer[1] = (crc >> 16) & 0xFF;
  crc_buffer[2] = (crc >> 8) & 0xFF;
  crc_buffer[3] = crc & 0xFF;
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_NONE, crc_buffer,
                                       ack_packet,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check the mock expectations
  mock().checkExpectations();

  // Check that the nvm app size and crc are set
  CHECK_EQUAL(app_crc, mock_nvm_get_data()->app_crc);
  CHECK_EQUAL(app_size, mock_nvm_get_data()->app_size);

  // Check that the desired state is DFU receive
  CHECK_EQUAL(VOYAGER_STATE_DFU_RECEIVE, voyager_private_get_desired_state());

  // Expect the bootloader hal erase flash function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_hal_erase_flash")
      .withParameter("start_address", mock_nvm_get_data()->app_start_address)
      .withParameter("end_address", mock_nvm_get_data()->app_end_address)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check the mock expectations
  mock().checkExpectations();

  // check that the current state is DFU receive
  CHECK_EQUAL(VOYAGER_STATE_DFU_RECEIVE, voyager_bootloader_get_state());

  // for half of the size, write using 2 byte packets, just for funsies
  size_t written_size = 0U;
  while (written_size != sizeof(fake_flash_data_1)) {
    size_t chunk_size = 2U;
    if (written_size > sizeof(fake_flash_data_1) / 2) {
      chunk_size = 16U; // write the rest in 16 byte chunks
    }

    if (chunk_size > sizeof(fake_flash_data_1) - written_size) {
      chunk_size = sizeof(fake_flash_data_1) - written_size;
    }

    // Create a data packet
    size_t bytes_written_to_packet_buffer =
        voyager_host_message_generator_generate_data_packet(
            buffer, chunk_size + 2U, &fake_flash_data_1[written_size],
            chunk_size, (written_size == 0U));

    // Process the packet
    CHECK_EQUAL(VOYAGER_ERROR_NONE,
                voyager_bootloader_process_receieved_packet(
                    buffer, bytes_written_to_packet_buffer));

    // Generate the comparison ack packet, which will generate a CRC of all the
    // bytes in the buffer except the first byte (the message ID)
    uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
    // compute the CRC of the last 7 bytes of the buffer sent by the host
    // and pass it as the payload to the generate ack message function
    uint32_t crc = voyager_host_calculate_crc(&buffer[1], chunk_size + 1U);
    uint8_t crc_buffer[4] = {0};
    crc_buffer[0] = (crc >> 24) & 0xFF;
    crc_buffer[1] = (crc >> 16) & 0xFF;
    crc_buffer[2] = (crc >> 8) & 0xFF;
    crc_buffer[3] = crc & 0xFF;

    voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_NONE, crc_buffer,
                                         ack_packet,
                                         VOYAGER_DFU_ACK_MESSAGE_SIZE);

    // Expect the bootloader send_to_host function to be called
    // and return VOYAGER_ERROR_NONE
    mock()
        .expectOneCall("voyager_bootloader_send_to_host")
        .withMemoryBufferParameter("data", ack_packet,
                                   VOYAGER_DFU_ACK_MESSAGE_SIZE)
        .andReturnValue(VOYAGER_ERROR_NONE);

    // Expect the bootloader hal write flash function to be called
    // and return VOYAGER_ERROR_NONE
    mock()
        .expectOneCall("voyager_bootloader_hal_write_flash")
        .withParameter("address",
                       mock_nvm_get_data()->app_start_address + written_size)
        .withMemoryBufferParameter("data", &fake_flash_data_1[written_size],
                                   chunk_size)
        .andReturnValue(VOYAGER_ERROR_NONE);

    // run the bootloader for one more iteration
    CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

    written_size += chunk_size;
    // Assert that the written size is equal to the voyager data bytes written
    CHECK_EQUAL(written_size, voyager_private_get_data()->bytes_written);

    // Check the mock expectations
    mock().checkExpectations();
  }

  // Check that the desired state is JUMP TO APP
  CHECK_EQUAL(VOYAGER_STATE_JUMP_TO_APP, voyager_private_get_desired_state());
}

// Test that an out of sequence packet generates an error packet and moves the
// bootloader back to idle state
TEST(test_dfu, out_of_sequence_data_packet) {
  // Clear the mock
  mock().clear();
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // get the mock nvm data and set the app size and crc to 0
  mock_nvm_data_t *nvm_data = mock_nvm_get_data();
  nvm_data->app_crc = 0;
  nvm_data->app_size = 0;
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Request to enter DFU mode
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_request(VOYAGER_REQUEST_ENTER_DFU));

  // Create a start request packet
  uint8_t buffer[8] = {0};
  voyager_bootloader_app_crc_t app_crc = voyager_private_calculate_crc(
      (const void *)fake_flash_data_1, sizeof(fake_flash_data_1));
  voyager_bootloader_app_size_t app_size = sizeof(fake_flash_data_1);
  voyager_host_message_generator_generate_start_request(buffer, 8, app_size,
                                                        app_crc);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Generate the comparison ack packet
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
  // compute the CRC of the last 7 bytes of the buffer sent by the host
  // and pass it as the payload to the generate ack message function
  uint32_t crc = voyager_host_calculate_crc(&buffer[1], 7);
  uint8_t crc_buffer[4] = {0};
  crc_buffer[0] = (crc >> 24) & 0xFF;
  crc_buffer[1] = (crc >> 16) & 0xFF;
  crc_buffer[2] = (crc >> 8) & 0xFF;
  crc_buffer[3] = crc & 0xFF;
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_NONE, crc_buffer,
                                       ack_packet,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check the mock expectations
  mock().checkExpectations();

  // Check that the nvm app size and crc are set
  CHECK_EQUAL(app_crc, mock_nvm_get_data()->app_crc);
  CHECK_EQUAL(app_size, mock_nvm_get_data()->app_size);

  // Check that the desired state is DFU receive
  CHECK_EQUAL(VOYAGER_STATE_DFU_RECEIVE, voyager_private_get_desired_state());

  // Expect the bootloader hal erase flash function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_hal_erase_flash")
      .withParameter("start_address", mock_nvm_get_data()->app_start_address)
      .withParameter("end_address", mock_nvm_get_data()->app_end_address)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check the mock expectations
  mock().checkExpectations();

  // check that the current state is DFU receive
  CHECK_EQUAL(VOYAGER_STATE_DFU_RECEIVE, voyager_bootloader_get_state());

  // generate the data packet with the reset sequence set to true
  size_t bytes_written = 0U;
  bytes_written = voyager_host_message_generator_generate_data_packet(
      buffer, 8, &fake_flash_data_1[0], 2U, true);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_process_receieved_packet(
                                      buffer, bytes_written));

  // Generate the comparison ack packet, which will generate a CRC of all the
  // bytes in the buffer except the first byte (the message ID)
  uint8_t ack_packet_1[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
  // compute the CRC of the last 7 bytes of the buffer sent by the host
  // and pass it as the payload to the generate ack message function
  crc = voyager_host_calculate_crc(&buffer[1], bytes_written - 1U);
  crc_buffer[0] = (crc >> 24) & 0xFF;
  crc_buffer[1] = (crc >> 16) & 0xFF;
  crc_buffer[2] = (crc >> 8) & 0xFF;
  crc_buffer[3] = crc & 0xFF;

  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_NONE, crc_buffer,
                                       ack_packet_1,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet_1,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // Expect the bootloader hal write flash function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_hal_write_flash")
      .withParameter("address", mock_nvm_get_data()->app_start_address)
      .withMemoryBufferParameter("data", &fake_flash_data_1[0], 2U)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // check the expectations
  mock().checkExpectations();

  // Send the same packet again
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_process_receieved_packet(
                                      buffer, bytes_written));

  // Generate the comparison ack packet, which will contain the out of sequence
  // error

  uint8_t ack_packet_2[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_OUT_OF_SEQUENCE, NULL,
                                       ack_packet_2,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  // and return VOYAGER_ERROR_NONE

  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet_2,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // check the expectations
  mock().checkExpectations();

  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check that the bootloader is now in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
}

TEST(test_dfu, packet_overrun_dfu_receive) {
  // Clear the mock
  mock().clear();
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // get the mock nvm data and set the app size and crc to 0
  mock_nvm_data_t *nvm_data = mock_nvm_get_data();
  nvm_data->app_crc = 0;
  nvm_data->app_size = 0;
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Request to enter DFU mode
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_request(VOYAGER_REQUEST_ENTER_DFU));

  // Create a start request packet
  uint8_t buffer[8] = {0};
  voyager_bootloader_app_crc_t app_crc = voyager_private_calculate_crc(
      (const void *)fake_flash_data_1, sizeof(fake_flash_data_1));
  voyager_bootloader_app_size_t app_size = sizeof(fake_flash_data_1);
  voyager_host_message_generator_generate_start_request(buffer, 8, app_size,
                                                        app_crc);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Generate the comparison ack packet
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
  // compute the CRC of the last 7 bytes of the buffer sent by the host
  // and pass it as the payload to the generate ack message function
  uint32_t crc = voyager_host_calculate_crc(&buffer[1], 7);
  uint8_t crc_buffer[4] = {0};
  crc_buffer[0] = (crc >> 24) & 0xFF;
  crc_buffer[1] = (crc >> 16) & 0xFF;
  crc_buffer[2] = (crc >> 8) & 0xFF;
  crc_buffer[3] = crc & 0xFF;
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_NONE, crc_buffer,
                                       ack_packet,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check the mock expectations
  mock().checkExpectations();

  // Check that the nvm app size and crc are set
  CHECK_EQUAL(app_crc, mock_nvm_get_data()->app_crc);
  CHECK_EQUAL(app_size, mock_nvm_get_data()->app_size);

  // Check that the desired state is DFU receive
  CHECK_EQUAL(VOYAGER_STATE_DFU_RECEIVE, voyager_private_get_desired_state());

  // Expect the bootloader hal erase flash function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_hal_erase_flash")
      .withParameter("start_address", mock_nvm_get_data()->app_start_address)
      .withParameter("end_address", mock_nvm_get_data()->app_end_address)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check the mock expectations
  mock().checkExpectations();

  // check that the current state is DFU receive
  CHECK_EQUAL(VOYAGER_STATE_DFU_RECEIVE, voyager_bootloader_get_state());

  // generate the data packet with the reset sequence set to true
  size_t bytes_written = 0U;
  bytes_written = voyager_host_message_generator_generate_data_packet(
      buffer, 8, &fake_flash_data_1[0], 2U, true);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_process_receieved_packet(
                                      buffer, bytes_written));

  // Send the same packet again
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_process_receieved_packet(
                                      buffer, bytes_written));

  // Generate the comparison ack packet, which will contain the out of sequence
  // error

  uint8_t ack_packet_2[VOYAGER_DFU_ACK_MESSAGE_SIZE] = {0};
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_PACKET_OVERRUN, NULL,
                                       ack_packet_2,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet_2,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // check the expectations
  mock().checkExpectations();

  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check that the bootloader is now in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
}

// Test that a data packet without a start request is rejected and sends an
// error
TEST(test_dfu, data_but_not_started) {
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Create a data packet
  uint8_t buffer[8];
  voyager_host_message_generator_generate_data_packet(buffer, 8, NULL, 0,
                                                      false);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Check the pending data flag is set
  CHECK_EQUAL(true, voyager_private_get_data()->pending_data);

  // Check that the packet size is set
  CHECK_EQUAL(8U, voyager_private_get_data()->packet_size);

  // Check that the bootloader is still in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Generate the ack packet to compare against
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE];
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_OUT_OF_SEQUENCE, NULL,
                                       ack_packet,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // Run the bootloader
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check that the bootloader is still in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());
}

#if 0 // TODO: Improvement. Deemed not a priority for now as the compiler will
      // catch this
// Test that a start request with size too big is rejected and sends an error
TEST(test_dfu, size_too_big) {
  // Ensure that the init function does not return an error
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_init());
  // Ensure that the bootloader is off after init
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Create a start request packet
  uint8_t buffer[8] = {0};
  // grab the start and end address from the mock nvm data
  mock_nvm_data_t *nvm_data = mock_nvm_get_data();
  voyager_bootloader_app_size_t app_size =
      nvm_data->app_end_address - nvm_data->app_start_address + 1;
  voyager_host_message_generator_generate_start_request(buffer, 8, app_size + 1,
                                                        1);

  // Process the packet
  CHECK_EQUAL(VOYAGER_ERROR_NONE,
              voyager_bootloader_process_receieved_packet(buffer, 8));

  // Check the pending data flag is set
  CHECK_EQUAL(true, voyager_private_get_data()->pending_data);

  // Check that the packet size is set
  CHECK_EQUAL(8U, voyager_private_get_data()->packet_size);

  // Check that the bootloader is still in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Generate the ack packet to compare against
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE];
  voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_SIZE_TOO_LARGE, NULL,
                                       ack_packet,
                                       VOYAGER_DFU_ACK_MESSAGE_SIZE);

  // Expect the bootloader send_to_host function to be called
  // and return VOYAGER_ERROR_NONE
  mock()
      .expectOneCall("voyager_bootloader_send_to_host")
      .withMemoryBufferParameter("data", ack_packet,
                                 VOYAGER_DFU_ACK_MESSAGE_SIZE)
      .andReturnValue(VOYAGER_ERROR_NONE);

  // Run the bootloader
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

  // Check that the bootloader is still in idle
  CHECK_EQUAL(VOYAGER_STATE_IDLE, voyager_bootloader_get_state());

  // Check the mock expectations
  mock().checkExpectations();
}
#endif

// Test packet overrun in DFU receive state generates error packet and moves to
// IDLE

// Test the packet unpack function
TEST(test_dfu, start_request_unpack) {
  // Create a start request packet
  uint8_t buffer[8];
  voyager_host_message_generator_generate_start_request(buffer, 8, 0XADBEEF,
                                                        0xDEADBEEF);

  // Unpack the packet
  voyager_message_t message = voyager_private_unpack_message(buffer, 8);

  // check that the app CRC and size are correct
  CHECK_EQUAL(0xDEADBEEF, message.start_packet_data.app_crc);
  CHECK_EQUAL(0xADBEEF, message.start_packet_data.app_size);
}
