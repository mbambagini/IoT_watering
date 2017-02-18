#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iomanip>
#include <sqlite3.h>
#include <stdlib.h>

using namespace std;

struct t_node {
    int cycles;
    int seconds;
    int n_fails;
    int n_total_sensing;
    int low_level_water;
    t_node() {
        cycles = 0;
        seconds = 0;
        n_fails = 0;
        n_total_sensing = 0;
        low_level_water = 0;
    }
};

map<int, t_node> nodes;

int time_offset = 0;
int duration = 0;

const string getDateTime(time_t t);
void print_script(ofstream& f, const char* filename, const char* parameter,
                                                const char* file_data, int id);
bool read_configuration(const char* filename, int &duration);
bool read_offset (char* filename, int &time_offset);
int callback(void *f, int argc, char **argv, char **azColName);

int callback_low_water_level(void *f, int argc, char **argv, char **azColName);

int main(int argc, char* argv[]) {
	// check inputs
    if (argc != 7) {
        cerr<<"Usage: "<<argv[0]<<" [db file] [config file] [time start]"
            <<" [output data] [output script] [mail file]"<<endl;
        return 1;
    }

	// open files
    if (!read_offset (argv[3], time_offset)) {
        cerr<<"Impossible to read the time offset"<<endl;
        return 1;
    }
    if (!read_configuration(argv[2], duration) || duration == 0) {
        cerr<<"Impossible to read the configuration file"<<endl;
        return 1;
    }
    sqlite3 *db;
    if (sqlite3_open(argv[1], &db)) {
        cerr<<"Can't open database"<<endl;
        sqlite3_close(db);
        return 1;
    }
    ofstream sensor_file;
    sensor_file.open(argv[5]);
    if (!sensor_file.is_open()) {
        cerr<<"Impossible to open the output data file"<<endl;
        return 1;
    }
    ofstream script_file(argv[4]);
    if (!script_file.is_open())  {
        cerr<<"Impossible to open the output script file"<<endl;
        return 1;
    }
    ofstream main_text(argv[6]);
    if (!main_text.is_open())  {
        cerr<<"Impossible to open the mail text file"<<endl;
        return 1;
    }

	// run query
    stringstream str;
    str<<"SELECT SENSTIME, ADDRESS, SENSSTATE, HUMD, TEMP, MOIS "
       <<"FROM SENSING "
       <<"WHERE SENSTIME >= "<<time_offset;
    string sql(str.str());
    if (sqlite3_exec(db, sql.c_str(), callback, (void*)&sensor_file,
                                                          NULL) != SQLITE_OK) {
        cerr<<"Cannot retrieve sensor data"<<endl;
        sensor_file.close();
        sqlite3_close(db);
        return 0;
    }
    sensor_file.close();

    stringstream str2;
    str2<<"SELECT WATERTIME, ADDRESS, WATERSTATE "
        <<"FROM WATERING "
        <<"WHERE WATERTIME >= "<<time_offset;
    string sql2(str2.str());
    if (sqlite3_exec(db, sql2.c_str(), callback_low_water_level, NULL,
                                                          NULL) != SQLITE_OK) {
        cerr<<"Cannot retrieve watering data"<<endl;
        sensor_file.close();
        sqlite3_close(db);
        return 0;
    }

    sqlite3_close(db);

    if (nodes.size() == 0) {
        script_file.close();
        return 2;
    }

	// generate gnuplot script
    print_script(script_file, "humi", "Humidity [percentage]", argv[5], 3);
    print_script(script_file, "temp", "Temperature [Celsius]", argv[5], 4);
    print_script(script_file, "mois", "Moisture [percentage]", argv[5], 5);
    script_file.close();

	// generate email message
    main_text<<"Update watering system"<<endl<<endl;
    main_text<<"Last update : "<<getDateTime(time_offset*60)<<endl;
    main_text<<"Current time: "<<getDateTime(time(0))<<endl<<endl;
    main_text<<"Sensing fail report"<<endl;
    main_text<<"Unit    Fails    Total    Ratio"<<endl;
    for (map<int, t_node>::const_iterator it = nodes.begin(); it != nodes.end();
                                                                         it++) {
        main_text<<setw(4)<<it->first<<"    ";
        main_text<<setw(5)<<it->second.n_fails<<"    ";
        main_text<<setw(5)<<it->second.n_total_sensing<<"    ";
        double ratio = (double)it->second.n_fails/it->second.n_total_sensing;
        main_text<<fixed<<setprecision(2)<<ratio*100.0<<"%"<<endl;
    }

    main_text<<endl<<"Low-water report"<<endl;
    for (map<int, t_node>::const_iterator it = nodes.begin(); it != nodes.end();
                                                                         it++) {
        main_text<<"Watering unit "<<it->first<<": ";
        if (it->second.low_level_water)
            main_text<<"LOW LEVEL"<<endl;
        else
            main_text<<"OK"<<endl;
    }

    main_text.close();

    return 0;
}

int callback_low_water_level(void *f, int argc, char **argv, char **azColName) {
    int addr = atoi(argv[1]);
    if (atoi(argv[2]) == 2)
        nodes[addr].low_level_water = 1;
    return 0;
}

const string getDateTime(time_t t) {
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&t);
    strftime(buf, sizeof(buf), "%d/%m/%Y %X", &tstruct);
    return buf;
}

void print_script(ofstream& f, const char* filename, const char* parameter,
                                               const char* file_data, int id) {
    f<<"set terminal postscript eps enhanced  size 6,3.5 color"<<endl;
    f<<"set output \""<<filename<<".eps\""<<endl;
    f<<"set grid"<<endl;
    f<<"set xlabel \"Time slot\" font \"Times-Roman, 34\" offset -0,-2";
    f<<endl;
    f<<"set ylabel \""<<parameter<<"\" font \"Times-Roman, 34\" offset -1, -1";
    f<<endl;
    f<<"set key inside top left"<<endl;
    f<<"set key spacing 3.5"<<endl<<endl;
    f<<"plot \\"<<endl;

    for (map<int, t_node>::const_iterator it = nodes.begin(); it != nodes.end();
                                                                         it++) {
        f<<"\t\""<<file_data<<"\" using 1:($2=="<<it->first;
        f<<"?$"<<id<<":1/0) with lines lw 6 ";
        f<<"title \"Watering unit #"<<it->first;
        f<<"\" \\"<<endl;
    }

    f<<endl;
}

bool read_configuration(const char* filename, int &duration) {
    int state = 0;
    int n_units = 0;
    
    nodes.clear();

    // read configuration file
    FILE *fp = fopen(filename, "r");
    if(fp == NULL)
        return false;
    char line[250];
    
    while(fgets(line, sizeof(line), fp)) {
        if (line[0] == '#')
            continue;
        switch(state) {
            case 0: //read interval length
                sscanf(line, "%d\r", &duration);
                state = 1;
                break;
            case 1: //read number of watering units
                sscanf(line, "%d\r", &n_units);
                state = 2;
                break;
            case 2: //read number of watering units
                t_node node;
                int addr;
                sscanf(line, "%d %d %d\r", &addr, &node.cycles, &node.seconds);
                nodes[addr] = node;
                n_units--;
                if (n_units == 0)
                    state = 3;
                break;
        }
    }
    fclose(fp);

    return true;
}

bool read_offset (char* filename, int &time_offset) {
    ifstream fp;
    fp.open(filename);
    if (!fp.is_open())
        return false;
    fp>>time_offset;
    fp.close();

    return true;
}

int callback(void *f, int argc, char **argv, char **azColName) {
    ofstream* file = (ofstream*)f;

    // common between watering and sensing: time address response
    int valid = atoi(argv[2]) == 0 ? 1 : 0;
    int addr = atoi(argv[1]);
    if (valid) {
        int sample_id = (atoi(argv[0])-time_offset)/duration;
        (*file)<<sample_id<<" "<<addr<<" "<<argv[3]<<" ";
        (*file)<<argv[4]<<" "<<argv[5]<<endl;
    } else {
        nodes[addr].n_fails++;
    }
    nodes[addr].n_total_sensing++;

    return 0;
}

