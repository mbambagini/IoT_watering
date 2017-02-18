#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <sqlite3.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <unistd.h>

using namespace std;

// return true if the tables are already present in the database
bool check_if_tables_exist(sqlite3 *db);

// read supervisor configuration (sampling interval and watering units
// description)
bool read_configuration(char* filename, int &time_interval);

// read time stamp (in minutes) of the last update
bool read_offset(char* filename, int &time_offset);

// read the API KEY to upload data on the cloud
bool read_cloud_configuration(char* filename, string& key);

// send tuple on the cloud
void update_data (int node, double humi, double temp, double mois, int valid,
                                                        int cycle, string key);

// create string from timestamp - in the format expected by the cloud
const string getDateTime(time_t t);


int main(int argc, char* argv[])
{
	// check parameters
    if (argc != 7) {
        cerr<<"Usage: "<<argv[0]<<" [db file] [sensor file] "
            <<"[water file] "<<"[config file] [time offset] "
            <<"[cloud config]"<<endl;
        return 1;
    }

	// read files
    int interval_duration = 0;
    if (!read_configuration(argv[4], interval_duration)) {
        cerr<<"Cannot open configuration file"<<endl;
        return 1;
    }
    int time_offset = 0;
    if (!read_offset(argv[5], time_offset)) {
        cerr<<"Cannot open timestamp file"<<endl;
        return 1;
    }
    string key;
    if (!read_cloud_configuration(argv[6], key)) {
        cerr<<"Cannot open cloud config file"<<endl;
        return 1;
    }
    sqlite3 *db;
    if (sqlite3_open(argv[1], &db)) {
        cerr<<"Cannot open database"<<endl;
        sqlite3_close(db);
        return 1;
    }
    if (!check_if_tables_exist(db)) {
        cerr<<"Impossible to create tables"<<endl;
        sqlite3_close(db);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    ifstream sensor_file;
    sensor_file.open(argv[2]);
    if (!sensor_file.is_open()) {
        cerr<<"Impossible to open the sensor file"<<endl;
        return 1;
    }

	// for each line in the sensor file, save the information in the database
    // and on the cloud
    int sens_errors = 0;
    int sens_total = 0;
    string line;
    while (getline(sensor_file, line)) {
        istringstream iss(line);
        int cycle, address, state;
        double hum, temp, mois;
        iss>>cycle>>address>>state>>hum>>temp>>mois;
        cycle = cycle * interval_duration + time_offset;

        // to the database
        stringstream oss;
		oss<<"INSERT INTO SENSING " \
             "(SENSTIME, ADDRESS, SENSSTATE, HUMD, TEMP, MOIS) " \
             "VALUES ("<<cycle<<", "<<address<<", "<<state<<", " \
                       <<hum<<", "<<temp<<", "<<mois<<");";
		if (sqlite3_exec(db, oss.str().c_str(), NULL, NULL, NULL) != SQLITE_OK)
			sens_errors++;

		// to the cloud
        update_data(address, hum, temp, mois, state==0, cycle, key);
        sens_total++;
    }
    sensor_file.close();

    ifstream water_file;
    water_file.open(argv[3]);
    int water_errors = 0;
    int water_total = 0;
    if (!water_file.is_open()) {
        cerr<<"Impossible to open the watering file"<<endl;
        return 1;
    }
	//for each line in the watering file, save it in the database
    while (getline(water_file, line)) {
        istringstream iss(line);
        int cycle, address, state, seconds;
        iss>>cycle>>address>>state>>seconds;
        stringstream oss;
        cycle = cycle * interval_duration + time_offset;
        oss<<"INSERT INTO WATERING " \
             "(WATERTIME, ADDRESS, WATERSTATE, SECONDS) " \
             "VALUES ("<<cycle<<", "<<address<<", "<<state<<", " \
                       <<seconds<<");";
        if (sqlite3_exec(db, oss.str().c_str(), NULL, NULL, NULL) != SQLITE_OK)
            water_errors++;
        water_total++;
    }
    water_file.close();

	// in case of database errors, print out the error rate
    if (water_errors > 0)
        cerr<<"Watering errors: "<<water_errors
            <<" out of "<<water_total<<endl;
    if (sens_errors > 0)
        cerr<<"Sensor errors: "<<sens_errors
            <<" out of "<<sens_total<<endl;

	sqlite3_close(db);
	curl_global_cleanup();
    return 0;
}

bool check_if_tables_exist(sqlite3 *db) {
	// try to run a simple query, if it fails, it means that the tables are
	// not present
    string sql("SELECT * FROM WATERING");
    if (sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL) == SQLITE_OK)
        return true;

	// tables not present, let's create it
    sql = "CREATE TABLE SENSING ("  \
          "ID         INTEGER PRIMARY KEY AUTOINCREMENT," \
          "SENSTIME    INT NOT NULL," \
          "ADDRESS    INT NOT NULL," \
          "SENSSTATE    INT NOT NULL," \
          "HUMD        REAL," \
          "TEMP        REAL," \
          "MOIS        REAL);";
    if (sqlite3_exec(db, sql.c_str(), NULL, 0, NULL) != SQLITE_OK)
        return false;

    sql = "CREATE TABLE WATERING ("  \
          "ID         INTEGER PRIMARY KEY AUTOINCREMENT," \
          "WATERTIME    INT NOT NULL," \
          "ADDRESS    INT NOT NULL," \
          "WATERSTATE    INT NOT NULL," \
          "SECONDS    INT);";
    if (sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL) != SQLITE_OK)
        return false;

    return true;
}

bool read_configuration (char* filename, int &time_interval) {
    // read configuration file
    FILE *fp = fopen(filename, "r");
    if(fp == NULL)
        return false;
    char line[250];
    
    while(fgets(line, sizeof(line), fp)) {
        if (line[0] == '#')
            continue;
        // only the first line is needed
        sscanf(line, "%d\r", &time_interval);
        return true;
    }
    fclose(fp);
    
    return false;
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

void update_data (int node, double humi, double temp, double mois, int valid,
                                                       int cycle, string key) {
    stringstream s;

    CURL *curl = curl_easy_init();
    if(curl) {
        s<<"http://api.thingspeak.com/update.json?";
        s<<"api_key="<<key;
        s<<"&field1="<<node;
        s<<"&field2="<<humi;
        s<<"&field3="<<temp;
        s<<"&field4="<<mois;
        s<<"&field5="<<valid;
        s<<"&created_at='"<<getDateTime(cycle*60)<<"'";
        curl_easy_setopt(curl, CURLOPT_URL, s.str().c_str());
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_perform(curl);
        usleep(100000);
        curl_easy_cleanup(curl);
    }
}

const string getDateTime(time_t t) {
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}

bool read_cloud_configuration(char* filename, string& key) {
    FILE *fp = fopen(filename, "r");
    if(fp == NULL)
        return false;
    char line[250];

    while(fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '#')
            continue;
        // only the first line is needed
		key = string(line);
		// remove latest character as it is newline
		key = key.substr(0, key.length()-1);
        fclose(fp);
        return true;
    }
    fclose(fp);
    
    return false;
}

