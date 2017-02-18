#include <iostream>
#include <fstream>
#include <list>
#include <math.h>

using namespace std;

struct node_t {
	int addr;
	int watering_wait_cycles;
	int watering_duration;
};

struct configuration_t {
	int interval_minutes;
	list<node_t> nodes;
};

void read_cfg(configuration_t& cfg);
void serialize_cfg(const configuration_t& cfg, const char* filename);

int main(int argc, char* argv[]) {
	// check inputs
	if (argc < 2) {
		cerr<<"usage: "<<argv[0]<<" [filename]"<<endl;
		return 1;
	}

	configuration_t cfg;

	// fill inputs
	read_cfg(cfg);

	// save file
	serialize_cfg(cfg, argv[1]);

	return 0;
}


void read_cfg(configuration_t& cfg) {
	cout<<"How many minutes does an interval last? ";
	cin>>cfg.interval_minutes;
	while(1) {
		char c;
		// ask for additional watering unit
		do {
			cout<<"Add a watering unit? [y/n] ";
			cin>>c;
		} while(c != 'y' && c != 'n');
		if (c == 'n')
			break;
		node_t node;
		bool valid;
		do {
			valid = true;
			// ask for the network identifier
			cout<<"Node address: ";
			cin>>node.addr;
			for (list<node_t>::iterator it = cfg.nodes.begin();
												   it != cfg.nodes.end(); it++)
				if (it->addr == node.addr) {
					cout<<"Address already assigned"<<endl;
					valid = false;
					break;
				}
		} while(!valid);
		double water_payload;
        // ask for the pump payload, tank capacity and water to provide
		// if the water to provide is a single shot is more than the
		// available tank, split the watering in two (or more) iterations
		// instead of doing everything in a single shot, ending up with
		// water shortage
		cout<<"Pump payload (litre per hour - l/h): ";
		cin>>water_payload;
		double tank_capacity;
		cout<<"Water tank capacity (litre - l): ";
		cin>>tank_capacity;
		double water;
		cout<<"Water to provide per day (litre per day): ";
		cin>>water;
		int seconds = water / (water_payload / 3600.0);
		int periods = (24 * 60) / cfg.interval_minutes;
		if (water > tank_capacity) {
			int factor = ceil(water / tank_capacity);
			seconds = seconds / (factor + 1);
			periods = periods / (factor + 1);
		}
		node.watering_wait_cycles = periods;
		node.watering_duration = seconds;

		do {
			cout<<"Confirm data? [y/n] ";
			cin>>c;
		} while(c != 'y' && c != 'n');
		if (c == 'y') {
			cfg.nodes.push_back(node);
			if (cfg.nodes.size() == 10)
				cout<<"Inserted the maximum number of watering units"<<endl;
		}
    }
}

void serialize_cfg(const configuration_t& cfg, const char* filename) {
	ofstream file;
	file.open(filename);
	file<<"# how long an interval last (minutes)"<<endl;
	file<<cfg.interval_minutes<<endl;
	file<<"# number of watering units"<<endl;
	file<<cfg.nodes.size()<<endl;
	file<<"# configuration of the watering units"<<endl;
	for (list<node_t>::const_iterator it = cfg.nodes.begin();
				 						it != cfg.nodes.end(); it++) {
		file<<it->addr<<" "
			<<it->watering_wait_cycles<<" "
			<<it->watering_duration<<endl;
	}
	file<<endl;
	file.close();
}

