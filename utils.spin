' by: Kwabena W. Agyeman - kwagyeman@openmv.io

con

  SAMPLE_FREQ = 80

var

  long dira_enable
  byte outa_frequency[32]

  byte processed_ina[32]
  byte posedge_ina[32]
  byte negedge_ina[32]

  long cog_id
  long cog_stack[32]

pub get(pin)

  return processed_ina[pin & 31]

pub get_posedge(pin)

  pin &= 31

  if posedge_ina[pin]
    return posedge_ina[pin]~

pub get_negedge(pin)

  pin &= 31

  if negedge_ina[pin]
    return negedge_ina[pin]~

pub set_freq(pin, freq)

  dira_enable := (dira_enable & !|<pin) | ((freq >= 0) & |<pin)
  outa_frequency[pin & 31] := SAMPLE_FREQ / (freq #> 0)

pub start

  cog_id := cognew(driver, @cog_stack) + 1

pub stop

  if cog_id
    cogstop(cog_id~ - 1)

pri driver | i, t, raw_ina[8], outa_counters[8]

  t := cnt

  repeat
    waitcnt(t += clkfreq / SAMPLE_FREQ)
    dira := dira_enable

    repeat i from 0 to 31
      raw_ina.byte[i] += raw_ina.byte[i] + ina[i]

      ifnot !raw_ina.byte[i]
        ifnot processed_ina[i]
          processed_ina[i] := true
          ifnot posedge_ina[i]
            posedge_ina[i] := true

      ifnot raw_ina.byte[i]
        if processed_ina[i]
          processed_ina[i] := false
          ifnot negedge_ina[i]
            negedge_ina[i] := true

      outa[i] := outa_counters.byte[i] => (outa_frequency[i] / 2)
      outa_counters.byte[i] := (outa_counters.byte[i] + 1) // outa_frequency[i]
