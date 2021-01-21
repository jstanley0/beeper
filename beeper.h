#pragma once

class Beeper {
  int pi;

public:
  Beeper();
  ~Beeper();

  void silence();

  // ch 0..3, val 0 (loudest) .. 15 (silent)
  void set_attenuator(int ch, int val);

  // ch 0..2
  void set_frequency(int ch, int freq);

  // fb 0 (periodic) .. 1 (white), rate 0..3
  void set_noise(int fb, int rate);

private:
  void send_byte(int ch);
};

