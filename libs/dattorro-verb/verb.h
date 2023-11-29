typedef float t_sample;
struct sDattorroVerb;

/* Get pointer to initialized DattorroVerb struct */
struct sDattorroVerb* DattorroVerb_create(void);

/* Free resources and delete DattorroVerb instance */
void DattorroVerb_delete(struct sDattorroVerb* v);

/* Set reverb parameters */
void DattorroVerb_setPreDelay(struct sDattorroVerb* v, t_sample value);
void DattorroVerb_setPreFilter(struct sDattorroVerb* v, t_sample value);
void DattorroVerb_setInputDiffusion1(struct sDattorroVerb* v, t_sample value);
void DattorroVerb_setInputDiffusion2(struct sDattorroVerb* v, t_sample value);
void DattorroVerb_setDecayDiffusion(struct sDattorroVerb* v, t_sample value);
void DattorroVerb_setDecay(struct sDattorroVerb* v, t_sample value);
void DattorroVerb_setDamping(struct sDattorroVerb* v, t_sample value);

/* Send mono input into reverbation tank */
void DattorroVerb_process(struct sDattorroVerb* v, t_sample in);

/* Get reverbated signal for left channel */
t_sample DattorroVerb_getLeft(struct sDattorroVerb* v);

/* Get reverbated signal for right channel */
t_sample DattorroVerb_getRight(struct sDattorroVerb* v);
