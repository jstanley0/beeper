/*
pin mapping (RPI GPIO => SN76489)

 4 => 14 CLOCK blue
24 => 4  READY yellow
25 => 5 /WE    yellow
15 => 10 D0    blue
18 => 11 D1    white
 7 => 12 D2    green
 8 => 13 D3    orange
 9 => 15 D4    yellow
10 => 1  D5    green
11 => 2  D6    red
14 => 3  D7    green
23 => (activity LED)
*/

#include <iostream>
#include <pigpiod_if2.h>
#include <unistd.h>

const int CLOCK = 4;
const int FREQ = 3579545;
const int READY = 24;
const int STROBE = 25;
const int LED = 23;
const int DATA_IOS[8] = {15, 18, 7, 8, 9, 10, 11, 14};
const int DATA_BITS[8] = {1<<15, 1<<18, 1<<7, 1<<8, 1<<9, 1<<10, 1<<11, 1<<14};

class Beeper {
  int pi;

public:
  Beeper() {
    pi = pigpio_start(NULL, NULL);
    set_mode(pi, CLOCK, PI_OUTPUT);
    set_mode(pi, STROBE, PI_OUTPUT);
    set_mode(pi, READY, PI_INPUT);
    for(int i = 0; i < 8; ++i)
      set_mode(pi, DATA_IOS[i], PI_OUTPUT);
    hardware_clock(pi, CLOCK, FREQ);
    silence();
  }

  ~Beeper() {
    silence();
    pigpio_stop(pi);
  }

  void silence() {
    for(int c = 0; c < 4; ++c)
      set_attenuator(c, 31);
  }

  // ch 0..3, val 0 (loudest) .. 31 (silent)
  void set_attenuator(int ch, int val) {
     int cmd = (val & 0xF);
     cmd |= ((ch & 0x3) << 5);
     cmd |= 0x90;
     send_byte(cmd);
  }

  // ch 0..2
  void set_frequency(int ch, int freq) {
    int val = FREQ / (32 * freq);
    int cmd = (val & 0xF);
    cmd |= ((ch & 0x3) << 5);
    cmd |= 0x80;
    send_byte(cmd);

    cmd = (val >> 4) & 0x7f;
    send_byte(cmd);
  }

  // fb 0 (periodic) .. 1 (white), rate 0..3
  void set_noise(int fb, int rate) {
    int cmd = rate & 3;
    cmd |= ((fb & 1) << 2);
    cmd |= 0xe0;
    send_byte(cmd);
  }

  void send_byte(int ch) {
    int set = 0, clear = 0;
    gpio_write(pi, LED, 1);
    for(int bit = 0; bit < 8; ++bit) {
      if (ch & (1 << bit))
        set |= DATA_BITS[bit];
      else
        clear |= DATA_BITS[bit];
    }
    set_bank_1(pi, set);
    clear_bank_1(pi, clear);
    gpio_trigger(pi, STROBE, 10, 0);
    gpio_write(pi, LED, 0);
  }

};

const int CHORD[] = {262, 311, 392};

int main(int argc, char **argv)
{
  Beeper beeper;

  for(int ch = 0; ch < 3; ++ch) {
    beeper.set_attenuator(ch, 9 - ch);
    beeper.set_frequency(ch, CHORD[ch]);
    usleep(700000);
  }
  usleep(1400000);
  beeper.silence();

  usleep(500000);

  beeper.set_attenuator(3, 0);
  for(int fb = 0; fb < 2; ++fb) {
    for(int z = 0; z < 4; ++z) {
      beeper.set_noise(fb, z);
      usleep(300000);
    }
  }

  return 0;
}

