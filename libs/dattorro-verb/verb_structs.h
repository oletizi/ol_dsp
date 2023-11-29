#include <cstdint>
typedef float t_sample;
enum {
  TAP_MAIN = 0,
  TAP_OUT1,
  TAP_OUT2,
  TAP_OUT3,
  MAX_TAPS
};

/* DelayBuffer context, also used in AllPassFilter */
typedef struct sDelayBuffer {
  // Sample buffer
  t_sample* buffer;

  // Mask for fast array index wrapping in read / write 
  uint16_t mask;

  // Read offsets
  uint16_t readOffset[MAX_TAPS];
} DelayBuffer;

/* DattorroVerb context */
typedef struct sDattorroVerb {
  // -- Reverb feedback network components --

  // Pre-delay
  DelayBuffer preDelay;       // Delay

  // Pre-filter
  t_sample      preFilter;      // LPF

  // input diffusors
  DelayBuffer inDiffusion[4]; // APF

  // Reverbation tank left / right halves
  DelayBuffer decayDiffusion1[2];  // APF
  DelayBuffer preDampingDelay[2];  // Delay
  t_sample      damping[2];          // LPF
  DelayBuffer decayDiffusion2[2];  // APF
  DelayBuffer postDampingDelay[2]; // Delay

  // -- Reverb settings --
  t_sample   preFilterAmount;

  t_sample   inputDiffusion1Amount;
  t_sample   inputDiffusion2Amount;

  t_sample   decayDiffusion1Amount;
  t_sample   dampingAmount;
  t_sample   decayAmount;
  t_sample   decayDiffusion2Amount;  // Automatically set in DattorroVerb_setDecay

  // Cycle count for syncing delay lines
  uint16_t t;
} DattorroVerb;
