/*
Copyright (C) 2011 Georgia Institute of Technology

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
* Generates square pulse current commands.
*/

#include <ia-activate.h>

extern "C" Plugin::Object *createRTXIPlugin(void) {
	return new IAact();
}

static DefaultGUIModel::variable_t vars[] =
{
	{ "Vin", "", DefaultGUIModel::INPUT, },
	{ "Iout", "", DefaultGUIModel::OUTPUT, },
	{ "Period (s)", "Duration of one cycle", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Delay (s)", "Time until step starts from beginning of cycle",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Min Amp (pA)", "Starting current of the steps",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Max Amp (pA)", "Ending current of the steps", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Increments", "How many steps to take between min and max",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER, },
	{ "Cycles (#)", "How many times to repeat the protocol",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER, },
	{ "Duty Cycle (%)", "On time of the step during a single cycle",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Fixed Depolarization (pA)", "Value of the depolarization current",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Depolarization Time (s)", "Time current is at depolarized value",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Offset (pA)", "DC offset to add", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, }, 
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

IAact::IAact(void) : DefaultGUIModel("IA Activation", ::vars, ::num_vars), dt(RT::System::getInstance()->getPeriod() * 1e-6), period(1.0), delay(0.0), Amin(-210.0), Amax(0.0), Nsteps(8), Ncycles(1), duty(15), fixedDepol(150), depolTime(1.0), offset(0.0) {
	setWhatsThis("<p><b>I-Step:</b><br>This module generates a train of current injection pulses with amplitudes between a user-specified minimum and maximum.</p>");
	createGUI(vars, num_vars);
	update(INIT);
	refresh();
	resizeMe();
}

IAact::~IAact(void) {}

void IAact::execute(void) {
	V = input(0);
	
	Iout = offset;
	if (cycle < Ncycles) {
		
		//Do all time keeping in seconds.
		if (step < Nsteps) {
			if (age >= delay && age < delay + period * (duty / 100) - EPS) {
				Iout = Iout + Amin + step * deltaI;
				age += dt / 1000;
			}
			else if(interage<depolTime) {
				Iout = fixedDepol;
				interage += dt / 1000;
			}
			
			else{
				age += dt/1000;
			}
			
			if (age >= (period - EPS)) {
				step++;
				interage = 0;
				age = 0;
			}
			
		}
		if (step == Nsteps) {
			cycle++;
			step = 0;
		}
	}
	output(0) = Iout * 1e-12;
}

void IAact::update(DefaultGUIModel::update_flags_t flag) {
	switch (flag) {
		case INIT:
			setParameter("Period (s)", period);
			setParameter("Delay (s)", delay);
			setParameter("Min Amp (pA)", Amin);
			setParameter("Max Amp (pA)", Amax);
			setParameter("Increments", Nsteps);
			setParameter("Cycles (#)", Ncycles);
			setParameter("Duty Cycle (%)", duty);
			setParameter("Fixed Depolarization (pA)", fixedDepol);
			setParameter("Depolarization Time (s)", depolTime);
			setParameter("Offset (pA)", offset);
			break;

		case MODIFY:
			period = getParameter("Period (s)").toDouble();
			delay = getParameter("Delay (s)").toDouble();
			Amin = getParameter("Min Amp (pA)").toDouble();
			Amax = getParameter("Max Amp (pA)").toDouble();
			Nsteps = getParameter("Increments").toInt();
			Ncycles = getParameter("Cycles (#)").toInt();
			duty = getParameter("Duty Cycle (%)").toDouble();
			fixedDepol = getParameter("Fixed Depolarization (pA)").toDouble();
			depolTime = getParameter("Depolarization Time (s)").toDouble();
			offset = getParameter("Offset (pA)").toDouble();
			break;

		case PAUSE:
			output(0) = 0;
	
		case PERIOD:
			dt = RT::System::getInstance()->getPeriod() * 1e-6;
	
		default:
			break;
	}
	
	// Some Error Checking for fun
	
	if (period <= 0) {
		period = 1;
		setParameter("Period (sec)", period);
	}
	
	if (Amin > Amax) {
		Amax = Amin;
		setParameter("Min Amp (pA)", Amin);
		setParameter("Max Amp (pA)", Amax);
	}
	
	if (Ncycles < 1) {
		Ncycles = 1;
		setParameter("Cycles (#)", Ncycles);
	}
	
	if (Nsteps < 0) {
		Nsteps = 0;
		setParameter("Increments", Nsteps);
	}
	
	if (duty < 0 || duty > 100) {
		duty = 0;
		setParameter("Duty Cycle (%)", duty);
	}
	
	if (delay <= 0 || delay > period * duty / 100) {
		delay = 0;
		setParameter("Delay (sec)", delay);
	}
	
	//Define deltaI based on params
	if (Nsteps > 1) {
		deltaI = (Amax - Amin) / (Nsteps - 1);
	}
	else {
		deltaI = 0;
	}
	
	//Initialize counters
	age = 0;
	step = 0;
	cycle = 0;
	interage = 0;	
}
