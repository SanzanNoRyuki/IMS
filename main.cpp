/**
 * @file main.cpp
 * @author Roman Fulla <xfulla00>
 * @author Pavol Szepsi <xszeps00>
 * @date 12.12.2021
 *
 * @brief Implementacia SHO vo vyrobe - Tovaren na etikety
 *
 **/

/* We simulate & measure time in minutes in this project */

#include <iostream>
#include "simlib.h"

using namespace std;

/* Simulation parameters */
#define DAY_COUNT 3650
#define HOURS_PER_ORDER 5

// Employees
#define SUPERVISOR_COUNT 1				// 1 Default
#define DESIGNER_COUNT 2				// 2 Default

// Machines
#define PRINTER_COUNT 3					// 3 Default
#define STAMPER_COUNT 1					// 1 Default
#define PLOTTER_COUNT 2					// 2 Default
#define FINISHING_MACHINES_COUNT 2		// 2 Default

/* Shared entities */
// Employees
Store supervisor("Supervisor", SUPERVISOR_COUNT);
Store designer("Designer", DESIGNER_COUNT);

// Machines
Facility printer[PRINTER_COUNT];
Facility stamper[STAMPER_COUNT];
Facility plotter[PLOTTER_COUNT];
Facility finishing_machine[FINISHING_MACHINES_COUNT];

/* Statistics */
// Histogram which outputs duration of orders in the system
Histogram order_duration("Duration of orders in the system", 0, 60, 20);

/* Order */
class Order : public Process {
private:
	bool stamped = false;
	double start_time = Time;

	// Process duurations
	double designing_duration = 0;
	double approval_duration  = 0;
	double printing_duration  = 0;
	double stamping_duration  = 0;
	double cutting_duration   = 0;
	double finishing_duration = 0;

	// Wait times
	double designer_wait_time          = 0;
	double printer_wait_time           = 0;
	double supervisor_wait_time        = 0;
	double stamper_wait_time           = 0;
	double plotter_wait_time           = 0;
	double finishing_machine_wait_time = 0;

	// Order steps
	void Designing() {
		double Entry = Time;
		Enter(designer, 1);
		designer_wait_time = Time - Entry;

		double redesign; 														// Designing process
		do {
			Wait(Exponential(7 * 60));
			redesign = Random();
		} while(redesign < 0.05);												// Complete redesign
		if (redesign < 0.95) {                   					            // Design adjustments
			Wait(Exponential(3 * 60));
		}

		Leave(designer, 1);
		designing_duration = Time - Entry - designer_wait_time;
	}

	void Printing() {
		double Entry = Time;
		int printer_idx = Seize_facility(printer, PRINTER_COUNT);
		printer_wait_time = Time - Entry;

		Wait(Exponential(30));													// Printer setup

		bool not_approved = true;												// Sample quality check
		double Approval_Entry = Time;
		do {
			double Supervisor_Entry = Time;
			Enter(supervisor, 1);
			supervisor_wait_time += Time - Supervisor_Entry;					// Cumulative

			Wait(Exponential(2));
			Leave(supervisor, 1);

			if (Random() < 0.15) Wait(Exponential(10));
			else not_approved = false;
		} while(not_approved);
		approval_duration = Time - Approval_Entry;

		double material = Random();												// Printing process(length depends on the material used)
		if      (material < 0.3) Wait(Exponential(10));
		else if (material < 0.9) Wait(Exponential(15));
		else 				     Wait(Exponential(30));

		Release(printer[printer_idx]);
		printing_duration = Time - Entry - printer_wait_time;
	}

	void Stamping() {
		stamped = true;

		double Entry = Time;
		int stamper_idx = Seize_facility(stamper, STAMPER_COUNT);
		stamper_wait_time = Time - Entry;

		Wait(45 + Uniform(3 * 60, 4 * 60));								     	// Stamper setup & process

		Release(stamper[stamper_idx]);
		stamping_duration = Time - Entry - stamper_wait_time;
	}

	void Cutting() {
		double Entry = Time;
		int plotter_idx = Seize_facility(plotter, PLOTTER_COUNT);
		plotter_wait_time = Time - Entry;

		Wait(Uniform(10, 15));													// Plotter setup
		if (stamped) Wait(Exponential(3 * 30));								    // Cutting process(3 times longer if stamped)
		else 		 Wait(Exponential(30));

		Release(plotter[plotter_idx]);
		cutting_duration = Time - Entry - plotter_wait_time;
	}

	void Finishing() {
		double Entry = Time;
		int finishing_machine_idx = Seize_facility(finishing_machine, FINISHING_MACHINES_COUNT);
		finishing_machine_wait_time = Time - Entry;

		if (stamped) Wait(Uniform(3 * 6, 3 * 11));	     			            // Finishing touches & packaging(3 times longer if stamped)
		else 		 Wait(Uniform(6, 11));


		Release(finishing_machine[finishing_machine_idx]);
		finishing_duration = Time - Entry - finishing_machine_wait_time;
	}

	// Internal methods
	void Stats_log() {
		cout << "------------------------------------------------------------" << '\n';
		cout << "Start time: " << start_time << '\n';
		cout << "............................................................" << '\n';
		cout << "PROCESS DURATIONS: " << '\n';
		cout << "............................................................" << '\n';
		cout << "Designing Duration: " << designing_duration / 60 << '\n';
		cout << "Approval Duration: "  << approval_duration  / 60 << '\n';
		cout << "Printing Duration: "  << printing_duration  / 60 << '\n';
		cout << "Stamping Duration: "  << stamping_duration  / 60 << '\n';
		cout << "Cutting Duration: "   << cutting_duration   / 60 << '\n';
		cout << "Finishing Duration: " << finishing_duration / 60 << '\n';
		cout << "............................................................" << '\n';
		cout << "WAIT TIMES: " << '\n';
		cout << "............................................................" << '\n';
		cout << "Designer Wait Time: "          << designer_wait_time          << '\n';
		cout << "Printer Wait Time: "           << printer_wait_time 		   << '\n';
		cout << "Supervisor Wait Time: "        << supervisor_wait_time        << '\n';
		cout << "Stamper Wait Time: "           << stamper_wait_time 		   << '\n';
		cout << "Plotter Wait Time: "           << plotter_wait_time 		   << '\n';
		cout << "Finishing_machine Wait Time: " << finishing_machine_wait_time << '\n';
	}

	int Seize_facility(Facility facility[], int facility_count) {		        // Seizes facility with the shortest queue
		int facility_idx = 0;
		for (size_t i = 1; i < facility_count; i++) {
			if (facility[i].QueueLen() < facility[facility_idx].QueueLen()) {
				facility_idx = i;
			}
		}

		Seize(facility[facility_idx]);
		return facility_idx;
	}

public:
	void Behavior() {
		if (Random() < 0.15) Designing();										// 85% of orders are from customers who have their own design or are returning
		Printing();
		if (Random() < 0.2) Stamping();											// Only 20% of orders go through stamping
		Cutting();
		Finishing();
	 // Stats_log();
		order_duration(Time - start_time);
	}
};

class Order_generator : public Event {
    void Behavior() {
        (new Order)->Activate();
        Activate(Time + Exponential(HOURS_PER_ORDER * 60));
    }
};

/* Material replacement */
class Material_replacement : public Process {
private:
	int idx;

public:
	Material_replacement(int idx) {
		this->idx = idx;
	}

	void Behavior() {
        Priority = 1;
		Seize(printer[idx]);
		Wait(10);																// Material replacement
		Release(printer[idx]);
    }
};

class Material_replacement_generator : public Event {
private:
	int idx;

public:
	Material_replacement_generator(int idx) {
		this->idx = idx;
	}

    void Behavior() {
        (new Material_replacement(idx))->Activate();
		Activate(Time + Exponential(5 * 60));
    }
};

/* Statistics functions */
void SetName_facility(Facility facility[], string facility_name, int facility_count) {
	for (size_t i = 0; i < facility_count; i++) {
		facility[i].SetName(facility_name + " " + to_string(i + 1));
	}
}

void Output_facility(Facility facility[], int facility_count) {
	for (size_t i = 0; i < facility_count; i++) {
		facility[i].Output();
	}
}

/* Main program */
int main() {
	SetOutput("output.dat");
	RandomSeed(time(nullptr));
	Init(0, DAY_COUNT * 24 * 60); 												// Days to minutes

	(new Order_generator)->Activate();
	for (size_t i = 0; i < PRINTER_COUNT; i++) {							    // Generates material replacement generator for every printer
		(new Material_replacement_generator(i))->Activate();
	}

	SetName_facility(printer, "Printer", PRINTER_COUNT);
	SetName_facility(stamper, "Stamper", STAMPER_COUNT);
	SetName_facility(plotter, "Plotter", PLOTTER_COUNT);
	SetName_facility(finishing_machine, "Finishing machine", FINISHING_MACHINES_COUNT);

	Run();

	order_duration.Output();
	designer.Output();
	supervisor.Output();
	Output_facility(printer, PRINTER_COUNT);
	Output_facility(stamper, STAMPER_COUNT);
	Output_facility(plotter, PLOTTER_COUNT);
	Output_facility(finishing_machine, FINISHING_MACHINES_COUNT);

	return 0;
}
