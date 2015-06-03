' by: Kwabena W. Agyeman - kwagyeman@openmv.io

con

  TARGET_ID_CODE_1 = $2BA0_1477
  CTRL_STAT_WRITE_VALUE = $5400_0000
  CTRL_STAT_READ_VALUE = $F400_0000
  CTRL_STAT_READ_MASK = $FF00_0000
  TARGET_ID_CODE_2 = $2477_0011

  HALT_ADDRESS = $E000_EDF0
  HALT_VALUE = $A05F_0003

  FLASH_UNLOCK_ADDRESS = $4002_3C0C
  FLASH_UNLOCK_KEY_1 = $89AB_CDEF
  FLASH_UNLOCK_KEY_2 = $0203_0405

  PROGRAM_UNLOCK_ADDRESS = $4002_3C10
  PROGRAM_UNLOCK_KEY_1 = $8C9D_AEBF
  PROGRAM_UNLOCK_KEY_2 = $1314_1516

  DATA_ADDRESS = $08000000
  
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
  waitcnt((clkfreq / 100) + cnt)
  dira[reset_pin] := 0

  cog_id := cognew(@cog_addr, @swd_action) + 1

pub stop(reset_pin)

  if cog_id
    cogstop(cog_id~ -1)

  outa[reset_pin] := 0 
  dira[reset_pin] := 1
  waitcnt((clkfreq / 100) + cnt)
  dira[reset_pin] := 0

dat

' /////////// Init

cog_addr      mov action_addr, par
              mov block_addr, par
              add block_addr, #4            

              or dira, swdio_pin
              or dira, swdclk_pin  

' /////////// Main

loop          rdlong par, action_addr
              cmp par, #"W" wz
if_z          call #do_w

              rdlong par, action_addr
              cmp par, #"R" wz
if_z          call #do_r

              jmp #loop

' /////////// Write Logic

do_w          cmp first_write, #1 wz
if_nz         mov first_write, #1
if_nz         call #w_init

              mov par, #128
              mov c, block_addr 
do_w_l        mov a, write_ptr
              rdlong b, c
              call #ahb_w
              add write_ptr, #4
              add c, #4
              djnz par, #do_w_l 

do_w_ret      ret
first_write   long 0
write_ptr     long DATA_ADDRESS

' /////////// Read Logic

do_r          cmp first_read, #1 wz
if_nz         mov first_read, #1
if_nz         call #r_init

              mov par, #128
              mov c, block_addr 
do_r_l        mov a, read_ptr
              call #ahb_r
              wrlong b, c
              add read_ptr, #4
              add c, #4
              djnz par, #do_r_l

do_r_ret      ret
first_read    long 0
read_ptr      long DATA_ADDRESS 

' /////////// Sub Write Init

w_init        call #send_res
              call #send_seq
              call #send_res

              ' read id 1
              
              mov x, #%0100
              call #send_req

              cmp y, init_id_code1 wz
if_nz         jmp #fatal_error

              ' power up req

              mov x, #%0010
              mov y, ctrl_stat_w
              call #send_req

              ' power up ack
              
              mov x, #%0110
              call #send_req

              and y, ctrl_stat_rm
              cmp y, ctrl_stat_r wz
if_nz         jmp #fatal_error

              ' set select

              mov x, #%0001
              mov y, #$F0
              call #send_req

              ' read id 2

              mov x, #%1111
              call #send_req

              cmp y, init_id_code2 wz
if_nz         jmp #fatal_error

              ' set select

              mov x, #%0001
              mov y, #0
              call #send_req

              ' setup control (32-bit auto-incrementing)

              mov x, #%1100
              call #send_req

              mov x, #%1000
              andn y, #$FF
              or y, #$12
              call #send_req
              
              ' halt processor

              mov a, halt_a
              mov b, halt_v
              call #ahb_w

              ' unlock flash

              mov a, flash_key_a
              mov b, flash_key_1
              call #ahb_w

              mov a, flash_key_a
              mov b, flash_key_2
              call #ahb_w

              ' unlock program

              mov a, program_key_a
              mov b, program_key_1
              call #ahb_w

              mov a, program_key_a
              mov b, program_key_2
              call #ahb_w             

w_init_ret    ret

init_id_code1 long TARGET_ID_CODE_1  
ctrl_stat_w   long CTRL_STAT_WRITE_VALUE
ctrl_stat_r   long CTRL_STAT_READ_VALUE
ctrl_stat_rm  long CTRL_STAT_READ_MASK
init_id_code2 long TARGET_ID_CODE_2

halt_a        long HALT_ADDRESS
halt_v        long HALT_VALUE

flash_key_a   long FLASH_UNLOCK_ADDRESS
flash_key_1   long FLASH_UNLOCK_KEY_1
flash_key_2   long FLASH_UNLOCK_KEY_2

program_key_a long PROGRAM_UNLOCK_ADDRESS
program_key_1 long PROGRAM_UNLOCK_KEY_1
program_key_2 long PROGRAM_UNLOCK_KEY_1

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

              mov x, #%0111 ' DP-RDBUFF Read
              call #send_req

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

send_res      mov i, #100
              or outa, swdio_pin 
send_res_l    xor outa, swdclk_pin    
              djnz i, #send_res_l
              andn outa, swdio_pin 
send_res_ret  ret

' /////////// Sub Send (JTAG-to-SWD) Seq

send_seq      mov i, #16
              mov x, seq
send_seq_l    shr x, #1 wc
              muxc outa, swdio_pin
              or outa, swdclk_pin
              andn outa, swdclk_pin
              djnz i, #send_seq_l
              andn outa, swdio_pin 
send_seq_ret  ret
seq           long $E79E

' /////////// Sub Send Req
' IN x - request (4-bits)
'   bit 3 = AP(1)/DP(0)
'   bit 2 = R(1)/W(0)
'   bit 1 = A2
'   bit 0 = A3
' IN/OUT y - write data (32-bits)

send_req      test x, #4 wz ' add parity bit 
              and x, #$F wc 
              rcl x, #1

              shl x, #2 ' add framing bits
              or x, #$81
              rev x, #24

              mov i, #8
send_req_l0   shr x, #1 wc
              muxc outa, swdio_pin
              or outa, swdclk_pin
              andn outa, swdclk_pin
              djnz i, #send_req_l0

              andn outa, swdio_pin
              andn dira, swdio_pin 
              or outa, swdclk_pin
              andn outa, swdclk_pin

              mov x, #0
              mov i, #3 
send_req_l1   or outa, swdclk_pin
              andn outa, swdclk_pin
              test swdio_pin, ina wc
              rcr x, #1 
              djnz i, #send_req_l1
              shr x, #29

              sub x, #1
              tjnz x, #fatal_error 

if_z          or outa, swdclk_pin
if_z          andn outa, swdclk_pin
if_z          or dira, swdio_pin

              testn y, #0 wc ' get parity
              mov i, #33
send_req_l2   rcr y, #1 wc
if_z          muxc outa, swdio_pin
              or outa, swdclk_pin
              andn outa, swdclk_pin
              test swdio_pin, ina wc
              djnz i, #send_req_l2

              muxc x, #1 ' already 0 - put parity
              testn y, #0 wc ' get parity 
              muxc i, #1 ' already 0 - put parity
              xor x, i
              tjnz x, #fatal_error
              
if_nz         or outa, swdclk_pin
if_nz         andn outa, swdclk_pin
              andn outa, swdio_pin
              or dira, swdio_pin

              mov i, #8
send_req_l3   or outa, swdclk_pin
              andn outa, swdclk_pin
              djnz i, #send_req_l3

              mov x, #0
send_req_ret  ret            

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
