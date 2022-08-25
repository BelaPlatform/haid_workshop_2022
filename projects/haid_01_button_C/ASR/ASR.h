#pragma once

class ASR
{
	public:
		ASR(){};
		~ASR(){};

		typedef enum {
			ATTACK = 0,
			SUSTAIN,
			RELEASE,
			OFF
		} EnvState;

		typedef enum {
			LINEAR = 0,
			TRIGONOMETRIC,
			SQUARE_LAW
		} EnvCurve;

		ASR(float attackT, float releaseT, float fs, EnvState initState = OFF);
		int setup(float attackT, float releaseT, float fs, EnvState initState = OFF);

		void setAttackTime(float time);
		float getAttackT();

		void setReleaseTime(float time);
		float getReleaseT();

		EnvState getCurrentState();

		void triggerOn();
		void triggerOff();
		void trigger();
		void trigger(bool on);

		void switchOn();
		void switchOff();

		void setAttackCurve(EnvCurve curve);
		EnvCurve getAttackCurve();

		void setReleaseCurve(EnvCurve curve);
		EnvCurve getReleaseCurve();

		float squareLawOut(float x, bool up = true);
		float trigonometricOut(float x, bool up = true);

		float process();

	private:
		float fs_;
		unsigned int attackS_; // Samples
		float attackIncr_; // Samples
		unsigned int releaseS_; // Samples
		float releaseIncr_; // Samples
		EnvState state_;

		EnvCurve attackCurve_ = LINEAR;
		EnvCurve releaseCurve_ = LINEAR;

		unsigned int count_ = 0;

		float linearOut_ = 0.0;
		float out_ = 0.0;

		void setState(EnvState state);
};
