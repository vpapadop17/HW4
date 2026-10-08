

// author: Programmer Intern Jordan

#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <sqlite3.h>

#include "Sim_Math.h"


double power_recursive(double a, int b){
	if(b == 1){
		return a;
	}
	double tmp = a * power_recursive(a, b-1);
	return tmp;
}

double power(double a, int b){
	if(b == 0){
		return 1;
	}

	if(a == 0){
		return 0;
	}

	// If b is negative, we flip the sign now and then later
	// we return 1.0 divided by the "normally" computed answer for a^b
	bool needs_inversion = false;
	if(b < 0){
		needs_inversion = true;
		b = -b;
	}
	
	//double ans = power_recursive(a, b);

	//if(needs_inversion){
		//ans = 1.0 / ans;
	//}

	//removed the recursion because it calls the function multiple times and makes the program slow 
	//multiply a by itself b times so the power is done liek this and works a bit faster not a lot though 
	double ans = 1.0;
    for(int i = 0; i < b; i++){
        ans = ans * a;  
    }

    if(needs_inversion) ans = 1.0 / ans;
    return ans;

}


// https://www.geeksforgeeks.org/dsa/fast-exponention-using-bit-manipulation/



int poisson(int lambda){ // lambda is the technical term, this value is the desired average
	// Knuth's algorithm for Poisson random numbers
	double L = power(e, -lambda);
	int k = 0;
	double p = 1.0;

	while(p > L){
		k = k + 1;
		double u = (double)rand() / RAND_MAX; // gives a uniformly random number between 0 and 1
		p *= u;
	}

	return k-1;
}


int binomial(int n, double p){
	// Binomial distribution, the number of successful trials in n Bernoulli events
	// Bernoulli events are simple:
		// each event is either a success or failure
		// all events are independent
		// all events have the same probability of success (p)
	// n gives the number of trials, in this case the number of cars
	// p gives the probability of success, in this case the probability an individual car exits

	int count = 0;
	for (int i = 0; i < n; i++){ // for each car
		double u = (double)rand() / RAND_MAX; // generate a uniform random number between 0 and 1
		if(u < p){ // if that random number is less than the probability of success, the car leaves
			count = count + 1; // count the cars that leave
		}
	}
	
	return count; // return the number of cars that left
}

int percentage(int portion, int overall){
	// returns portion / overall as a percentage
	// for example, if portion = 16 and overall = 82
	// then this function returns 19% = .019 = 16/82
	double per = (portion / static_cast<double>(overall)) * 100;
	return static_cast<int>(per);
}


bool crash_occurs(double prob_of_crash_today){
	if( ((double)rand() / RAND_MAX) < prob_of_crash_today ){
		return true;
	}
	return false;
}


int day_of_week_code(TimeCode cur_time){
	int cur_hour = cur_time.GetHours();
	cur_hour = cur_hour % 168;
	if(cur_hour <= 24){
		return 0; // Sunday
	} else if(cur_hour <= 48){
		return 1; // Monday
	} else if(cur_hour <= 72){
		return 2; // Tuesday
	} else if(cur_hour <= 96){
		return 3; // Wednesday
	} else if(cur_hour <= 120){
		return 4; // Thursday
	} else if(cur_hour <= 144){
		return 5; // Friday
	} else {
		return 6; // Saturday
	}
}

std::string day_of_week_name(int code){
	switch(code){
		case 0: return "Sunday";
		case 1: return "Monday";
		case 2: return "Tuesday";
		case 3: return "Wednesday";
		case 4: return "Thursday";
		case 5: return "Friday";
		case 6: return "Saturday";
		default: 
			throw std::invalid_argument("Invalid day of week code: " + std::to_string(code));
	}
}


int prepare(sqlite3* db, const char* query, sqlite3_stmt** stmt){
	int rc = sqlite3_prepare_v2(db, query, -1, stmt, 0); // (db, query, ?, response/answer, ?)
	if(rc != SQLITE_OK){
		sqlite3_close(db);
		throw std::runtime_error("Could not prepare database query.");
	}
	return rc;
}


// insanely slow BUT IDK why?!
std::unordered_map<std::string, double> build_crash_prob_map(){
	// Create a hashmap like
	// {Fri: 0.50448,
	// Wed: 0.324194,
	// Thu: 0.410394,
	// Tue: 0.239068,
	// Mon: 0.147312,
	// Sat: 0.59319,
	// Sun: 0.0645161}
	// The key is a day of the week (actually a number like 2 for Tuesday)
	// The value is the probability of a crash on that day.  Based on data from the database
	// the probability of a crash on e.g., Tuesday is  (num crashes on tuesdays) / (total num crashes)

	sqlite3* db;
	int rc = sqlite3_open("CRASH_LANCASTER_2024.db", &db);
	if(rc){
		throw std::invalid_argument("Could not open CRASH_LANCASTER_2024.db database file!");
	}

	// Step 1. Count all stuff (row things?) from DB
	const char* first_sql_query = "SELECT COUNT(*) FROM incidents;";  
	double total_crashes = 0.0;
	sqlite3_stmt* stmt = nullptr;
	rc = prepare(db, first_sql_query, &stmt);
	if(rc == SQLITE_OK){
		rc = sqlite3_step(stmt);
		if(rc == SQLITE_ROW){
			total_crashes = static_cast<double>(sqlite3_column_int(stmt, 0));
			//std::cout << "total_crashes: " << total_crashes << std::endl;
		}
	}
	sqlite3_finalize(stmt);

	// Step 2 & 3: Count crashes per day of week in ONE query
	std::unordered_map<std::string, double> crash_prob_map;
	
	//the original looped over every day of the week and inside that over every crash and did a separate query for each one which were a lot of queries.
	//GROUP BY counts the crashes for each day, so the work is done in a single query instead of thousands.
	//Count gives the size of the piles (each of them)

	const char* group_sql_query = "SELECT DAY_OF_WEEK, COUNT(*) FROM incidents WHERE AUTOMOBILE_COUNT != 0 GROUP BY DAY_OF_WEEK;";
	rc = prepare(db, group_sql_query, &stmt);

	//this loops through every day 
	while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
		int dow = sqlite3_column_int(stmt, 0);       // Column 0: DAY_OF_WEEK (1 to 7)
		int count = sqlite3_column_int(stmt, 1);     // Column 1: total crashes for that day

		//indexed as 0-6 (the days) 
		if(dow >= 1 && dow <= 7 && total_crashes > 0){
			crash_prob_map[day_of_week_name(dow - 1)] = static_cast<double>(count) / total_crashes;
		}
	}

	return crash_prob_map;
}
