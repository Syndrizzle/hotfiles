
/* Settings for smoothing functions and transformations commonly
   used to display FFT output.

   IMPORTANT: THESE VALUES CAN BE OVERRIDDEN IN MODULE CONFIG
              FILES, IF CHANGING VALUES HERE DOES NOT WORK, CHECK
              TO MAKE SURE THEY ARE NOT BEING SET ELSEWHERE.
*/

/* The type of formula to use for weighting values when smoothing.
   Possible values:
   
   - circular     heavily rounded points
   - sinusoidal   rounded at both low and high weighted values
                    like a sine wave
   - linear       not rounded at all; linear distance
   */
#define ROUND_FORMULA sinusoidal

/* The sampling mode for processing raw FFT input:
   
   - average     averages all the inputs in the sample range for
                   a given point. Produces smooth output, but peaks
                   are not well represented
   - maximum     obtains the best value from the closest peak in
                   the sample range. Very accurate peaks, but
                   output is jagged and sporadic.
   - hybrid      uses the results from both `average` and `maximum`
                   with the weight provided in `SAMPLE_HYBRID_WEIGHT` */
#define SAMPLE_MODE average
/* Weight should be provided in the range (0, 1). Higher values favour
   averaged results. `hybrid` mode only. */
#define SAMPLE_HYBRID_WEIGHT 0.65

/* Factor used to scale frequencies. Lower values allows lower
   frequencies to occupy more space. */
#define SAMPLE_SCALE 8

/* The frequency range to sample. 1.0 would be the entire FFT output,
   and lower values reduce the displayed frequencies in a log-like
   scale. */
#define SAMPLE_RANGE 0.9

/* Factor for how to scale higher frequencies. Used in a linear equation
   which is multiplied by the result of the fft transformation. */
#request setfftscale 10.2

/* Cutoff for the bass end of the audio data when scaling frequencies.
   Higher values cause more of the bass frequencies to be skipped when
   scaling. */
#request setfftcutoff 0.3

/* How many frames to queue and run through the average function.
   Increasing this value will create latency between the audio and the
   animation, but will make for much smoother results. */
#request setavgframes 6

/* Whether to window frames ran through the average function (new & old
   frames are weighted less). This massively helps smoothing out
   spontaneous values in the animation. */
#request setavgwindow true

/* Gravity step, higher values means faster drops. The step is applied
   in a rate independant method like so:
   
   val -= (gravitystep) * (seconds per update) */
#request setgravitystep 4.2

/* Smoothing factor. Larger values mean more smoothing in the output,
   however high values can be expensive to compute. Values are in
   normalized width: [0.0, 1.0) */
#request setsmoothfactor 0.025

/* Whether to use a separate pass for audio data while smoothing. On
   most hardware, this will improve performance, but involves doing a
   separate render step for each audio texture and will add some driver
   (CPU) overhead. */
#request setsmoothpass true
