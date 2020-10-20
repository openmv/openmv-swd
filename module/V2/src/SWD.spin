' by: Kwabena W. Agyeman - kwagyeman@openmv.io

con

  M4_TARGET_ID_CODE_1 = $2BA0_1477 ' STM32F4
  M7_TARGET_ID_CODE_1 = $5BA0_2477 ' STM32F7
  H7_TARGET_ID_CODE_1 = $6BA0_2477 ' STM32H7
  CTRL_STAT_W_VALUE = $5000_0000 ' STM32F4/STM32F7/STM32H7
  CTRL_STAT_R_VALUE = $F000_0000 ' STM32F4/STM32F7/STM32H7
  CTRL_STAT_M_VALUE = $F000_0000 ' STM32F4/STM32F7/STM32H7
  M4_TARGET_ID_CODE_2 = $2477_0011 ' STM32F4
  M7_TARGET_ID_CODE_2 = $7477_0001 ' STM32F7
  H7_TARGET_ID_CODE_2 = $8477_0001 ' STM32H7
  CSW_W_VALUE = $2300_0012 ' STM32F4/STM32F7/STM32H7

  HALT_1_ADDRESS = $E000_EDF0 ' STM32F4/STM32F7/STM32H7
  HALT_1_VALUE = $A05F_0003 ' STM32F4/STM32F7/STM32H7
  HALT_2_ADDRESS = $E000_EDFC ' STM32F4/STM32F7/STM32H7
  HALT_2_VALUE = $0000_0001 ' STM32F4/STM32F7/STM32H7
  HALT_3_ADDRESS = $E000_ED0C ' STM32F4/STM32F7/STM32H7
  HALT_3_VALUE = $05FA_0004 ' STM32F4/STM32F7/STM32H7

  M47_FLASH_UNLOCK_ADDRESS = $4002_3C04 ' STM32F4/STM32F7
  H7_FLASH_UNLOCK_ADDRESS_0 = $5200_2004 ' STM32H7
  H7_FLASH_UNLOCK_ADDRESS_1 = $5200_2104 ' STM32H7
  FLASH_UNLOCK_KEY_1 = $4567_0123 ' STM32F4/STM32F7/STM32H7
  FLASH_UNLOCK_KEY_2 = $CDEF_89AB ' STM32F4/STM32F7/STM32H7

  M47_MASS_ERASE_ADDRESS = $4002_3C10 ' STM32F4/STM32F7
  H7_MASS_ERASE_ADDRESS_0 = $5200_200C ' STM32H7
  H7_MASS_ERASE_ADDRESS_1 = $5200_210C ' STM32H7
  M47_MASS_ERASE_VALUE = $0001_8205 ' STM32F4/STM32F7
  H7_MASS_ERASE_VALUE = $0000_00AA ' STM32H7

  M47_MASS_ERASE_STATUS_ADDRESS = $4002_3C0C ' STM32F4/STM32F7
  H7_MASS_ERASE_STATUS_ADDRESS_0 = $5200_2010 ' STM32H7
  H7_MASS_ERASE_STATUS_ADDRESS_1 = $5200_2110 ' STM32H7
  M47_MASS_ERASE_STATUS_MASK = $0001_0000 ' STM32F4/STM32F7
  H7_MASS_ERASE_STATUS_MASK = $0000_0001 ' STM32H7

  DATA_ADDRESS = $0800_0000 ' MUST BE 4KB ALIGNED

  M4_ID_ADDRESS = $1FFF_7A10
  M7_ID_ADDRESS = $1FF0_F420
  H7_ID_ADDRESS = $1FF1_E800

  M4_FB_ADDRESS = $2000_0018
  M7_FB_ADDRESS = $2002_0018
  H7_FB_ADDRESS = $2400_0018

  M4_ERASE_TIMEOUT = 10
  M7_ERASE_TIMEOUT = 10
  H7_ERASE_TIMEOUT = 10

  M4_TEST_TIMEOUT = 10
  M7_TEST_TIMEOUT = 10
  H7_TEST_TIMEOUT = 10

var

  long cog_id, swd_action, device_type, unique_id[3], data_block[128] ' Do Not Rearrange!

pub get_device

  return device_type

pub get_id

  return @unique_id

pub internal_block_address

  return @data_block

pub init

  if swd_action == "E"
    abort 1

  swd_action := "I"

pub write_block

  if swd_action == "E"
    abort 1

  swd_action := "W"

pub read_block

  if swd_action == "E"
    abort 1

  swd_action := "R"

pub fini

  if swd_action == "E"
    abort 1

  swd_action := "F"

pub busy

  if swd_action == "E"
    abort 1

  return swd_action

pub start(io_pin, clk_pin, reset_pin)

  swdio_pin := |<io_pin
  swdclk_pin := |<clk_pin
  swdrst_pin := |<reset_pin
  swd_action := 0

  m4_erase_time := clkfreq * M4_ERASE_TIMEOUT
  m7_erase_time := clkfreq * M7_ERASE_TIMEOUT
  h7_erase_time := clkfreq * H7_ERASE_TIMEOUT
  m4_test_time := clkfreq * M4_TEST_TIMEOUT
  m7_test_time := clkfreq * M7_TEST_TIMEOUT
  h7_test_time := clkfreq * H7_TEST_TIMEOUT

  cog_id := cognew(@cog_addr, @swd_action) + 1

pub stop

  if cog_id
    cogstop(cog_id - 1)
    cog_id := 0

dat

              org 0

' /////////// Init

cog_addr      mov action_addr, par
              mov device_addr, par
              add device_addr, #4
              mov id_addr,     par
              add id_addr,     #8
              mov block_addr,  par
              add block_addr,  #20

              or outa, swdrst_pin
              or dira, swdio_pin
              or dira, swdclk_pin
              or dira, swdrst_pin

' /////////// Main

loop          rdlong x, action_addr
              cmp x, #"I" wz
if_z          call #sub_init

              rdlong x, action_addr
              cmp x, #"W" wz
if_z          call #do_w

              rdlong x, action_addr
              cmp x, #"R" wz
if_z          call #do_r

              rdlong x, action_addr
              cmp x, #"F" wz
if_z          call #sub_fini

              jmp #loop

' /////////// Write Logic

do_w

'               mov write_cnt, #128
'               mov c, block_addr
' do_w_l        mov a, write_ptr
'               rdlong b, c
'               call #ahb_w
'               add c, #4
'               add write_ptr, #4
'               djnz write_cnt, #do_w_l

              ' FASTER BEGIN - REQ 4KB ALIGNED

              mov x, #%1010 ' AP-TAR Write
              mov y, write_ptr
              call #send_req

              add write_ptr, D512

              mov w_cnt, #128
              mov c, block_addr

do_w_l        mov x, #%1011 ' AP-DRW Write
              rdlong y, c
              call #send_req

              add c, #4
              djnz w_cnt, #do_w_l

              ' FASTER END

              mov x, #0
              wrlong x, action_addr

do_w_ret      ret
write_ptr     long DATA_ADDRESS

' /////////// Read Logic

do_r

'               mov read_cnt, #128
'               mov c, block_addr
' do_r_l        mov a, read_ptr
'               call #ahb_r
'               wrlong b, c
'               add c, #4
'               add read_ptr, #4
'               djnz read_cnt, #do_r_l

              ' FASTER BEGIN - REQ 4KB ALIGNED

              mov x, #%1010 ' AP-TAR Write
              mov y, read_ptr
              call #send_req

              add read_ptr, D512

              mov x, #%1111 ' AP-DRW Read
              call #send_req

              mov r_cnt, #127
              mov c, block_addr

do_r_l        mov x, #%1111 ' AP-DRW Read
              call #send_req
              wrlong y, c

              add c, #4
              djnz r_cnt, #do_r_l

              mov x, #%0111 ' DP-RDBUFF Read
              call #send_req
              wrlong y, c

              ' FASTER END

              mov x, #0
              wrlong x, action_addr

do_r_ret      ret
read_ptr      long DATA_ADDRESS

D512          long 512

' /////////// Sub Init

sub_init      mov fatal_error, #0 ' disable fatal error

              andn outa, swdrst_pin
              rdlong x, #0
              shr x, #2
              add x, cnt
              waitcnt x, #0
              or outa, swdrst_pin

              call #send_res
              call #send_seq
              call #send_res
              call #clk_pulse
              call #clk_pulse

              ' read id 1

              mov x, #%0100
              call #send_req

              cmp y, m4_id_code1 wz
if_nz         cmp y, m7_id_code1 wz
if_nz         cmp y, h7_id_code1 wz
if_nz         jmp #fatal_error
              mov c, y ' backup
              wrlong c, device_addr

              ' power up req

              mov x, #%0000
              mov y, #$1F
              call #send_req

              mov x, #%0010
              mov y, ctrl_stat_w
              call #send_req

              ' power up ack

              mov x, #%0110
              call #send_req

              and y, ctrl_stat_m
              cmp y, ctrl_stat_r wz
if_nz         jmp #fatal_error

              ' set select

              mov x, #%0001
              mov y, #$F0
              call #send_req

              ' read id 2

              mov x, #%1111
              call #send_req

              mov x, #%0111
              call #send_req

              cmp c, m4_id_code1 wz
if_z          mov x, m4_id_code2
              cmp c, m7_id_code1 wz
if_z          mov x, m7_id_code2
              cmp c, h7_id_code1 wz
if_z          mov x, h7_id_code2
              cmp y, x wz
if_nz         jmp #fatal_error

              ' set select

              mov x, #%0001
              mov y, #0
              call #send_req

              ' setup access

              mov x, #%1000
              mov y, csw_w
              call #send_req

              ' halt processor

              mov a, halt_1_a
              mov b, halt_1_v
              call #ahb_w

              mov a, halt_2_a
              mov b, halt_2_v
              call #ahb_w

              mov a, halt_3_a
              mov b, halt_3_v
              call #ahb_w

              mov a, halt_2_a
              mov b, #0
              call #ahb_w

              ' unlock and erase flash

              cmp c, h7_id_code1 wz
if_nz         mov a, m47_f_key_a
if_z          mov a, h7_f_key_a_0
              mov b, f_key_1
              call #ahb_w

              cmp c, h7_id_code1 wz
if_nz         mov a, m47_f_key_a
if_z          mov a, h7_f_key_a_0
              mov b, f_key_2
              call #ahb_w

              cmp c, h7_id_code1 wz
if_nz         mov a, m47_f_ctrl_a
if_nz         mov b, m47_f_ctrl_v
if_z          mov a, h7_f_ctrl_a_0
if_z          mov b, h7_f_ctrl_v
              call #ahb_w

              cmp c, h7_id_code1 wz
if_nz         jmp #sub_init_j_0

              mov a, h7_f_key_a_1
              mov b, f_key_1
              call #ahb_w

              mov a, h7_f_key_a_1
              mov b, f_key_2
              call #ahb_w

              mov a, h7_f_ctrl_a_1
              mov b, h7_f_ctrl_v
              call #ahb_w

              ' wait till done

sub_init_j_0  mov w_cnt, cnt
sub_init_l    mov x, cnt
              sub x, w_cnt
              cmp c, m4_id_code1 wz
if_z          cmp x, m4_erase_time wc
              cmp c, m7_id_code1 wz
if_z          cmp x, m7_erase_time wc
              cmp c, h7_id_code1 wz
if_z          cmp x, h7_erase_time wc
if_nc         jmp #fatal_error

              cmp c, h7_id_code1 wz
if_nz         mov a, m47_f_sts_a
if_z          mov a, h7_f_sts_a_0
              call #ahb_r

              cmp c, h7_id_code1 wz
if_nz         test b, m47_f_sts_msk wc
if_z          test b, h7_f_sts_msk wc
if_c          jmp #sub_init_l

              cmp c, h7_id_code1 wz
if_nz         jmp #sub_init_j_1

              mov a, h7_f_sts_a_1
              call #ahb_r

              test b, h7_f_sts_msk wc
if_c          jmp #sub_init_l

              ' clear test address

sub_init_j_1  cmp c, m4_id_code1 wz
if_z          mov a, m4_test_addr
              cmp c, m7_id_code1 wz
if_z          mov a, m7_test_addr
              cmp c, h7_id_code1 wz
if_z          mov a, h7_test_addr
              mov b, #0
              call #ahb_w

              ' read id

              cmp c, m4_id_code1 wz
if_z          mov a, m4_id_addr
              cmp c, m7_id_code1 wz
if_z          mov a, m7_id_addr
              cmp c, h7_id_code1 wz
if_z          mov a, h7_id_addr
              call #ahb_r
              mov c, id_addr
              wrlong b, c

              add a, #4
              call #ahb_r
              add c, #4
              wrlong b, c

              add a, #4
              call #ahb_r
              add c, #4
              wrlong b, c

              mov x, #0
              wrlong x, action_addr

              mov fatal_error, fatal_error_t ' enable fatal error

sub_init_ret  ret

' /////////// Sub Fini

sub_fini      ' wait for the test to finish

              andn outa, swdrst_pin
              rdlong x, #0
              shr x, #2
              add x, cnt
              waitcnt x, #0
              or outa, swdrst_pin

              rdlong c, device_addr
              cmp c, m4_id_code1 wz
if_z          mov x, m4_test_time
              cmp c, m7_id_code1 wz
if_z          mov x, m7_test_time
              cmp c, h7_id_code1 wz
if_z          mov x, h7_test_time
              add x, cnt
              waitcnt x, #0

              andn outa, swdrst_pin
              rdlong x, #0
              shr x, #2
              add x, cnt
              waitcnt x, #0
              or outa, swdrst_pin

              call #send_res
              call #send_seq
              call #send_res
              call #clk_pulse
              call #clk_pulse

              ' read id 1

              mov x, #%0100
              call #send_req

              cmp y, m4_id_code1 wz
if_nz         cmp y, m7_id_code1 wz
if_nz         cmp y, h7_id_code1 wz
if_nz         jmp #fatal_error

              ' power up req

              mov x, #%0000
              mov y, #$1F
              call #send_req

              mov x, #%0010
              mov y, ctrl_stat_w
              call #send_req

              ' power up ack

              mov x, #%0110
              call #send_req

              and y, ctrl_stat_m
              cmp y, ctrl_stat_r wz
if_nz         jmp #fatal_error

              ' set select

              mov x, #%0001
              mov y, #$F0
              call #send_req

              ' read id 2

              mov x, #%1111
              call #send_req

              mov x, #%0111
              call #send_req

              cmp c, m4_id_code1 wz
if_z          mov x, m4_id_code2
              cmp c, m7_id_code1 wz
if_z          mov x, m7_id_code2
              cmp c, h7_id_code1 wz
if_z          mov x, h7_id_code2
              cmp y, x wz
if_nz         jmp #fatal_error

              ' set select

              mov x, #%0001
              mov y, #0
              call #send_req

              ' setup access

              mov x, #%1000
              mov y, csw_w
              call #send_req

              ' halt processor

              mov a, halt_1_a
              mov b, halt_1_v
              call #ahb_w

              mov a, halt_2_a
              mov b, halt_2_v
              call #ahb_w

              mov a, halt_3_a
              mov b, halt_3_v
              call #ahb_w

              mov a, halt_2_a
              mov b, #0
              call #ahb_w

              '  test address

              cmp c, m4_id_code1 wz
if_z          mov a, m4_test_addr
              cmp c, m7_id_code1 wz
if_z          mov a, m7_test_addr
              cmp c, h7_id_code1 wz
if_z          mov a, h7_test_addr
              call #ahb_r

'              cmp b, deadbeef wz
'if_nz         jmp #fatal_error

              mov x, #0
              wrlong x, action_addr

sub_fini_ret  ret

m4_id_code1   long M4_TARGET_ID_CODE_1
m7_id_code1   long M7_TARGET_ID_CODE_1
h7_id_code1   long H7_TARGET_ID_CODE_1
ctrl_stat_w   long CTRL_STAT_W_VALUE
ctrl_stat_r   long CTRL_STAT_R_VALUE
ctrl_stat_m   long CTRL_STAT_M_VALUE
m4_id_code2   long M4_TARGET_ID_CODE_2
m7_id_code2   long M7_TARGET_ID_CODE_2
h7_id_code2   long H7_TARGET_ID_CODE_2
csw_w         long CSW_W_VALUE

halt_1_a      long HALT_1_ADDRESS
halt_1_v      long HALT_1_VALUE
halt_2_a      long HALT_2_ADDRESS
halt_2_v      long HALT_2_VALUE
halt_3_a      long HALT_3_ADDRESS
halt_3_v      long HALT_3_VALUE

m47_f_key_a   long M47_FLASH_UNLOCK_ADDRESS
h7_f_key_a_0  long H7_FLASH_UNLOCK_ADDRESS_0
h7_f_key_a_1  long H7_FLASH_UNLOCK_ADDRESS_1
f_key_1       long FLASH_UNLOCK_KEY_1
f_key_2       long FLASH_UNLOCK_KEY_2

m47_f_ctrl_a  long M47_MASS_ERASE_ADDRESS
h7_f_ctrl_a_0 long H7_MASS_ERASE_ADDRESS_0
h7_f_ctrl_a_1 long H7_MASS_ERASE_ADDRESS_1
m47_f_ctrl_v  long M47_MASS_ERASE_VALUE
h7_f_ctrl_v   long H7_MASS_ERASE_VALUE

m47_f_sts_a   long M47_MASS_ERASE_STATUS_ADDRESS
h7_f_sts_a_0  long H7_MASS_ERASE_STATUS_ADDRESS_0
h7_f_sts_a_1  long H7_MASS_ERASE_STATUS_ADDRESS_1
m47_f_sts_msk long M47_MASS_ERASE_STATUS_MASK
h7_f_sts_msk  long H7_MASS_ERASE_STATUS_MASK

m4_id_addr    long M4_ID_ADDRESS
m7_id_addr    long M7_ID_ADDRESS
h7_id_addr    long H7_ID_ADDRESS

m4_test_addr  long M4_FB_ADDRESS
m7_test_addr  long M7_FB_ADDRESS
h7_test_addr  long H7_FB_ADDRESS

deadbeef      long $DEAD_BEEF

' /////////// Sub AHB Write
' IN a = Address (32-bits)
' IN b = Data (32-bits)

ahb_w         mov x, #%1010 ' AP-TAR Write
              mov y, a
              call #send_req

              mov x, #%1011 ' AP-DRW Write
              mov y, b
              call #send_req

              ' mov x, #%0111 ' DP-RDBUFF Read
              ' call #send_req

ahb_w_ret     ret

' /////////// Sub AHB Read
' IN a = Address (32-bits)
' OUT b = Data (32-bits)

ahb_r         mov x, #%1010 ' AP-TAR Write
              mov y, a
              call #send_req

              mov x, #%1111 ' AP-DRW Read
              call #send_req

              mov x, #%0111 ' DP-RDBUFF Read
              call #send_req
              mov b, y

ahb_r_ret     ret

' /////////// Sub Send Reset

send_res      mov i, #53
              or outa, swdio_pin
send_res_l    call #clk_pulse
              djnz i, #send_res_l
              andn outa, swdio_pin
send_res_ret  ret

' /////////// Sub Send (JTAG-to-SWD) Seq

send_seq      mov i, #16
send_seq_l    ror seq, #1 wc
              muxc outa, swdio_pin
              call #clk_pulse
              djnz i, #send_seq_l
send_seq_ret  ret
seq           long $E7_9E_E7_9E

' /////////// Sub Send Req
' IN x - request (4-bits)
'   bit 3 = AP(1)/DP(0)
'   bit 2 = R(1)/W(0)
'   bit 1 = A2
'   bit 0 = A3
' IN/OUT y - write data (32-bits)

send_req_l    mov x, #0

              call #clk_pulse
              or dira, swdio_pin
              call #clk_pulse

send_req      movs send_req_l, x ' backup command

              test x, #4 wz ' add parity bit
              and x, #$F wc
              rcl x, #1

              shl x, #2 ' add framing bits
              or x, #$81
              rev x, #24

              mov i, #7
send_req_l0   shr x, #1 wc
              muxc outa, swdio_pin
              call #clk_pulse
              djnz i, #send_req_l0

              or outa, swdio_pin
              mov clk_pulse_m, send_req_tmp
              call #clk_pulse
              xor clk_pulse_m, clk_pulse_m

              mov x, #0
              mov i, #3
send_req_l1   call #clk_pulse
              test swdio_pin, ina wc
              rcr x, #1
              djnz i, #send_req_l1
              shr x, #29

              muxnz i, #1 ' already 0 - save flag
              cmp x, #2 wz
if_z          jmp #send_req_l
              test i, #1 wz

              sub x, #1
              tjnz x, #fatal_error

if_z          call #clk_pulse
if_z          or dira, swdio_pin
if_z          call #clk_pulse

              testn y, #0 wc ' get parity
              mov i, #33
send_req_l2   rcr y, #1 wc
if_z          muxc outa, swdio_pin
              call #clk_pulse
              test swdio_pin, ina wc
              djnz i, #send_req_l2

              muxc x, #1 ' already 0 - put parity
              testn y, #0 wc ' get parity
              muxc i, #1 ' already 0 - put parity
              xor x, i
              tjnz x, #fatal_error

if_nz         call #clk_pulse
if_nz         or dira, swdio_pin
if_nz         call #clk_pulse

send_req_ret  ret
send_req_tmp  andn dira, swdio_pin

' /////////// Sub Clock Pulse

clk_pulse     mov cnt, clk_pulse_cfg
              djnz cnt, #$

              or outa, swdclk_pin

              mov cnt, clk_pulse_cfg
              djnz cnt, #$

clk_pulse_m   nop

              mov cnt, clk_pulse_cfg
              djnz cnt, #$

              andn outa, swdclk_pin

              mov cnt, clk_pulse_cfg
              djnz cnt, #$

clk_pulse_ret ret

clk_pulse_cfg long 1

' /////////// Sub Fatal Error

fatal_error   jmp #fatal_error_2
              add clk_pulse_cfg, #1
              cmp clk_pulse_cfg, #6 wc
if_c          jmp #sub_init

fatal_error_2 mov x, #"E"
              wrlong x, action_addr
              cogid x
              cogstop x

fatal_error_t jmp #fatal_error_2

swdio_pin     long 0
swdclk_pin    long 0
swdrst_pin    long 0

action_addr   long 0
device_addr   long 0
id_addr       long 0
block_addr    long 0

m4_erase_time long 0
m7_erase_time long 0
h7_erase_time long 0
m4_test_time  long 0
m7_test_time  long 0
h7_test_time  long 0

w_cnt         res 1
r_cnt         res 1

a             res 1
b             res 1
c             res 1

i             res 1
x             res 1
y             res 1

              fit 496