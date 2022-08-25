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
#include "ASR/ASR.h"
#include <cmath>

/* Audio output channels for driver (haptic output) and headphones */
unsigned int kHapticOutputCh = 1;
unsigned int kAudibleOutputCh = 0;

/* Digital input for button */
unsigned int kButtonInput = 0; // Connect button to digital input (see diagram on the IDE)
int gPreviousButtonState = 0;


/** OSCILLATORS **/
// Oscillator (aliased) objects
Oscillator hapticOsc;	// Haptic oscillator (for driver) 
Oscillator audibleOsc;	// Audible oscillator

// Frequency range for oscillator controls
float gHapticOscFreqRange[2] = {10, 200.0}; 	// Haptic oscillator
float gAudibleOscFreqRange[2] = {200, 440};		// Audible oscillator

bool gOscToggle = false; // Toggle oscillator(s) on (true) and off (false)

float gOscOnTimeRange[2] = {5.0, 120.0};	// Time that the oscillator is on (range for min and max values) (milliseconds)
int gOscOnCount = 0;			// Counter for toggling oscillator on/off

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

/** ENVELOPE **/
ASR asrEnvelope; // Attack-Sustain-Release

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
	gAudibleAmplitudeSliderId = controller.addSlider("Audio Amplitude", 0.15, 0, 0.5, 0.001);									// Amplitude of Audible output
	gOscOnTimeSliderId = controller.addSlider("Osc On T [ms]", (int)(0.5*(gOscOnTimeRange[1] - gOscOnTimeRange[0])), gOscOnTimeRange[0], gOscOnTimeRange[1], 1);	// Time that oscillator is on 

	/** BUTTON **/
	pinMode(context, 0, kButtonInput, INPUT); // set button's digital to input

	/** ENVELOPE **/
	asrEnvelope.setup(gOscOnTimeRange[0]*0.001, gOscOnTimeRange[0]*0.001, context->audioSampleRate);

	return true;
}

void render(BelaContext *context, void *userData)
{
	// Access the sliders specifying the index we obtained when creating then to retrieve their current value
	float hapticOscFreq = controller.getSliderValue(gHapticOscFreqSliderId);		// Frequency of haptic oscillator
	float hapticAmplitude = controller.getSliderValue(gHapticAmplitudeSliderId);	// Amplitude of Haptic output
	float audibleAmplitude = controller.getSliderValue(gAudibleAmplitudeSliderId);	// Amplitude of Audible output
	float oscOnTimeMs = controller.getSliderValue(gOscOnTimeSliderId);				// Time that oscillator is on 
	// [note] There is no smoothing for amplitude and frequency, you will get clicks when the values change

	
	hapticOsc.setFrequency(hapticOscFreq);	// Set frequency of haptic oscillator

	// Get frequency for audio oscillator by remapping frequency of haptic oscillator to appropriate range
	float audibleOscFreq = map(hapticOscFreq, gHapticOscFreqRange[0], gHapticOscFreqRange[1], gAudibleOscFreqRange[0], gAudibleOscFreqRange[1]);
	audibleOsc.setFrequency(audibleOscFreq); // Set frequency of 

	asrEnvelope.setAttackTime(0.5*oscOnTimeMs*0.001);
	asrEnvelope.setReleaseTime(0.5*oscOnTimeMs*0.001);
	
	/* Frame loop (audio rate) */
	for(unsigned int n = 0; n < context->audioFrames; n++)
	{
		
		/** DIGITAL **/
		// Read the state of the button
		int buttonState = digitalRead(context, n, kButtonInput);
		
		// If button pressed...
		// ... and button was not pressed before
		if(buttonState != gPreviousButtonState)	
		{
			asrEnvelope.triggerOn(); // Trigger envelope!!!
			rt_printf("Trigger envelope!\n");
		}
		// Update previous state to current state
		gPreviousButtonState = buttonState;
		// [note] There is no debouncing for the button so miss-triggers might occur
	
		
		// If envelope reaches the SUSTAIN state (ATTACK state finishes)
		if(asrEnvelope.getCurrentState() == ASR::SUSTAIN)
		{
				asrEnvelope.triggerOff(); // Release envelope!!!
				rt_printf("Release envelope!\n");
		}
	
		
		/** AUDIO **/
		// Compute output by scaling with the amplitude
		float envelope = asrEnvelope.process();
		float hapticOut = hapticOsc.process() * hapticAmplitude * envelope;		// Haptic output
		float audibleOut = audibleOsc.process() * audibleAmplitude * envelope;	// Audio output
		
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
