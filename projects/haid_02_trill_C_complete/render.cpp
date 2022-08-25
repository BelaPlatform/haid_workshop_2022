/*
 ____  _____ _        _
| __ )| ____| |      / \
|  _ \|  _| | |     / _ \
| |_) | |___| |___ / ___ \
|____/|_____|_____/_/   \_\
http://bela.io
*/
/**
\example Gui/sliders/render.cpp

*/

#include <Bela.h>
#include <libraries/Oscillator/Oscillator.h>
#include <libraries/Gui/Gui.h>
#include <libraries/GuiController/GuiController.h>
#include <libraries/Trill/Trill.h>

#include <cmath>

/* Audio output channels for driver (haptic output) and headphones */
unsigned int kHapticOutputCh = 1;
unsigned int kAudibleOutputCh = 0;

/** OSCILLATORS **/
// Oscillator (aliased) objects
Oscillator hapticOsc;	// Haptic oscillator (for driver) 
Oscillator audibleOsc;	// Audible oscillator

// Frequency range for oscillator controls
float gHapticOscFreqRange[2] = {10, 200.0}; 	// Haptic oscillator
float gAudibleOscFreqRange[2] = {200, 440};		// Audible oscillator

bool gOscToggle = false; // Toggle oscillator(s) on (true) and off (false)

float gOscOnTimeRange[2] = {5.0, 120.0};	// Time that the oscillator is on (range for min and max values) (milliseconds)
int gOscOnCount = 0;	

/** GUI **/
// GUI instance
Gui gui;
// GUI controller instance
GuiController controller;

// Indices for different slider controls
unsigned int gHapticOscFreqSliderId;	// Frequency of haptic oscillator
unsigned int gHapticAmplitudeSliderId;	// Amplitude of Haptic output
unsigned int gAudibleAmplitudeSliderId;	// Amplitude of Audible output
unsigned int gOscOnTimeSliderId;		// Time that oscillator is on 

/** TRILL **/
#define NUM_TOUCH 5 // Number of touches on Trill sensor

// Trill object declaration
Trill touchSensor;

// Location of touches on Trill Bar
float gTouchLocation[NUM_TOUCH] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
// Size of touches on Trill Bar
float gTouchSize[NUM_TOUCH] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
// Number of active touches
int gNumActiveTouches = 0;
// Previous number of active touches
int gPrevNumActiveTouches = 0;

// Sleep time for auxiliary task in microseconds
unsigned int gTaskSleepTime = 5000; // microseconds

/*
 * Function to be run on an auxiliary task that reads data from the Trill sensor.
 * Here, a loop is defined so that the task runs recurrently for as long as the
 * audio thread is running.
 */
void loop(void*)
{
	while(!Bela_stopRequested())
	{
		// Read locations from Trill sensor
		touchSensor.readI2C();
		gNumActiveTouches = touchSensor.getNumTouches();
		for(unsigned int i = 0; i < gNumActiveTouches; i++)
		{
			gTouchLocation[i] = touchSensor.touchLocation(i);
			gTouchSize[i] = touchSensor.touchSize(i);
		}
		// For all inactive touches, set location and size to 0
		for(unsigned int i = gNumActiveTouches; i <  NUM_TOUCH; i++)
		{
			gTouchLocation[i] = 0.0;
			gTouchSize[i] = 0.0;
		}
		usleep(gTaskSleepTime);
	}
}

bool setup(BelaContext *context, void *userData)
{
	// Setup Haptic Oscillator
	hapticOsc.setup(context->audioSampleRate);
	hapticOsc.setType(Oscillator::sine);
	// Setup Audio Oscillator
	audibleOsc.setup(context->audioSampleRate);
	audibleOsc.setType(Oscillator::sine);

	
	/** GUI **/
	// Setup GUI
	gui.setup(context->projectName);
	// ...and attach gui controllerto it
	controller.setup(&gui, "Controls");

	// Setup sliders
	// Arguments: name, default value, minimum, maximum, increment (store the return value to read from the slider later on)
	gHapticOscFreqSliderId = controller.addSlider("Haptic OSC freq",(int)(0.5*(gHapticOscFreqRange[1] - gHapticOscFreqRange[0])) , gHapticOscFreqRange[0], gHapticOscFreqRange[1], 1);	// Frequency of haptic oscillator
	gHapticAmplitudeSliderId = controller.addSlider("Haptic Amplitude", 1.0, 0, 1.0, 0.001);									// Amplitude of Haptic output
	gAudibleAmplitudeSliderId = controller.addSlider("Audio Amplitude", 0.15, 0, 0.5, 0.001);										// Amplitude of Audible output
	gOscOnTimeSliderId = controller.addSlider("Osc On T [ms]", (int)(0.5*(gOscOnTimeRange[1] - gOscOnTimeRange[0])), gOscOnTimeRange[0], gOscOnTimeRange[1], 1);	// Time that oscillator is on 

	/** TRILL **/
	// Setup a Trill Bar sensor on i2c bus 1, using the default mode and address
	if(touchSensor.setup(1, Trill::BAR) != 0) {
		fprintf(stderr, "Unable to initialise Trill Bar\n");
		return false;
	}

	touchSensor.printDetails();

	// Set and schedule auxiliary task for reading sensor data from the I2C bus
	Bela_runAuxiliaryTask(loop);
	
	return true;
}

void render(BelaContext *context, void *userData)
{
	// Access the sliders specifying the index we obtained when creating then to retrieve their current value
	float hapticMinOscFreq = controller.getSliderValue(gHapticOscFreqSliderId);		// Frequency of haptic oscillator
	float hapticAmplitude = controller.getSliderValue(gHapticAmplitudeSliderId);	// Amplitude of Haptic output
	float audibleAmplitude = controller.getSliderValue(gAudibleAmplitudeSliderId);		// Amplitude of Audible output
	float oscOnTimeMs = controller.getSliderValue(gOscOnTimeSliderId);				// Time that oscillator is on 

	// [note] There is no smoothing for amplitude and frequency, you will get clicks when the values change




	/* Frame loop (audio rate) */
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
		// If there is a new touch
		if(gNumActiveTouches != gPrevNumActiveTouches && gPrevNumActiveTouches == 0)
		{
			gOscOnCount = (int)(context->audioSampleRate * oscOnTimeMs * 0.001); // Reset counter
			
			// Scale haptic oscillator frequency based on touch location of first touch
			float hapticOscFreq = map(gTouchLocation[0], 0, 1, hapticMinOscFreq, gHapticOscFreqRange[1]);
			hapticOsc.setFrequency(hapticOscFreq);	// Set frequency of haptic oscillator
			
			// Get frequency for audio oscillator by remapping frequency of haptic oscillator to appropriate range
			float audibleOscFreq = map(hapticOscFreq, gHapticOscFreqRange[0], gHapticOscFreqRange[1], gAudibleOscFreqRange[0], gAudibleOscFreqRange[1]);
			audibleOsc.setFrequency(audibleOscFreq); // Set frequency of 

		}
		// Update previous number to current number
		gPrevNumActiveTouches = gNumActiveTouches;
		
		/** COUNT **/
		// If count has elapsed...
		if(gOscOnCount >= 0)
		{
			gOscToggle = true; // Toggle oscillator on
			gOscOnCount--;	// Decrement counter
		}
		// otherwise ...
		else
		{
			gOscToggle = false;	// Toggle oscillator off
		}
		
		
		// Compute otput by scaling with the gain and including AM modulation
		float hapticOut = hapticOsc.process() * hapticAmplitude  * gOscToggle;		// Haptic output
		float audibleOut = audibleOsc.process() * audibleAmplitude  * gOscToggle;	// Audio output
		
		// Iterate over audio channels
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++)
		{
			float out = 0.0;
			if(channel == kAudibleOutputCh)			// If channel matches channel assigned to audible output... 
				out = audibleOut;
			else if (channel == kHapticOutputCh) 	// If channel matches channel assigned to haptic output... 
				out = hapticOut;
			// Write output to channel
			audioWrite(context, n, channel, out);
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{}
