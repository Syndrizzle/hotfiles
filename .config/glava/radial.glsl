/* center radius (pixels) */
#define C_RADIUS 80

/* center line thickness (pixels) */
#define C_LINE 2

/* outline color --- (R, G, B, transparency)*/
#define OUTLINE vec4(0, 0, 0, 0)

/* number of bars (use even values for best results) */
#define NBARS 50

/* width (in pixels) of each bar*/
#define BAR_WIDTH 10

/* outline color */
#define BAR_OUTLINE OUTLINE

/* outline width (in pixels, set to 0 to disable outline drawing) */
#define BAR_OUTLINE_WIDTH 0

/* Amplify magnitude of the results each bar displays */
#define AMPLIFY 95

/* Bar color */
#define COLOR mix(#7dcfff, #78a0f7, clamp(d / 80, 0, 1))

/* Angle (in radians) for how much to rotate the visualizer */
#define ROTATE (PI / 360)

/* Whether to switch left/right audio buffers */
#define INVERT 0

/* Smoothing factor, in normalized width */
#define SMOOTH 0.02

/* How many frames to queue and run through the average function */
#request setavgframes 6

/* Whether to window frames ran through the average function (new & old frames
   are weighted less). This massively helps smoothing out spikes in the animation */
#request setavgwindow true

/* Gravity step, higher values means faster drops. The step is applied in a rate
   independant method like so:
   
   val -= (gravitystep) * (seconds per update) */
#request setgravitystep 6
