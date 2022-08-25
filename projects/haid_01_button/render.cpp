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
#include <cmath>

/* Audio output channels for driver (haptic output) and headphones */
unsigned int kHapticOutputCh = 1;
unsigned int kAudibleOutputCh = 0;

/* Digital input for button */
unsigned int kButtonInput = 0; // Connect button to digital input (see diagram on the IDE)
unsigned int gPreviousButtonState = 0;


/** OSCILLATORS **/
// Oscillator (aliased) objects
Oscillator hapticOsc;	// Haptic oscillator (for driver) 
Oscillator audibleOsc;	// Audible oscillator
Oscillator amLfo;		// Amplitude modulation LFO (low frequency oscillator)

// Frequency range for oscillator controls
float gHapticOscFreqRange[2] = {10, 200.0}; 	// Haptic oscillator
float gAudibleOscFreqRange[2] = {200, 440};		// Audible oscillator
float gAmLFOFreqRange[2] = {0.0, 30.0};			// AM LFO

bool gOscToggle = false; // Toggle oscillator(s) on (true) and off (false)

/** GUI **/
// GUI instance
Gui gui;
// GUI controller instance
GuiController controller;

// Indices for different slider controls
unsigned int gHapticOscFreqSliderId;	// Frequency of haptic oscillator
unsigned int gAMLfoFreqSliderId;		// Frequency of AM LFO
unsigned int gHapticAmplitudeSliderId;	// Amplitude of Haptic output
unsigned int gAudibleAmplitudeSliderId;	// Amplitude of Audible output

bool setup(BelaContext *context, void *userData)
{
	// Setup Haptic Oscillator
	hapticOsc.setup(context->audioSampleRate);
	hapticOsc.setType(Oscillator::sine);
	// Setup Audio Oscillator
	audibleOsc.setup(context->audioSampleRate);
	audibleOsc.setType(Oscillator::sine);
	// Setup Amplitude Modulation LFO
	amLfo.setup(context->audioSampleRate);
	amLfo.setType(Oscillator::square);
	
	/** GUI **/
	// Setup GUI
	gui.setup(context->projectName);
	// ...and attach gui controllerto it
	controller.setup(&gui, "Controls");

	// Setup sliders
	// Arguments: name, default value, minimum, maximum, increment (store the return value to read from the slider later on)
	gHapticOscFreqSliderId = controller.addSlider("Haptic OSC freq",(int)(0.5*(gHapticOscFreqRange[1] - gHapticOscFreqRange[0])) , gHapticOscFreqRange[0], gHapticOscFreqRange[1], 1);	// Frequency of haptic oscillator
	gAMLfoFreqSliderId = controller.addSlider("AM LFO freq",gAmLFOFreqRange[0] , gAmLFOFreqRange[0], gAmLFOFreqRange[1], 1);	// Frequency of AM LFO
	gHapticAmplitudeSliderId = controller.addSlider("Haptic Amplitude", 1.0, 0, 1.0, 0.001);									// Amplitude of Haptic output
	gAudibleAmplitudeSliderId = controller.addSlider("Audio Amplitude", 0.15, 0, 0.5, 0.001);									// Amplitude of Audible output

	/** BUTTON **/
	pinMode(context, 0, kButtonInput, INPUT); // set button's digital to input

	return true;
}

void render(BelaContext *context, void *userData)
{
	// Access the sliders specifying the index we obtained when creating then to retrieve their current value
	float hapticOscFreq = controller.getSliderValue(gHapticOscFreqSliderId);		// Frequency of haptic oscillator
	float amLfoFreq = controller.getSliderValue(gAMLfoFreqSliderId);				// Frequency of AM LFO
	float hapticAmplitude = controller.getSliderValue(gHapticAmplitudeSliderId);	// Amplitude of Haptic output
	float audibleAmplitude = controller.getSliderValue(gAudibleAmplitudeSliderId);		// Amplitude of Audible output

	// [note] There is no smoothing for amplitude and frequency, you will get clicks when the values change

	
	hapticOsc.setFrequency(hapticOscFreq);	// Set frequency of haptic oscillator
	amLfo.setFrequency(amLfoFreq);			// Set frequency of AM LFO

	// Get frequency for audio oscillator by remapping frequency of haptic oscillator to appropriate range
	float audibleOscFreq = map(hapticOscFreq, gHapticOscFreqRange[0], gHapticOscFreqRange[1], gAudibleOscFreqRange[0], gAudibleOscFreqRange[1]);
	audibleOsc.setFrequency(audibleOscFreq); // Set frequency of 
	
	// Calculate Amplitude Modulation
	float amLfoVal;
	if(amLfoFreq == 0.0)	// If frequency is zero...
		amLfoVal = 1.0;		// .. no amplitude modulation
	else					// If frequency is not zero...
		amLfoVal = 0.5 * (amLfo.process() + 1); // .. re-scale oscillator output from (-1,1) to (0,1) 
	
	/* Frame loop (audio rate) */
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
		/** DIGITAL **/
		// Read the state of the button
		int buttonState = digitalRead(context, n, kButtonInput);
		
		// If button pressed...
		if(buttonState == 1)	
			gOscToggle = true;
		// If button released...
		else
			gOscToggle = false;
		
		/** AUDIO **/
		// Compute output by scaling with the amplitude
		float hapticOut = hapticOsc.process() * hapticAmplitude * gOscToggle;		// Haptic output
		float audibleOut = audibleOsc.process() * audibleAmplitude * gOscToggle;	// Audio output
		
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
