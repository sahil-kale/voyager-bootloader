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
  uint8_t buffer[8];
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

  // run the bootloader for one more iteration
  CHECK_EQUAL(VOYAGER_ERROR_NONE, voyager_bootloader_run());

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

  /*
    // Generate a the ack packet to compare against
  uint8_t ack_packet[VOYAGER_DFU_ACK_MESSAGE_SIZE];
      voyager_private_generate_ack_message(VOYAGER_DFU_ERROR_PACKET_OVERRUN,
                                           ack_packet,
  VOYAGER_DFU_ACK_MESSAGE_SIZE);
  */
}

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
