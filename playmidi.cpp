#include <iostream>
#include <unistd.h>
#include <math.h>
extern "C" {
#include "midi.h"
}
#include "beeper.h"

struct Note {
  int ref;
  int val;
  int seq;
  int atten;
  bool decaying;

  Note() : ref(0), val(0), seq(0), atten(15), decaying(false) {}
};

const int POLYPHONY = 3;

class Player {
  Beeper beeper;
  int freq_table[128];
  int seq;
  Note playing[POLYPHONY];

  int velocity_to_attenuator(int vel) {
    // MIDI: 0 (silent) to 127 (loudest)
    // beeper: 15 (silent) to 0 (loudest)
    if (vel < 0)
      vel = 0;
    else if (vel > 127)
      vel = 127;

    return (127 - vel) / 8;
  }

public:
  Player() {
    for(int i = 0; i < 128; ++i) {
      freq_table[i] = (int)(440.0 * pow(2, (i - 69.0) / 12));
    }
    seq = 0;
  }

  void note_off(int ch, int note, int vel) {
    for(int i = 0; i < POLYPHONY; ++i) {
      if (playing[i].val == note) {
        if (--playing[i].ref == 0) {
          playing[i].val = 0;
          playing[i].decaying = true;
        }
      }
    }
  }

  void note_on(int ch, int note, int vel) {
    // sanity check
    if (note < 0 || note >= 128)
      return;

    // some midi files are dumb this way
    if (vel == 0) {
      note_off(ch, note, vel);
      return;
    }

    if (ch == 9) {
      // percussion channel
      // TODO: map this to the beeper's noise channel!
      return;
    }

    // see if this note is already playing, and increment the ref count
    int i;
    for(i = 0; i < POLYPHONY; ++i) {
      if (playing[i].val == note) {
        ++playing[i].ref;
        return;
      }
    }

    // find an available output channel
    for(i = 0; i < POLYPHONY; ++i) {
      if (playing[i].val == 0 && !playing[i].decaying)
        break;
    }

    // if all channels are in use, pick one where a note is in decay
    if (i == POLYPHONY) {
      for(i = 0; i < POLYPHONY; ++i) {
        if (playing[i].val == 0)
          break;
      }
    }

    // all our output channels are actively playing, so ... preempt the oldest one :(
    if (i == POLYPHONY) {
      i = 0;
      for(int j = 1; j < POLYPHONY; ++j) {
        if (playing[j].seq < playing[i].seq) {
          i = j;
        }
      }
    }

    // play the note!
    playing[i].val = note;
    playing[i].ref = 1;
    playing[i].seq = ++seq;
    playing[i].atten = velocity_to_attenuator(vel);
    beeper.set_frequency(i, freq_table[note]);
    beeper.set_attenuator(i, playing[i].atten);
  }

  void decay()
  {
    for(int i = 0; i < POLYPHONY; ++i) {
      if (playing[i].decaying) {
        if (++playing[i].atten >= 15) {
          playing[i].decaying = false;
        } else {
          beeper.set_attenuator(i, playing[i].atten);
        }
      }
    }
  }
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: playmidi file.mid" << std::endl;
    return 1;
  }

  midi_file *midi = midi_open_file(argv[1]);
  if (!midi) {
    std::cerr << "Failed to open file " << argv[1] << std::endl;
    return 1;
  }

  midi_trks *trks = midi_parse_tracks(midi);
  if (!trks) {
    std::cerr << "Failed to parse MIDI file" << std::endl;
    return 1;
  }

  midi_combine_tracks(trks);
  midi_convert_deltatime(trks);

  midi_hdr hdr;
  midi_get_header(midi, &hdr);

  int tempo = 500000;

  Player player;
  for(midi_evt_node *node = trks->trk[0]; node != NULL; node = node->next) {
    switch(node->evt) {
    case midi_meta_evt:
      if (node->meta == midi_set_tempo)
        tempo = node->param1;
    case midi_noteoff:
      player.note_off(node->chan, node->param1, node->param2);
      break;
    case midi_noteon:
      player.note_on(node->chan, node->param1, node->param2);
      break;
    }
    if (node->time > 0) {
      int us = (node->time * tempo) / hdr.division;
      while(us > 50000) {
        usleep(45000);  // HAX. compensate for some overhead :P
        us -= 50000;
        player.decay();
      }
      usleep(us);
    }
  }

  midi_free_tracks(trks);
  midi_close(midi);
  return 0;
}
