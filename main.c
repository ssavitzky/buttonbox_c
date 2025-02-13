#include "SDL.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "time.h"
#include "math.h"

#include "keyboard.c"

#include "synthesis.c"

#include "config.c"

#include "display.c"

int main(int argc, char *argv[]) {

  uint32_t RunningSampleIndex = 1;

  int BufferSamples = 512;
  int16_t SampleOut[4096];
  void *SoundBuffer = (void *)SampleOut;

  setupSDL();

  initKeygrid();
  initNotes();

  // apply config from command line
  for (int i = 1; i < argc; i++) {
    char *arg = argv[i];
    char *pos = strchr(arg, '=');
    if (pos == NULL) { doConfigFile(arg); }
    else { doConfigLine(arg, ""); }
  }

  setDefaultInstrumentIfZero();

  initDisplay();

  int quit = 0;
  SDL_Event e;
  SDL_Keycode keysym = 0;

  while (! quit) {
    while (SDL_PollEvent(&e)){
      switch (e.type) {
      case SDL_QUIT:
        quit = 1;
        break;

      case SDL_KEYDOWN:
	if (e.key.keysym.sym == SDLK_ESCAPE) {
	  quit = 1;
	}

	else if (e.key.repeat == 0) {
	  keysym = e.key.keysym.sym;
	  if (keysym > 0 && keysym < 128 && keygrid[keysym] != -1) {
	    notes[keysym].instrument = instrument[keyboardForGrid(keygrid[keysym])];
	    notes[keysym].frequency = frequencyForNote(noteForGrid(keygrid[keysym]));
	    notes[keysym].onset = RunningSampleIndex;
	    notes[keysym].offset = 0;

	    drawKeyIcon(keygrid[keysym], 1);
	  }
	}
	break;

      case SDL_KEYUP:
	keysym = e.key.keysym.sym;
	if (keysym > 0 && keysym < 128 && keygrid[keysym] != -1) {
	  notes[keysym].offset = RunningSampleIndex;
	  drawKeyIcon(keygrid[keysym], 0);
	}
	break;
	
      }

    }

    int BytesToWrite = TargetQueueBytes - SDL_GetQueuedAudioSize(1);
    // printf("%i\n", BytesToWrite);

    for (int SampleIndex = 0;
	 SampleIndex < BytesToWrite / BytesPerSample;
	 ++SampleIndex) {
      SampleOut[SampleIndex*2] = 0;
      SampleOut[SampleIndex*2 + 1] = 0;
    }

    for (int n = 0; n < 128; n++) {
      if (notes[n].onset) {
	for (int SampleIndex = 0;
	     SampleIndex < BytesToWrite / BytesPerSample;
	     ++SampleIndex)
	  {
	    int16_t SampleValue = sampleValue(notes[n], RunningSampleIndex + SampleIndex);

	    SampleOut[SampleIndex*2] += SampleValue * (1 - notes[n].instrument.pan);
	    SampleOut[SampleIndex*2 + 1] += SampleValue * (1 + notes[n].instrument.pan);
	  }
      }
    }

    RunningSampleIndex += BytesToWrite / BytesPerSample;

    for (int n = 0; n < 128; n++) {
      if (isNoteFinished(notes[n], RunningSampleIndex)) {
	notes[n].onset = 0;
      }
    }

    SDL_QueueAudio(1, SoundBuffer, BytesToWrite);

    updateDisplay();

    SDL_Delay(500 * TargetQueueBytes / (BytesPerSample * SamplesPerSecond));
  }

  teardownSDL();
}
