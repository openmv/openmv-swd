' by: Kwabena W. Agyeman - kwagyeman@openmv.io

con

  _CLKFREQ = 80_000_000
  _CLKMODE = XTAL1 + PLL16X

  NUM_SWD = 5
  NUM_SEL = 4

  HEART_BEAT_LED = 26'23

  SD_DO_PIN = 22'24
  SD_CLK_PIN = 23'25
  SD_DI_PIN = 24'26
  SD_CS_PIN = 25'27
  SD_WP_PIN = -1
  SD_CD_PIN = -1

  TX_PIN = 30
  RX_PIN = 31

  BAUD_RATE = 115200

var

  long hb_cnt, hb_state, command_buf, command_state

dat

  swd_io_pin byte 5, 8, 11, 14, 2'17
  swd_clk_pin byte 4, 7, 10, 13, 1'16
  swd_res_pin byte 6, 9, 12, 15, 3'18

  swd_sel_pin byte 0, 0, 0, 0'3, 2, 1, 0
  swd_pwr_pin byte 0, 0, 0, 0'19, 20, 21, 22

  m4_firmware_file_name byte "OPENMV2/OPENMV.BIN", 0 ' 8.3 file name
  m7_firmware_file_name byte "OPENMV3/OPENMV.BIN", 0 ' 8.3 file name

obj

  com : "Full-Duplex_COMEngine.spin"
  fat[NUM_SWD] : "SD-MMC_FATEngine.spin"
  swd[NUM_SWD] : "SWD.spin"

con

  #0, SWD_IDLE, SWD_BEGIN_INITING, SWD_BEGIN_PROGRAMMING, SWD_PROGRAMMING, SWD_VERIFYING, SWD_FINISHED, SWD_FATAL_ERROR

var

  long board_state[NUM_SWD], remaining_blocks[NUM_SWD], debounce_cnt[NUM_SWD], debounce_shift[NUM_SWD], debounce_state[NUM_SWD]

pub main | i, x, r

  dira[HEART_BEAT_LED] := 1
  repeat i from 0 to constant(NUM_SEL - 1)
    outa[swd_sel_pin[i]] := 1

  com.COMEngineStart(RX_PIN, TX_PIN, BAUD_RATE)
  fat.FATEngineStart(SD_DO_PIN, SD_CLK_PIN, SD_DI_PIN, SD_CS_PIN, SD_WP_PIN, SD_CD_PIN, -1, -1, -1)

  hb_cnt := cnt
  repeat i from 0 to constant(NUM_SWD - 1)
    debounce_cnt[i] := cnt

  repeat
    x := 0

    repeat while com.receivedNumber
      command_buf := (command_buf << 8) | com.readByte
      case command_buf

        $53_57_44_30: ' Program SWD 0
          if debounce_reset(0)
            x := constant(0 + 1)
          else
            com.writeString(string("SWD0 [Not Ready] 0%", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_31: ' Program SWD 1
          if debounce_reset(1)
            x := constant(1 + 1)
          else
            com.writeString(string("SWD1 [Not Ready] 0%", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_32: ' Program SWD 2
          if debounce_reset(2)
            x := constant(2 + 1)
          else
            com.writeString(string("SWD2 [Not Ready] 0%", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_33: ' Program SWD 3
          if debounce_reset(3)
            x := constant(3 + 1)
          else
            com.writeString(string("SWD3 [Not Ready] 0%", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_34: ' Program SWD 4
          if debounce_reset(4)
            x := constant(4 + 1)
          else
            com.writeString(string("SWD4 [Not Ready] 0%", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_35: ' Activate row 0
          kill_port
          if command_state~ <> constant(0 + 1)
            command_state := constant(0 + 1)
            dira[swd_sel_pin[0]] := 1
            dira[swd_pwr_pin[0]] := 1
            waitcnt(clkfreq + cnt)
            com.writeString(string("ROW0 On", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_36: ' Activate row 1
          kill_port
          if command_state~ <> constant(1 + 1)
            command_state := constant(1 + 1)
            dira[swd_sel_pin[1]] := 1
            dira[swd_pwr_pin[1]] := 1
            waitcnt(clkfreq + cnt)
            com.writeString(string("ROW1 On", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_37: ' Activate row 2
          kill_port
          if command_state~ <> constant(2 + 1)
            command_state := constant(2 + 1)
            dira[swd_sel_pin[2]] := 1
            dira[swd_pwr_pin[2]] := 1
            waitcnt(clkfreq + cnt)
            com.writeString(string("ROW2 On", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_38: ' Activate row 3
          kill_port
          if command_state~ <> constant(3 + 1)
            command_state := constant(3 + 1)
            dira[swd_sel_pin[3]] := 1
            dira[swd_pwr_pin[3]] := 1
            waitcnt(clkfreq + cnt)
            com.writeString(string("ROW3 On", com#Carriage_Return, com#Line_Feed))
          quit

        $53_57_44_39: ' Ping
          com.writeString(string("Hello World - v2.1.0", com#Carriage_Return, com#Line_Feed))
          quit

    repeat i from 0 to constant(NUM_SWD - 1)
      debounce_reset(i)
      case board_state[i]

        SWD_IDLE:
          if x == (i + 1)
            board_state[i] := SWD_BEGIN_INITING

        SWD_BEGIN_INITING:
          r := \begin_initing(i)

          if r
            board_state[i] := SWD_FATAL_ERROR

        SWD_BEGIN_PROGRAMMING:
          r := \begin_programming(i)

          if r
            board_state[i] := SWD_FATAL_ERROR

        SWD_PROGRAMMING:
          r := \programming(i)

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

    heart_beat

pri kill_port | x

  if command_state
    x := command_state - 1
    dira[swd_sel_pin[x]] := 0
    dira[swd_pwr_pin[x]] := 0
    waitcnt(clkfreq + cnt)
    com.writeString(string("ROW"))
    com.writeString(DECOut(x))
    com.writeString(string(" Off", com#Carriage_Return, com#Line_Feed))

pri heart_beat | i

  if (cnt - hb_cnt) > (clkfreq / 10)
    hb_cnt := cnt ' don't care if we loose ticks

    case hb_state
      1, 2, 4, 5:
        !outa[HEART_BEAT_LED]
      7:
        repeat i from 0 to constant(NUM_SWD - 1)
          if board_state[i] == SWD_BEGIN_PROGRAMMING
            com.writeString(string("SWD"))
            com.writeString(DECOut(i))
            com.writeString(string(" [Erasing] 0%", com#Carriage_Return, com#Line_Feed))
          if board_state[i] == SWD_FINISHED
            com.writeString(string("SWD"))
            com.writeString(DECOut(i))
            com.writeString(string(" [Testing] 0%", com#Carriage_Return, com#Line_Feed))

    if ++hb_state == 10
      hb_state := 0

pri debounce_reset(port)

  if (cnt - debounce_cnt[port]) > (clkfreq / 1000)
    debounce_cnt[port] := cnt ' don't care if we loose ticks
    debounce_shift[port] += debounce_shift[port] + ina[swd_res_pin[port]]

    ifnot !debounce_shift[port]
      debounce_state[port] := true

    ifnot debounce_shift[port]
      debounce_state[port] := false

  return debounce_state[port]

pri begin_initing(port) ' may abort... returns 0 normally - non-zero on abort

  swd[port].start(swd_io_pin[port], swd_clk_pin[port], swd_res_pin[port])
  swd[port].init

  com.writeString(string("SWD"))
  com.writeString(DECOut(port))
  com.writeString(string(" [Erasing] 0%", com#Carriage_Return, com#Line_Feed))

  board_state[port] := SWD_BEGIN_PROGRAMMING

pri begin_programming(port) ' may abort... returns 0 normally - non-zero on abort

  ifnot swd[port].busy

    fat[port].mountPartition(0)
    case swd[port].get_device
      swd#M4_TARGET_ID_CODE_1:
        fat[port].openFile(@m4_firmware_file_name, "R")
      swd#M7_TARGET_ID_CODE_1:
        fat[port].openFile(@m7_firmware_file_name, "R")

    remaining_blocks[port] := (fat[port].fileSize + 511) >> 9

    if remaining_blocks[port]
      remaining_blocks[port] -= 1
      program_block(port)

      com.writeString(string("SWD"))
      com.writeString(DECOut(port))
      com.writeString(string(" [Programing] 0%", com#Carriage_Return, com#Line_Feed))

      board_state[port] := SWD_PROGRAMMING

    else
      abort 1

pri programming(port) | x ' may abort... returns 0 normally - non-zero on abort

  ifnot swd[port].busy

    x := (fat[port].fileSize + 511) >> 9

    com.writeString(string("SWD"))
    com.writeString(DECOut(port))
    com.writeString(string(" [Programing] "))
    com.writeString(DECOut(((x - remaining_blocks[port]) * 100) / x))
    com.writeString(string("%", com#Carriage_Return, com#Line_Feed))

    if remaining_blocks[port]
      remaining_blocks[port] -= 1
      program_block(port)

    else
      fat[port].fileSeek(0)

      remaining_blocks[port] := (fat[port].fileSize + 511) >> 9

      if remaining_blocks[port]
        remaining_blocks[port] -= 1
        swd[port].read_block

        com.writeString(string("SWD"))
        com.writeString(DECOut(port))
        com.writeString(string(" [Verifying] 0%", com#Carriage_Return, com#Line_Feed))

        board_state[port] := SWD_VERIFYING

      else
        abort 1

pri program_block(port) | x ' may abort... returns 0 normally - non-zero on abort

  x := swd[port].internal_block_address

  longfill(x, 0, 128)
  fat[port].readData(x, 512)

  swd[port].write_block

pri verifying(port) | x ' may abort... returns 0 normally - non-zero on abort

  ifnot swd[port].busy
    verify_block(port)

    x := (fat[port].fileSize + 511) >> 9

    com.writeString(string("SWD"))
    com.writeString(DECOut(port))
    com.writeString(string(" [Verifying] "))
    com.writeString(DECOut(((x - remaining_blocks[port]) * 100) / x))
    com.writeString(string("%", com#Carriage_Return, com#Line_Feed))

    if remaining_blocks[port]
      remaining_blocks[port] -= 1
      swd[port].read_block

    else
      swd[port].fini(swd_res_pin[port])

      com.writeString(string("SWD"))
      com.writeString(DECOut(port))
      com.writeString(string(" [Testing] 0%", com#Carriage_Return, com#Line_Feed))

      board_state[port] := SWD_FINISHED

pri verify_block(port) | i, x, data[128] ' may abort... returns 0 normally - non-zero on abort

  x := swd[port].internal_block_address

  longfill(@data, 0, 128)
  fat[port].readData(@data, 512)

  repeat i from 0 to 127
    if data[i] <> long[x][i]
      abort 1

pri finished(port) ' may abort... returns 0 normally - non-zero on abort

  ifnot swd[port].busy

    fat[port].unmountPartition
    swd[port].stop(swd_res_pin[port])

    com.writeString(string("SWD"))
    com.writeString(DECOut(port))
    com.writeString(string(" [Done "))
    case swd[port].get_device
      swd#M4_TARGET_ID_CODE_1:
        com.writeString(string("M4"))
      swd#M7_TARGET_ID_CODE_1:
        com.writeString(string("M7"))
    com.writeString(string(":"))
    com.writeString(HEXOut(long[swd[port].get_id][2]))
    com.writeString(HEXOut(long[swd[port].get_id][1]))
    com.writeString(HEXOut(long[swd[port].get_id][0]))
    com.writeString(string("] 100%", com#Carriage_Return, com#Line_Feed))

    board_state[port] := SWD_IDLE

pri fatal_error(port)

  com.writeString(string("SWD"))
  com.writeString(DECOut(port))
  com.writeString(string(" [Error] 0%", com#Carriage_Return, com#Line_Feed))

  board_state[port] := SWD_IDLE

CON _DEC_OUT_TEMP_STRING_SIZE = 3 ' Signed 10 digit string plus NULL character.

VAR long tokenStringPointer, tempString[_DEC_OUT_TEMP_STRING_SIZE] ' String processing variables.

PRI STRToken(stringPointer) ' Splits a string into tokens.

' Notes:
'
' StringPointer - The address of the string to tokenize. Zero to continue tokenizing.
'
' Returns a pointer to an individual token from the tokenized string.

  if(stringPointer)
    tokenStringPointer := stringPointer

  if(tokenStringPointer)
    tokenStringPointer := result := STRTrim(tokenStringPointer)

    stringPointer := " "

    if(byte[tokenStringPointer] == com#Quotation_Marks)
      result := (STRTrim(++tokenStringPointer))

      stringPointer := com#Quotation_Marks

    repeat while(byte[tokenStringPointer])
      if(byte[tokenStringPointer++] == stringPointer)
        byte[tokenStringPointer][-1] := com#Null
        quit

PRI STRTrim(stringPointer) ' Trims leading white space from a string.

' Notes:
'
' StringPointer - The address of the string to trim.
'
' Returns a pointer to the trimmed string.

  if(stringPointer)
    result := --stringPointer
    repeat ' Format saves two bytes.
    while(byte[++result] == " ")

PRI DECOut(integer) | sign ' Integer to string.

' Notes:
'
' Integer - The integer to convert.
'
' Returns a pointer to the converted integer string.

  longfill(@tempString, 0, _DEC_OUT_TEMP_STRING_SIZE)
  sign := (integer < 0) ' Store sign.

  repeat result from 10 to 1 ' Convert number.
    tempString.byte[result] := ((||(integer // 10)) + "0")
    integer /= 10

  result := @tempString ' Skip past leading zeros.
  repeat ' Format saves two bytes.
  while(byte[++result] == "0")
  result += (not(byte[result]))

  if(sign) ' Insert sign.
    byte[--result] := "-"

PRI DECIn(stringPointer) | sign ' String to integer.

' Notes:
'
' StringPointer - The string to convert.
'
' Returns the converted string's integer value.

  if(stringPointer := STRTrim(stringPointer))
    sign := ((byte[stringPointer] == "-") | 1)
    stringPointer -= ((sign == -1) or (byte[stringPointer] == "+")) ' Skip sign.

    if(byte[stringPointer] == "0")
      if((byte[stringPointer + 1] == "X") or (byte[stringPointer + 1] == "x"))
        return (HEXIn(stringPointer + 2) * sign)

    repeat (strsize(stringPointer) <# 10) ' Expect 10 digits.
      if((byte[stringPointer] < "0") or ("9" < byte[stringPointer]))
        quit ' Break on a non-digit.

      result := ((result * 10) + (byte[stringPointer++] & $F))
    result *= sign

PRI HEXOut(integer) ' Integer to string.

' Notes:
'
' Integer - The integer to convert.
'
' Returns a pointer to the converted integer string.

  bytemove(@tempString, string("00000000"), 9)

  repeat result from constant(7) to constant(0)
    tempString.byte[result] := lookupz((integer & $F): "0".."9", "A".."F")
    integer >>= 4

  return @tempString

PRI HEXIn(stringPointer) ' String to integer.

' Notes:
'
' StringPointer - The string to convert.
'
' Returns the converted string's integer value.

  if(stringPointer)

    repeat (strsize(stringPointer) <# 8)
      ifnot(("0" =< byte[stringPointer]) and (byte[stringPointer] =< "9"))
        ifnot(("A" =< byte[stringPointer]) and (byte[stringPointer] =< "F"))
          quit

        result += constant(($A - ("A" & $F)) -> 4)
      result := ((result <- 4) + (byte[stringPointer++] & $F))