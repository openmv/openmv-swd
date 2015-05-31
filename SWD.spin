' by: Kwabena W. Agyeman - kwagyeman@openmv.io

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

  return swd_action <> 0

pub start(io_pin, clk_pin, reset_pin)

  swdio_pin := |<io_pin
  swdclk_pin := |<clk_pin

  outa[reset_pin] := 0 
  dira[reset_pin] := 1
  waitcnt((clkfreq / 1000) + cnt)
  dira[reset_pin] := 0
  
  swd_action := 0
  cog_id := cognew(@cog_addr, @swd_action) + 1

pub stop

  if cog_id
    cogstop(cog_id~ -1)

dat

' /////////// Init

cog_addr      mov action_addr, par
              mov block_addr, par
              add block_addr, #4            

              or dira, swdclk_pin  

' /////////// Main

do_init       call #send_res
              call #send_seq
              call #send_res

              mov x, #%0100
              call #send_req

send_res      mov i, #100
              or outa, swdio_pin 
send_res_l    xor outa, swdclk_pin    
              djnz i, #send_res_l
              andn outa, swdio_pin 
send_res_ret  ret

send_seq      mov i, #16
              mov j, seq
send_seq_l    shr j, #1 wc
              muxc outa, swdio_pin
              or outa, swdclk_pin
              andn outa, swdclk_pin
              djnz i, #send_seq_l 
send_seq_ret  ret
seq           long $E79E

send_req      test x, #31 wc
              rcl x, #1
              shl x, #2
              or x, #129
              rev x, #24
              test x, #4 wz

              or dira, swdio_pin 
              
              mov i, #8
send_req_l0   shr x, #1 wc
              muxc outa, swdio_pin
              or outa, swdclk_pin
              andn outa, swdclk_pin
              djnz i, #send_req_l0

              andn dira, swdio_pin 
              or outa, swdclk_pin
              andn outa, swdclk_pin

              mov x, #0
              mov i, #3 
send_req_l1   or outa, swdclk_pin
              andn outa, swdclk_pin
              test swdio_pin, ina wc
              rcl x, #1 
              djnz i, #send_req_l1
            
if_z          or dira, swdio_pin
if_nz         andn dira, swdio_pin 
if_z          or outa, swdclk_pin
if_z          andn outa, swdclk_pin

              mov i, #32
              mov j, y
send_req_l2   rcr y, #1 wc
              muxc outa, swdio_pin
              or outa, swdclk_pin
              andn outa, swdclk_pin
              test swdio_pin, ina wc
              djnz i, #send_req_l2               

              rcr y, #1
              rev y, #0

              'test j, #$FFFFFFFF

              ' do parity stuff
              
if_nz         or outa, swdclk_pin
if_nz         andn outa, swdclk_pin
if_nz         or dira, swdio_pin         
              
send_req_ret  ret            

swdio_pin     long 0
swdclk_pin    long 0
action_addr   long 0
block_addr    long 0

i             res 1
j             res 1
x             res 1
y             res 1