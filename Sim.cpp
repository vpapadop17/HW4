/* Sim.cpp */
// Author: Programmer Intern Jordan (with help from my teammates! :P)

#include <iostream> // cout and stuff
#include <algorithm> // 
#include <vector> // for vectors
#include <fstream> // to output to the HTML file easier
#include <string> // due
#include <unordered_map> // to track the time points when there are X num of cars present



#include "Sim_Math.h" // poisson, binomial, percentage, crash_occurrence
#include "TimeCode.h"
#include "TimeCode_Tests.h"
#include "BigInteger.h" // to count the total number of cars, which may be a really large value


// Hard-code some time durations for ease of use, uie L to avoid overflow
#define DURATION_10MIN 600
#define DURATION_1HR 3600
#define DURATION_1DAY 86400L
#define DURATION_1WK 604800L
#define DURATION_1MO 2629746L
#define DURATION_3MO 7889238L
#define DURATION_6MO 15778476L
#define DURATION_1YRS 31556952L
#define DURATION_5YRS 157784760L
#define DURATION_10YRS 315569520L


// A struct is like an object without any functions
// These structs are used to keep some of the important data together
struct data_point_pair{ // the data plotted on the chart
	TimeCode t; // x-axis
	int num_cars; // y-axis
};

struct results{ // All the "output" data from the simulation
	TimeCode total_duration;
	int road_capacity;
	int new_car_rate;
	int num_crashes;
	std::string total_cars;
	std::unordered_map<int, std::vector<TimeCode>> count_times_map;

	std::vector<int> x_data;
	std::vector<int> y_data;
	TimeCode plotted_duration;
};


// Ideally this would be a function of the TimeCode object itself
std::string TimeCodeVerboseString(TimeCode tc){
	std::string res = std::to_string(tc.GetHours()) + "hrs " + std::to_string(tc.GetMinutes()) + "mins " + std::to_string(tc.GetSeconds()) + "secs";
	return res;
}


// Function to generate the HTML file with embedded Plotly.js chart
void generateHTMLoutput(results& data, const std::string& filename = "plot.html") {

    std::ofstream html_file(filename);

    if (!html_file.is_open()) {
        std::cerr << "Error: Could not open " << filename << " for writing." << std::endl;
        return;
    }

    // Start of the HTML boilerplate
    html_file << "<!DOCTYPE html>\n";
    html_file << "<html>\n";
    html_file << "<head>\n";
    html_file << "    <title>Traffic Simulation</title>\n";
    // Load Plotly.js library from a CDN, requires internet access
    html_file << "    <script src=\"https://cdn.plot.ly/plotly-latest.min.js\"></script>\n";
    html_file << "    <style>\n";
    html_file << "        body { font-family: sans-serif; display: flex; flex-direction:column; justify-content: center; align-items: center; margin: 0; background-color: #f0f2f5; }\n";
	html_file << "        p {margin: 0;}\n";
    html_file << "    </style>\n";
    html_file << "</head>\n";
    html_file << "<body>\n";


	html_file << "<h1>Traffic Simulation</h1>\n";
	html_file << "<p>Duration: " << TimeCodeVerboseString(data.total_duration) << "</p>\n";
	html_file << "<p>Road Capacity: " << data.road_capacity << "</p>\n";
	html_file << "<p>Average New Cars / Sec: " << data.new_car_rate << "</p>\n";
	html_file << "<p>Number of Crashes: " << data.num_crashes << "</p>\n";
	html_file << "<p>Total # of Cars Simulated: " << data.total_cars << "</p>\n";

	html_file << "The number of moments (individual seconds) in which there were 0 cars: " << data.count_times_map[0].size() << "</p>\n";
	html_file << "The number of moments (individual seconds) in which there was exactly 1 car: " << data.count_times_map[1].size() << "</p>\n";
	html_file << "The number of moments (individual seconds) in which there were only 2 cars: " << data.count_times_map[2].size() << "</p>\n";
	int sum = 0;
	for(int i = 0; i <= 10; i++){
		sum = sum + data.count_times_map[i].size();
	}
	int per = percentage(sum, data.total_duration.GetTimeCodeAsSeconds());
	html_file << "The number of moments (individual seconds) in which there were 10 cars or fewer: " << sum << " (about "  <<  per << "% of the entire simulation time.)" << "</p>\n";


	html_file << "<h4>Below is a plot of the number of cars actively traveling between 2hr 0min and 2hr 10min.</h4>\n";

    // This is the div where Plotly will render the chart
    html_file << "    <div id=\"TraffSimPlotDiv\"></div>\n";

    // Start of the JavaScript block
    html_file << "    <script>\n";

    // Convert C++ vectors to JavaScript arrays
    html_file << "        var xData = [";
    for (size_t i = 0; i < data.x_data.size(); ++i) {
        html_file << data.x_data[i];
        if (i < data.x_data.size() - 1) {
            html_file << ", ";
        }
    }
    html_file << "];\n";

    html_file << "        var yData = [";
    for (size_t i = 0; i < data.y_data.size(); ++i) {
        html_file << data.y_data[i];
        if (i < data.y_data.size() - 1) {
            html_file << ", ";
        }
    }
    html_file << "];\n";

    // Define the data trace for Plotly
    html_file << "        var trace1 = {\n";
    html_file << "            x: xData,\n";
    html_file << "            y: yData,\n";
    html_file << "            mode: 'lines',\n"; // Can be 'lines', 'markers', or 'lines+markers'
    html_file << "            type: 'scattergl',\n";
    html_file << "            marker: { size: 8, color: 'blue' }\n";
    html_file << "        };\n";

    // Define the layout for the plot
    html_file << "        var layout = {\n";
    html_file << "            title: 'Cars Traveling Over Time',\n";
    html_file << "            xaxis: { title: 'Time (seconds)' },\n";
    html_file << "            yaxis: { title: 'Number of Cars' },\n";
    html_file << "            hovermode: 'closest'\n"; // Enable tooltips on hover
    html_file << "        };\n";

    // Combine data and layout
    html_file << "        var data = [trace1];\n";

    // Render the plot 
    html_file << "        Plotly.newPlot('TraffSimPlotDiv', data, layout, {responsive: true});\n";

    // Generate a PNG image of the plot for embedding or download
    //html_file << "        Plotly.toImage('TraffSimPlotDiv', {format: 'png'}).then(function(dataUrl) {\n";
    //html_file << "            var img = new Image();\n";
    //html_file << "            img.src = dataUrl;\n";
    //html_file << "            document.body.appendChild(img);\n";
    //html_file << "        });\n";

    html_file << "    </script>\n";
    // End of the HTML boilerplate
    html_file << "</body>\n";
    html_file << "</html>\n";

    html_file.close();
    std::cout << "HTML file generated successfully: " << filename << std::endl;
}


//changed the results to be a passed by refrence and did the same for the generate HTMLoutput function instead of results being a value.
// Passing by value made a copy of the results every time this function was called. With & the function
// uses the original, so nothing is copied.
void generateTerminalOutput(results& data) {
	std::cout << "\tduration: " << TimeCodeVerboseString(data.total_duration)
		<< "\n\troad capacity: " << data.road_capacity
		<< "\n\tavg new cars / second: " << data.new_car_rate
		<< "\n\tnumber of crashes: " << data.num_crashes
		<< "\n\ttotal number of cars simulated: " << data.total_cars << std::endl;


	std::cout << "\tThe number of moments (individual seconds) in which there were 0 cars: " << data.count_times_map[0].size() << std::endl;
	std::cout << "\tThe number of moments (individual seconds) in which there was exactly 1 car: " << data.count_times_map[1].size() << std::endl;
	std::cout << "\tThe number of moments (individual seconds) in which there were only 2 cars: " << data.count_times_map[2].size() << std::endl;
	int sum = 0;
	for(int i = 0; i <= 10; i++){
		sum = sum + data.count_times_map[i].size();
	}
	int per = percentage(sum, data.total_duration.GetTimeCodeAsSeconds());;
	std::cout << "\tThe number of moments (individual seconds) in which there were 10 cars or fewer: " << sum << " (about "  <<  per << "% of the entire simulation time.)" << std::endl;
}




bool is_int(std::string a){
	for(size_t i = 0; i < a.size(); i++){
		if(!isdigit(a.at(i))){
			return false;
		}
	}
	return true;
}

unsigned long long int parse_args(int argc, char* argv[]){
	unsigned long long int duration = 0;
	if(argc != 2){
		std::cout << "Usage: traffic-simulation <duration_in_seconds>" << std::endl;
		std::exit(1);
	}

	std::string duration_str = argv[1];
	if(is_int(duration_str)){
		duration = std::stoull(duration_str); // stoull = string to unsigned long long (int)
		if(duration <= 0){
			std::cout << "Error: duration must be positive." << std::endl;
			std::exit(1);
		}
		
	} else {
		std::cout << "Error: duration must be an integer." << std::endl;
		std::exit(1);
	}

	return duration;
}


int main(int argc, char* argv[]) {
	const unsigned long long int DURATION = parse_args(argc, argv);

	std::cout << "--- Traffic Simulator ---" << std::endl;
	TimeCode dur = TimeCode(0, 0, DURATION);

	const int CAP = 200; // maximum capacity of the road
	const double new_car_rate = 4; // average new cars per second
	const double p_car_exits = 0.298; // probability a car departs when road is empty

	std::cout << "Computing crash probabilities from database..." << std::endl;
	// Build the crash probability table
	const std::unordered_map<std::string, double> crash_prob_map = build_crash_prob_map();
	for(const auto& pair : crash_prob_map){
		std::cout << "Day: " << pair.first << ", probability of a crash: " << pair.second << std::endl;
	}



	std::cout << "Simulating traffic..." << std::endl;
	BigInteger total_num_cars = BigInteger("0"); // number of cars that traveled, use BigInteger in case of integer overflow
	std::vector<data_point_pair> data; // to store the data points
	int num_cars = 0; // number of cars (intially 0)
	int crash_count = 0;
	int crash_timer = 0; // cool-off period of a crash
	for(TimeCode t = TimeCode(); t < dur; t = t + TimeCode(0, 0, 1)){
		int progress = percentage(t.GetTimeCodeAsSeconds(), dur.GetTimeCodeAsSeconds());
		std::cout << "\r" << progress << "%" << std::flush;

		//ALL();
		//i do not think i need all as all runs all the timecode tests and its on the for loop here so i dont need to run the tests every second also it does not change anything on this code (it doesnt change number of cars etc)

		// --- New Cars Show Up (maybe) ---
		int num_new_cars = poisson(new_car_rate);
		if(num_cars >= CAP){
			num_new_cars = 0; // road is full, no new cars
		}
		total_num_cars = total_num_cars + BigInteger(std::to_string(num_new_cars));

		// --- Existing Cars Might Leave ---
		// Strong assumptions of model here!
		// if the road is full, the cars cannot move and therefore cannot exit
		// As the road becomes more congested, less cars can exit so we decrease the probability of an exit
		// Overall the chances are something like 30% chance of exit for each car present
		// So about 1/3 of cars leave typically.
		double cur_p_car_exits = p_car_exits * (1.0 - double(num_cars) / CAP); // a simple linear approximation
		int num_cars_exiting = binomial(num_cars, cur_p_car_exits);
		// no cars exit if there was a crash recently
		if(crash_timer > 0){
			num_cars_exiting = 0;
		}
		crash_timer--;

		// check for a crash once / day
		
		//std::cout << "prob of crash today: " << prob_of_crash_today << std::endl;
		if(t.GetTimeCodeAsSeconds() % DURATION_1DAY == 0){
			//what i changed: I moved the line that looks up today's crash probability inside the if statement that is for only once a day. It used to be above the if, so the program
			// worked out the day of the week, the day name and searched the map every second, even though the probability is only used once per day
			double prob_of_crash_today = crash_prob_map.at(day_of_week_name(day_of_week_code(t)));
			bool crash = crash_occurs(prob_of_crash_today);
			if(crash){
				///std::cout << "CRASH! No cars exit for a while now" << std::endl;
				crash_count++;
				crash_timer += DURATION_10MIN;
			}
		}

		// --- Compute Number of Cars Now ---
		int cand_num_cars = num_cars + num_new_cars - num_cars_exiting;
		if(cand_num_cars < 0){
			num_cars = 0; // road is empty
		} else {
			num_cars = cand_num_cars;
		}

		// --- Track Data for Plotting Later ---
		data.push_back({t, num_cars});
		//cout << "Time: " << t.ToString() << "s   Num Cars: " << num_cars << endl;
	}
	std::cout << std::endl;


	// Find the times at which certain amount of cars are present
	std::cout << "Computing sample statistics..." << std::endl;
	std::unordered_map<int, std::vector<TimeCode>> count_times;
	//added this as it means that nothing gets printed yet 
	int last_progress = -1; //holds teh last precentage printed on the screen its one of teh things i changed to make sure precentages do not get printed again and again 
	for(size_t i = 0; i < data.size(); i++){
		//what i changed: this percentage only changes 100 times but it runs every second and thats a lot of looping and apparently flush is very slow as it puts teh number on the string at that specific moment so when we get two numbers who are teh samae like 1% it keeps printing 1%
		
		int progress = percentage(i, data.size());
		if(progress != last_progress){                  //print only if its a new number we dont want 1% to be printed all the time
			std::cout << "\r" << progress << "%" << std::flush;   
			last_progress = progress;                   //remember the number
		}
		
		data_point_pair cur = data[i];
		//if(count_times.find(cur.num_cars) != count_times.end()){
			//std::vector<TimeCode> times_list = count_times[cur.num_cars];
			//times_list.push_back(cur.t);
			//count_times[cur.num_cars] = times_list;
		//} else {
			//count_times[cur.num_cars] = std::vector<TimeCode>{cur.t};
		//}
		//instead of copying every single list from the map and add one item adn copy again, I instead add the item directly in the list in the map 
		//find is not needed as the line creates a vector if one is not existing 
		count_times[cur.num_cars].push_back(cur.t);
	}
	std::cout << "\n---Simulation Finished---" << std::endl;



	results res;

	res.total_duration = dur;
	res.road_capacity = CAP;
	res.new_car_rate = new_car_rate;
	res.num_crashes = crash_count;
	res.total_cars = total_num_cars.ToString();
	res.count_times_map = count_times;

	std::vector<int> x_data;
	std::vector<int> y_data;
	size_t start = 7200;
	for(size_t i = 7200; i < start + DURATION_10MIN; i++){
		x_data.push_back(data[i].t.GetTimeCodeAsSeconds());
		y_data.push_back(data[i].num_cars);
	}
	res.x_data = x_data;
	res.y_data = y_data;
	res.plotted_duration = TimeCode(0, 0, DURATION_10MIN);
	//res.plotted_duration.WasteTimeAndBeSlow();
	//i do not need this as it only changes teh local variable num but that variable is never used so its literally just wasting time. 


	generateTerminalOutput(res);
	generateHTMLoutput(res);


	return 0;
}
