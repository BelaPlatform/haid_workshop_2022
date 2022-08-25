#include "ASR.h"
#include <math.h>

ASR::ASR(float attackT, float releaseT, float fs, EnvState initState)
{
	setup(attackT, releaseT, fs, initState);
}

int ASR::setup(float attackT, float releaseT, float fs, EnvState initState)
{
	fs_ = fs;
	setAttackTime(attackT);
	setAttackCurve(LINEAR);
	setReleaseTime(releaseT);
	setReleaseCurve(LINEAR);
	if(initState == OFF || initState == SUSTAIN)
		setState(initState);

	return 0;
}

void ASR::setAttackTime(float time)
{
	attackS_ = (int)(time * fs_);
	attackIncr_ = 1.0 / attackS_;
}

float ASR::getAttackT()
{
	return attackS_ / fs_;
}

void ASR::setReleaseTime(float time)
{
	releaseS_ = (int)(time * fs_);
	releaseIncr_ = 1.0 / releaseS_;
}

float ASR::getReleaseT()
{
	return releaseS_ / fs_;
}

void ASR::setState(EnvState state)
{
	state_ = state;
}

ASR::EnvState ASR::getCurrentState()
{
	return state_;
}

void ASR::triggerOn()
{
	state_ = ATTACK;
	count_ = 0;
	linearOut_ = 0.0;
}

void ASR::switchOn()
{
	state_ = SUSTAIN;
	linearOut_ = 1.0;
}

void ASR::triggerOff()
{
	state_ = RELEASE;
	count_ = 0;
	linearOut_ = 1.0;
}

void ASR::switchOff()
{
	state_ = OFF;
	linearOut_ = 0.0;
}

void ASR::trigger(bool on)
{
	if(on)
		triggerOn();
	else
		triggerOff();
}

void ASR::trigger()
{
	if(state_ == OFF)
		triggerOn();
	else if(state_ == SUSTAIN)
		triggerOff();
}

float ASR::process()
{

	/* ATTACK */
	if(state_ == ATTACK)
	{
		//linearOut_ = count_ * attackIncr_;
		linearOut_ += attackIncr_;

		++count_;
		if(count_ >= attackS_ || linearOut_ > 1)
		{
			linearOut_ = 1.0;
			out_ = 1.0;
			// Change state
			state_ = SUSTAIN;
		}
		else
		{
			if( attackCurve_ == TRIGONOMETRIC )
			{
				out_ = trigonometricOut(linearOut_, true);
			}
			else if( attackCurve_ == SQUARE_LAW )
			{
				out_ = squareLawOut(linearOut_, true);
			}
			else
			{
				out_ = linearOut_;
			}
		}
	}
	/* SUSTAIN */
	else if(state_ == SUSTAIN)
	{
		out_ = 1.0;
	}
	/* RELEASE */
	else if(state_ == RELEASE)
	{
		//linearOut_ = 1 - count_ * releaseIncr_;
		linearOut_ -= releaseIncr_;

		++count_;
		if(count_ >= releaseS_ || linearOut_ < 0)
		{
			linearOut_ = 0.0;
			out_ = 0.0;
			// Change state
			state_ = OFF;
		}
		else
		{
			if( attackCurve_ == TRIGONOMETRIC )
			{
				out_ = trigonometricOut(linearOut_, false);
			}
			else if( attackCurve_ == SQUARE_LAW )
			{
				out_ = squareLawOut(linearOut_, false);
			}
			else
			{
				out_ = linearOut_;
			}
		}
	}
	/* OFF */
	else if(state_ == OFF)
	{
		out_ = 0.0;
	}

	return out_;
}

float ASR::squareLawOut(float x, bool up)
{
	float out = 0.0;
	if(!up)
	{
		out = 1.0 - ( 0.25 * x * x);
	}
	else
	{
		out = 0.5 * x - 0.5;
		out = 1 - out * out;
	}
	return out;
}

float ASR::trigonometricOut(float x, bool up)
{
	float out = 0.0;
	if(!up)
	{
		out = cos(0.5 * M_PI * x);
	}
	else
	{
		out = sin(0.5 * M_PI * x);
	}
	return out;
}

void ASR::setAttackCurve(EnvCurve curve)
{
	attackCurve_ = curve;
}

ASR::EnvCurve ASR::getAttackCurve()
{
	return attackCurve_;
}

void ASR::setReleaseCurve(EnvCurve curve)
{
	releaseCurve_ = curve;
}

ASR::EnvCurve ASR::getReleaseCurve()
{
	return releaseCurve_;
}
