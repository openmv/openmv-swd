' by: Kwabena W. Agyeman - kwagyeman@openmv.io

con

  TARGET_ID_CODE_1 = $2BA0_1477
  CTRL_STAT_W_VALUE = $5400_0000
  CTRL_STAT_R_VALUE = $F400_0000
  CTRL_STAT_M_VALUE = $FF00_0000
  TARGET_ID_CODE_2 = $2477_0011
  CSW_W_VALUE = $2300_0012

  HALT_1_ADDRESS = $E000_EDF0
  HALT_1_VALUE = $A05F_0003
  HALT_2_ADDRESS = $E000_EDFC
  HALT_2_VALUE = $0000_0001
  HALT_3_ADDRESS = $E000_ED0C
  HALT_3_VALUE = $FA05_0004

  FLASH_UNLOCK_ADDRESS = $4002_3C04 ' STM32F427/07
  FLASH_UNLOCK_KEY_1 = $4567_0123 ' STM32F427/07
  FLASH_UNLOCK_KEY_2 = $CDEF_89AB ' STM32F427/07

  MASS_ERASE_ADDRESS = $4002_3C10 ' STM32F427/07
  MASS_ERASE_VALUE = $0001_8205 ' STM32F427/07

  MASS_ERASE_STATUS_ADDRESS = $4002_3C0C ' STM32F427/07
  MASS_ERASE_STATUS_R = $0001_0000 ' STM32F427/07
  MASS_ERASE_STATUS_M = $0001_0000 ' STM32F427/07

  DATA_ADDRESS = $0800_0000 ' MUST BE 4KB ALIGNED

var

  long cog_id, swd_action, data_block[128] ' Do Not Rearrange!

pub internal_block_address

  return @data_block

pub write_block

  if swd_action == "E"
    abort 1

  swd_action := "W"

pub read_block

  if swd_action == "E"
    abort 1

  swd_action := "R"

pub busy

  if swd_action == "E"
    abort 1

  return swd_action

pub start(io_pin, clk_pin, reset_pin)

  swdio_pin := |<io_pin
  swdclk_pin := |<clk_pin
  swd_action := 0

  outa[reset_pin] := 0
  dira[reset_pin] := 1
  waitcnt((clkfreq / 1000) + cnt)
  dira[reset_pin] := 0

  cog_id := cognew(@cog_addr, @swd_action) + 1

pub stop(reset_pin)

  if cog_id
    cogstop(cog_id~ -1)

  outa[reset_pin] := 0
  dira[reset_pin] := 1
  waitcnt((clkfreq / 1000) + cnt)
  dira[reset_pin] := 0

dat

' /////////// Init

              org 0

cog_addr      mov action_addr, par
              mov block_addr, par
              add block_addr, #4

              or dira, swdio_pin
              or dira, swdclk_pin

' /////////// Main

loop          rdlong x, action_addr
              cmp x, #"W" wz
if_z          call #do_w

              rdlong x, action_addr
              cmp x, #"R" wz
if_z          call #do_r

              jmp #loop

' /////////// Write Logic

do_w          cmp write_cnt, #511 wz
if_z          call #w_init

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

              mov write_cnt, #128
              mov c, block_addr

do_w_l        mov x, #%1011 ' AP-DRW Write
              rdlong y, c
              call #send_req

              add c, #4
              djnz write_cnt, #do_w_l

              ' FASTER END

              mov x, #0
              wrlong x, action_addr

do_w_ret      ret
write_cnt     long 511 ' first write value trigger
write_ptr     long DATA_ADDRESS

' /////////// Read Logic

do_r          cmp read_cnt, #511 wz
if_z          call #r_init

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

              mov read_cnt, #127
              mov c, block_addr

do_r_l        mov x, #%1111 ' AP-DRW Read
              call #send_req
              wrlong y, c

              add c, #4
              djnz read_cnt, #do_r_l

              mov x, #%0111 ' DP-RDBUFF Read
              call #send_req
              wrlong y, c

              ' FASTER END

              mov x, #0
              wrlong x, action_addr

do_r_ret      ret
read_cnt      long 511 ' first read value trigger
read_ptr      long DATA_ADDRESS

D512          long 512

' /////////// Sub Write Init

w_init        call #send_res
              call #send_seq
              call #send_res
              call #clk_pulse
              call #clk_pulse

              ' read id 1

              mov x, #%0100
              call #send_req

              cmp y, init_id_code1 wz
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

              cmp y, init_id_code2 wz
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

              mov a, flash_key_a
              mov b, flash_key_1
              call #ahb_w

              mov a, flash_key_a
              mov b, flash_key_2
              call #ahb_w

              mov a, flash_ctrl_a
              mov b, flash_ctrl_v
              call #ahb_w

              ' wait till done

w_init_l      mov a, flash_sts_a
              call #ahb_r

              and b, flash_sts_m
              cmp b, flash_sts_r wz
if_z          jmp #w_init_l

w_init_ret    ret

init_id_code1 long TARGET_ID_CODE_1
ctrl_stat_w   long CTRL_STAT_W_VALUE
ctrl_stat_r   long CTRL_STAT_R_VALUE
ctrl_stat_m   long CTRL_STAT_M_VALUE
init_id_code2 long TARGET_ID_CODE_2
csw_w         long CSW_W_VALUE

halt_1_a      long HALT_1_ADDRESS
halt_1_v      long HALT_1_VALUE
halt_2_a      long HALT_2_ADDRESS
halt_2_v      long HALT_2_VALUE
halt_3_a      long HALT_3_ADDRESS
halt_3_v      long HALT_3_VALUE

flash_key_a   long FLASH_UNLOCK_ADDRESS
flash_key_1   long FLASH_UNLOCK_KEY_1
flash_key_2   long FLASH_UNLOCK_KEY_2

flash_ctrl_a  long MASS_ERASE_ADDRESS
flash_ctrl_v  long MASS_ERASE_VALUE

flash_sts_a   long MASS_ERASE_STATUS_ADDRESS
flash_sts_r   long MASS_ERASE_STATUS_R
flash_sts_m   long MASS_ERASE_STATUS_M

' /////////// Sub Read Init

r_init
r_init_ret    ret

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

clk_pulse     ' nop
              or outa, swdclk_pin
              ' nop
clk_pulse_m   nop
              ' nop
              andn outa, swdclk_pin
              ' nop
clk_pulse_ret ret

' /////////// Sub Fatal Error

fatal_error   mov x, #"E"
              wrlong x, action_addr
              cogid x
              cogstop x

swdio_pin     long 0
swdclk_pin    long 0
action_addr   long 0
block_addr    long 0

a             res 1
b             res 1
c             res 1

i             res 1
x             res 1
y             res 1

              fit 496
