con

  _CLKFREQ = 80_000_000
  _CLKMODE = XTAL1 + PLL16X

  NUM_PORTS = 20

  ADC_MIN_RANGE = 2047
  ADC_MAX_RANGE = 4095

  BAUD_RATE = 115200

  NUM_BASE_PINS = 4  
  SWD_IO_PIN_BASE = 0
  SWD_CLK_PIN_BASE = 1
  SWD_RESET_PIN_BASE = 2
  SWD_STATUS_PIN_BASE = 3   

  ADC_DIN = 18
  ADC_DOUT = 19
  ADC_SCLK = 20
  ADC_CS = 21
  
  SD_DO_PIN = 22
  SD_CLK_PIN = 23
  SD_DI_PIN = 24
  SD_CS_PIN = 25
  SD_WP_PIN = -1
  SD_CD_PIN = SD_DI_PIN
  
  TX_PIN = 30
  RX_PIN = 31

dat

  firmware_file_name byte "FIRMWARE.DAT", 0 ' 8.3 file name
  
obj

  fat[NUM_PORTS] : "SD-MMC_FATEngine.spin"
  ser : "Full-Duplex_COMEngine.spin"
  swd[NUM_PORTS] : "SWD.spin"

var

  long board_state[NUM_PORTS]
  
pub main | i, x

  fat.FATEngineStart(SD_DO_PIN, SD_CLK_PIN, SD_DI_PIN, SD_CS_PIN, SD_WP_PIN, SD_CD_PIN, -1, -1, -1)
  ser.COMEngineStart(RX_PIN, TX_PIN, BAUD_RATE)

  repeat
    repeat i from 0 to constant(NUM_PORTS - 1)

      if get_board_connected(i)
        board_state[i] := 1    

      if get_board_disconnected(i)
        x := \board_disconnected_code(i)

      case board_state[i]
        1 : x := \board_connected_code(i)
        2 : x := \board_program_code(i)
        3 : x := \board_disconnected_code(i)  
        other : 

pri board_connected_code(port) ' may abort...

  fat[port].mountPartition(0)
  fat[port].openFile(@firmware_file_name, "R")

  swd[port].start(SWD_IO_PIN_BASE + port, SWD_CLK_PIN_BASE + port, SWD_RESET_PIN_BASE + port) 

  dira[SWD_STATUS_PIN_BASE + port] := 1

  board_state[port] := 2 
  
pri board_program_code(port) ' may abort... 

  if board_state[port] == 2
    if swd[port].write_pointer_available
      !outa[SWD_STATUS_PIN_BASE + port]
      ifnot fat[port].readData(swd[port].get_write_pointer, 512)
        outa[SWD_STATUS_PIN_BASE + port] := 1
        board_state[port] := 3   
  
pri board_disconnected_code(port) ' may abort... 

  swd[port].stop

  fat[port].unmountPartition

  board_state[port] := 0 

con

  SAMPLE_RATE = 500
  
  BLINK_TICKS = 100 

var

  long adc_debounce[NUM_PORTS * 32]
  long adc_debounce_i[NUM_PORTS]
  long adc_debounce_x[NUM_PORTS]

  long board_present[NUM_PORTS]
  long board_connected_event[NUM_PORTS]
  long board_disconnected_event[NUM_PORTS]

  long status_blink[NUM_PORTS]
  long status_blink_counters[NUM_PORTS]
  
  long debouncer_cog
  long debouncer_cog_stack[32]
  
pri get_board_connected(port)

  if board_connected_event[port]
    return board_connected_event[port]~

pri get_board_disconnected(port)

  if board_disconnected_event[port]
    return board_disconnected_event[port]~

pri set_status_led(port, state)

  case state
    0, 1:
      outa[(port * NUM_BASE_PINS) + SWD_STATUS_PIN_BASE] := 0
      dira[(port * NUM_BASE_PINS) + SWD_STATUS_PIN_BASE] := 0
      status_blink[port] := 0
    2:
      status_blink[port] := 1
    3:
      outa[(port * NUM_BASE_PINS) + SWD_STATUS_PIN_BASE] := 1
      dira[(port * NUM_BASE_PINS) + SWD_STATUS_PIN_BASE] := 1
      status_blink[port] := 0
 
pri debouncer_start

  debouncer_cog := cognew(debouncer_main, @debouncer_cog_stack) + 1

pri debouncer_stop

  if debouncer_cog
    cogstop(debouncer_cog~ -1)

pri debouncer_main | i, t

  adc_init 

  repeat i from 0 to constant(NUM_PORTS - 1)
    dira[(i * NUM_BASE_PINS) + SWD_STATUS_PIN_BASE] := 1
  
  t := cnt
  repeat
    t += clkfreq / SAMPLE_RATE
    waitcnt(t)
    
    repeat i from 0 to constant(NUM_PORTS - 1)
      debounce(i)

      outa[(i * NUM_BASE_PINS) + SWD_STATUS_PIN_BASE] := status_blink_counters[i] > constant(BLINK_TICKS / 2)
      status_blink_counters[i] := (status_blink_counters[i] + 1) // BLINK_TICKS

pri debounce(port) : r | i 

  r := adc_debounce[(port * 32) + adc_debounce_i] := read_adc(port)
  adc_debounce_i := (adc_debounce_i + 1) & 31
  adc_debounce_x += adc_debounce_x + (((ADC_MIN_RANGE =< r) and (r =< ADC_MAX_RANGE)) & 1)  

  ifnot !adc_debounce_x ' all ones
    ifnot board_present[port]
      board_present[port] := 1
      ifnot board_connected_event[port]
        board_connected_event[port] := 1

  ifnot adc_debounce_x ' all zeros
    if board_present[port]
      board_present[port] := 0
      ifnot board_disconnected_event[port]
        board_disconnected_event[port] := 1         
  
pri adc_init

  outa[ADC_CS] := 1
  dira[ADC_CS] := 1
  
  outa[ADC_SCLK] := 1 
  dira[ADC_SCLK] := 1

  dira[ADC_DIN] := 1

pri read_adc(port) : in | out

  port &= $3

  out := ((port << 27) | (port << 11)) >< 32

  outa[ADC_CS] := 0

  repeat 32
    outa[ADC_SCLK] := 0
    outa[ADC_DIN] := out
    out >>= 1
    in += in + ina[ADC_DOUT] 
    outa[ADC_SCLK] := 1
 
  outa[ADC_CS] := 1  

  in &= $FFF     
