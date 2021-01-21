#include <iostream>
#include <unistd.h>
#include "beeper.h"

const int CHORD[] = {262, 333, 392};

int main(int argc, char **argv)
{
  Beeper beeper;

  for(int ch = 0; ch < 3; ++ch) {
    beeper.set_attenuator(ch, 0);
    beeper.set_frequency(ch, CHORD[ch]);
    usleep(750000);
  }
  for(int atten = 1; atten < 16; ++atten) {
    beeper.set_attenuator(0, atten);
    beeper.set_attenuator(1, atten);
    beeper.set_attenuator(2, atten);
    usleep(150000);
  }
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
