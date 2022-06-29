/* center radius (pixels) */
#define C_RADIUS 128
/* center line thickness (pixels) */
#define C_LINE 2
/* outline color */
#define OUTLINE vec4(0, 0, 0, 1)
/* Amplify magnitude of the results each bar displays */
#define AMPLIFY 150
/* Angle (in radians) for how much to rotate the visualizer */
#define ROTATE (-PI / 2)
/* Whether to switch left/right audio buffers */
#define INVERT 0
/* Whether to fill in the space between the line and inner circle */
#define C_FILL 0
/* Whether to apply a post-processing image smoothing effect
   1 to enable, 0 to disable. Only works with `xroot` transparency,
   and improves performance if disabled. */
#define C_SMOOTH 0.02

/* Gravity step, overrude frin `smooth_parameters.glsl` */
#request setgravitystep 6.0

/* Smoothing factor, override from `smooth_parameters.glsl` */
#request setsmoothfactor 0.01

/* number of bars (use even values for best results) */
#define NBARS 50

/* width (in pixels) of each bar*/
#define BAR_WIDTH 6

/* outline color */
#define BAR_OUTLINE OUTLINE


/* outline width (in pixels, set to 0 to disable outline drawing) */
#define BAR_OUTLINE_WIDTH 0

/* Amplify magnitude of the results each bar displays */
#define AMPLIFY 300

/* Bar color */
/*            Color    R	 G	  B  trans  * ((d / x) + y)
  y = {0 = 1 opaqe color, < 1  = starts with grey, >1 = more bahet/brigtness}
  x = { higher values drives the gradient slower toward white
          Green is (0.09, 0.39, 0.09, 1) * ((d / 60) + 1)) */
#define COLOR (vec4(0.39, 0.39, 0.39, 1) * ((d / 60) + 1))

