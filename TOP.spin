' by: Kwabena W. Agyeman - kwagyeman@openmv.io

con

  _CLKFREQ = 80_000_000
  _CLKMODE = XTAL1 + PLL16X

  NUM_SWD = 1 ' 5

  PROGRAM_BUTTON = 0 ' 20
  PROGRAM_STATUS = 26 ' 21

  SD_DO_PIN = 22
  SD_CLK_PIN = 23
  SD_DI_PIN = 24
  SD_CS_PIN = 25
  SD_WP_PIN = -1
  SD_CD_PIN = -1 ' SD_DI_PIN

dat

  swd_io_pin byte 14 ' 00, 04, 08, 12, 16
  swd_clk_pin byte 15 ' 01, 05, 09, 13, 17
  swd_res_pin byte 13 ' 02, 06, 10, 14, 18
  swd_led_pin byte 27 ' 03, 07, 11, 15, 19

  firmware_file_name byte "F1.BIN", 0 ' "OPENMV.BIN", 0 ' 8.3 file name

obj

  fat[NUM_SWD] : "SD-MMC_FATEngine.spin"
  swd[NUM_SWD] : "SWD.spin"
  utl : "utils.spin"

con

  #0, SWD_IDLE, SWD_BEGIN_PROGRAMING, SWD_PROGRAMMING, SWD_BEGIN_VERIFYING, SWD_VERIFYING, SWD_FINISHED, SWD_FATAL_ERROR

var

  long board_state[NUM_SWD], remaining_blocks[NUM_SWD]

pub main | i, x, y, r

  fat.FATEngineStart(SD_DO_PIN, SD_CLK_PIN, SD_DI_PIN, SD_CS_PIN, SD_WP_PIN, SD_CD_PIN, -1, -1, -1)
  utl.start

  repeat
    x := utl.get_posedge(PROGRAM_BUTTON)

    y := true
    repeat i from 0 to constant(NUM_SWD - 1)
      y and= utl.get(swd_res_pin[i])
      case board_state[i]

        SWD_IDLE:
          if x
            board_state[i] := SWD_BEGIN_PROGRAMING

          ifnot utl.get(swd_res_pin[i])
            utl.set_freq(swd_led_pin[i], -1)

        SWD_BEGIN_PROGRAMING:
          r := \begin_programming(i)

          if r
            board_state[i] := SWD_FATAL_ERROR

        SWD_PROGRAMMING:
          r := \programming(i)

          if r
            board_state[i] := SWD_FATAL_ERROR

        SWD_BEGIN_VERIFYING:
          r := \begin_verifying(i)

          if r
            board_state[i] := SWD_FATAL_ERROR

        SWD_VERIFYING:
          r := \verifying(i)

          if r
            board_state[i] := SWD_FATAL_ERROR

        SWD_FINISHED:
          r := \finished(i)

          if r
            board_state[i] := SWD_FATAL_ERROR

        SWD_FATAL_ERROR:
          fatal_error(i)

    utl.set_freq(PROGRAM_STATUS, !y)

pri begin_programming(port) ' may abort... returns 0 normally - non-zero on abort

  fat[port].mountPartition(0)
  fat[port].openFile(@firmware_file_name, "R")

  swd[port].stop(swd_res_pin[port])
  swd[port].start(swd_io_pin[port], swd_clk_pin[port], swd_res_pin[port])

  remaining_blocks[port] := (fat[port].fileSize + 511) / 512

  if remaining_blocks[port]
    remaining_blocks[port] -= 1
    program_block(port)

    utl.set_freq(swd_led_pin[port], 10)

    board_state[port] := SWD_PROGRAMMING

  else
    abort 1

pri programming(port) ' may abort... returns 0 normally - non-zero on abort

  ifnot swd[port].busy

    if remaining_blocks[port]
      remaining_blocks[port] -= 1
      program_block(port)

    else
      board_state[port] := SWD_BEGIN_VERIFYING

pri program_block(port) | x ' may abort... returns 0 normally - non-zero on abort

  x := swd[port].internal_block_address

  longfill(x, 0, 128)
  fat[port].readData(x, 512)

  swd[port].write_block

pri begin_verifying(port) ' may abort... returns 0 normally - non-zero on abort

  fat[port].fileSeek(0)

  remaining_blocks[port] := (fat[port].fileSize + 511) / 512

  if remaining_blocks[port]
    remaining_blocks[port] -= 1
    swd[port].read_block

    board_state[port] := SWD_VERIFYING

  else
    abort 1

pri verifying(port) ' may abort... returns 0 normally - non-zero on abort

  ifnot swd[port].busy
    verify_block(port)

    if remaining_blocks[port]
      remaining_blocks[port] -= 1
      swd[port].read_block

    else
      board_state[port] := SWD_FINISHED

pri verify_block(port) | i, x, data[128] ' may abort... returns 0 normally - non-zero on abort

  x := swd[port].internal_block_address

  longfill(@data, 0, 128)
  fat[port].readData(@data, 512)

  repeat i from 0 to 127
    if data[i] <> long[x][i]
      abort 1

pri finished(port) ' may abort... returns 0 normally - non-zero on abort

  swd[port].stop(swd_res_pin[port])

  fat[port].unmountPartition

  utl.set_freq(swd_led_pin[port], 0)

  board_state[port] := SWD_IDLE

pri fatal_error(port)

  utl.set_freq(swd_led_pin[port], 1)

  board_state[port] := SWD_IDLE
