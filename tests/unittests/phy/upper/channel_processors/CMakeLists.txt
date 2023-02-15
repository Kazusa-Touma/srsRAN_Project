#
# Copyright 2013-2022 Software Radio Systems Limited
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the distribution.
#

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)
set(TEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/test_data)

set_directory_properties(PROPERTIES LABELS "phy")

add_executable(pbch_encoder_test pbch_encoder_test.cpp)
target_link_libraries(pbch_encoder_test srsgnb_channel_processors srslog)
add_test_vector(pbch_encoder_test pbch_encoder_test_data.tar.gz "")

add_executable(pbch_modulator_test pbch_modulator_test.cpp)
target_link_libraries(pbch_modulator_test srsgnb_channel_processors srslog)
add_test_vector(pbch_modulator_test pbch_modulator_test_data.tar.gz "")

add_executable(pdcch_encoder_test pdcch_encoder_test.cpp)
target_link_libraries(pdcch_encoder_test srsgnb_channel_processors srslog)
add_test_vector(pdcch_encoder_test pdcch_encoder_test_data.tar.gz "")

add_executable(pdcch_modulator_test pdcch_modulator_test.cpp)
target_link_libraries(pdcch_modulator_test srsgnb_channel_processors srslog srsgnb_channel_modulation srsgnb_sequence_generators)
add_test_vector(pdcch_modulator_test pdcch_modulator_test_data.tar.gz "")

add_executable(pdcch_processor_unittest pdcch_processor_unittest.cpp)
target_link_libraries(pdcch_processor_unittest srsgnb_channel_processors srslog)
add_test(pdcch_processor_unittest pdcch_processor_unittest)

add_executable(pdcch_processor_vectortest pdcch_processor_vectortest.cpp)
target_link_libraries(pdcch_processor_vectortest srsgnb_channel_processors srslog gtest gtest_main)
add_test_vector(pdcch_processor_vectortest pdcch_processor_test_data.tar.gz "")

add_executable(pdsch_encoder_test pdsch_encoder_test.cpp)
target_link_libraries(pdsch_encoder_test srsgnb_channel_processors srslog)
add_test_vector(pdsch_encoder_test pdsch_encoder_test_data.tar.gz "")

add_executable(pdsch_modulator_test pdsch_modulator_test.cpp)
target_link_libraries(pdsch_modulator_test srsgnb_channel_processors srslog)
add_test_vector(pdsch_modulator_test pdsch_modulator_test_data.tar.gz "")

add_executable(pdsch_processor_unittest pdsch_processor_unittest.cpp)
target_link_libraries(pdsch_processor_unittest srsgnb_channel_processors srslog)
add_test(pdsch_processor_unittest pdsch_processor_unittest)

add_executable(pdsch_processor_validator_test pdsch_processor_validator_test.cpp)
target_link_libraries(pdsch_processor_validator_test
        srsgnb_channel_equalizer
        srsgnb_channel_processors
        srsgnb_phy_support
        srslog
        srsgnb_upper_phy_support
        gtest
        gtest_main)
add_test(pdsch_processor_validator_test pdsch_processor_validator_test)

add_executable(prach_detector_unittest prach_detector_unittest.cpp)
target_link_libraries(prach_detector_unittest srsgnb_channel_processors srsgnb_generic_funcs srslog gtest gtest_main)
add_test(prach_detector_unittest prach_detector_unittest)

add_executable(prach_detector_vectortest prach_detector_vectortest.cpp)
target_link_libraries(prach_detector_vectortest srsgnb_channel_processors srsgnb_generic_funcs srsgnb_phy_support srslog gtest gtest_main)
add_test(prach_detector_vectortest prach_detector_vectortest)

add_executable(prach_generator_vectortest prach_generator_vectortest.cpp)
target_link_libraries(prach_generator_vectortest srsgnb_channel_processors srsgnb_generic_funcs srslog gtest gtest_main)
add_test_vector(prach_generator_vectortest prach_generator_test_data.tar.gz "")

add_executable(pucch_demodulator_format2_test pucch_demodulator_format2_test.cpp)
target_link_libraries(pucch_demodulator_format2_test
        srsgnb_channel_processors
        srsgnb_channel_equalizer
        srsgnb_phy_support
        srslog
        srsgnb_upper_phy_support
        gtest
        gtest_main)
add_test_vector(pucch_demodulator_format2_test pucch_demodulator_format2_test_data.tar.gz "")

add_executable(pucch_detector_test pucch_detector_test.cpp)
target_link_libraries(pucch_detector_test srsgnb_channel_processors srsgnb_channel_equalizer srslog gtest gtest_main)
add_test_vector(pucch_detector_test pucch_detector_test_data.tar.gz "")

add_executable(pucch_processor_format1_unittest pucch_processor_format1_unittest.cpp)
target_link_libraries(pucch_processor_format1_unittest
        srsgnb_channel_equalizer
        srsgnb_channel_processors
        srslog
        gtest
        gtest_main)
add_test(pucch_processor_format1_unittest pucch_processor_format1_unittest)

add_executable(pucch_processor_format1_vectortest pucch_processor_format1_vectortest.cpp)
target_link_libraries(pucch_processor_format1_vectortest
        srsgnb_channel_equalizer
        srsgnb_phy_support
        srsgnb_channel_processors
        srslog
        gtest
        gtest_main)
add_test_vector(pucch_processor_format1_vectortest pucch_processor_format1_test_data.tar.gz "")

add_executable(pucch_processor_format2_vectortest pucch_processor_format2_vectortest.cpp)
target_link_libraries(pucch_processor_format2_vectortest
        srsgnb_channel_processors
        srsgnb_channel_equalizer
        srsgnb_phy_support
        gtest
        gtest_main)
add_test(pucch_processor_format2_vectortest pucch_processor_format2_vectortest)
add_test_vector(pucch_processor_format2_vectortest pucch_processor_format2_test_data.tar.gz "")

add_executable(pucch_processor_validator_format2_test pucch_processor_validator_format2_test.cpp)
target_link_libraries(pucch_processor_validator_format2_test
        srsgnb_channel_equalizer
        srsgnb_channel_processors
        srsgnb_phy_support
        srslog
        srsgnb_upper_phy_support
        gtest
        gtest_main)
add_test(pucch_processor_validator_format2_test pucch_processor_validator_format2_test)

add_executable(pucch_processor_validator_format1_test pucch_processor_validator_format1_test.cpp)
target_link_libraries(pucch_processor_validator_format1_test
        srsgnb_channel_equalizer
        srsgnb_channel_processors
        srsgnb_phy_support
        srslog
        srsgnb_upper_phy_support
        gtest
        gtest_main)
add_test(pucch_processor_validator_format1_test pucch_processor_validator_format1_test)

add_executable(pusch_decoder_test pusch_decoder_test.cpp)
target_link_libraries(pusch_decoder_test srsgnb_channel_processors srslog srsgnb_upper_phy_support)
add_test_vector(pusch_decoder_test pusch_decoder_test_data.tar.gz "-e0 -i2")
add_test(pusch_decoder_early_stop_test pusch_decoder_test -e1 -i6)

add_executable(pusch_demodulator_vectortest pusch_demodulator_vectortest.cpp)
target_link_libraries(pusch_demodulator_vectortest
        srsgnb_channel_processors
        srsgnb_channel_equalizer
        srsgnb_phy_support
        srslog
        srsgnb_upper_phy_support
        gtest
        gtest_main)
add_test_vector(pusch_demodulator_vectortest pusch_demodulator_test_data.tar.gz "")

add_executable(pusch_processor_validator_test pusch_processor_validator_test.cpp)
target_link_libraries(pusch_processor_validator_test
        srsgnb_channel_equalizer
        srsgnb_channel_processors
        srsgnb_phy_support
        srslog
        srsgnb_upper_phy_support
        gtest
        gtest_main)
add_test(pusch_processor_validator_test pusch_processor_validator_test)

add_executable(pusch_processor_vectortest pusch_processor_vectortest.cpp)
target_link_libraries(pusch_processor_vectortest
        srsgnb_channel_equalizer
        srsgnb_channel_processors
        srsgnb_phy_support
        srslog
        srsgnb_upper_phy_support
        gtest
        gtest_main)
add_test_vector(pusch_processor_vectortest pusch_processor_test_data.tar.gz "")

add_executable(pusch_processor_unittest pusch_processor_unittest.cpp)
target_link_libraries(pusch_processor_unittest srsgnb_channel_processors srslog gtest gtest_main)
add_test(pusch_processor_unittest pusch_processor_unittest)

add_executable(ssb_processor_unittest ssb_processor_unittest.cpp)
target_link_libraries(ssb_processor_unittest srsgnb_channel_processors srslog)
add_test(ssb_processor_unittest ssb_processor_unittest)

add_executable(ssb_processor_vectortest ssb_processor_vectortest.cpp)
target_link_libraries(ssb_processor_vectortest srsgnb_channel_processors srsgnb_lower_phy_modulation srslog)
add_test_vector(ssb_processor_vectortest ssb_processor_test_data.tar.gz "")

add_executable(uci_decoder_test uci_decoder_test.cpp)
target_link_libraries(uci_decoder_test srsgnb_channel_processors srsgnb_channel_coding srsgnb_upper_phy_support)
add_test_vector(uci_decoder_test uci_decoder_test_data.tar.gz "")

add_executable(ulsch_demultiplex_test ulsch_demultiplex_test.cpp)
target_link_libraries(ulsch_demultiplex_test srsgnb_channel_processors srsgnb_channel_coding gtest gtest_main)
add_test_vector(ulsch_demultiplex_test ulsch_demultiplex_test_data.tar.gz "")