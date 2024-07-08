// volunteer file first lines:
// 2024-08-14T09:00:00.000000+02:00
//  id,name, 9h - 10h,10h - 11h,11h - 12h,12h - 13h,13h - 14h,14h - 15h,15h - 16h,16h - 17h,17h - 18h,18h - 19h,19h - 20h,20h - 21h,21h - 22h,22h - 23h,23h - 00h,00h - 01h
//  24ab5f55-77db-4fc6-8ac0-1d1aaf5fbe06,Vincent,1,2,2,1,1,1,1,1,1,1,1,1,1,3,3,1

// quest file first lines:
//  id,name, Needed,Start,End
//  ef12342f-d252-4a4c-9829-3fcf1f8686ba,"Chapeau: Cie Déo",3,2024-08-14T23:50:00.000000+02:00,2024-08-15T00:05:00.000000+02:00

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <regex>

using namespace std;

struct Volunteer
{
    int num;
    string name;
    vector<int> hours;
    vector<int> availableQuests;
    vector<int> inconveniences;
};

struct Quest
{
    int num;
    string name;
    int needed;
    // use time_t instead of string for start and end
    time_t start;
    time_t end;
    int duration; // in minutes
    vector<int> incompatibleQuests;
};

map<string, bool> parseModelBool(const string &output)
{
    map<string, bool> model;

    //  (define-fun conc2 () Real
    //(/ 3.0 4.0))
    // should return conc2 0.75

    // Regex pattern to match variable assignments
    // regex pattern(R"(define-fun ([a-zA-Z0-9_]+) \(\) Real\n\s+([0-9.]+))");
    // search for concentrations only
    regex pattern(R"(define-fun ([a-z0-9_]+) \(\) Bool\n\s+([^\n]+)\)\n)");

    // Iterate over matches
    sregex_iterator it(output.begin(), output.end(), pattern);
    sregex_iterator end;
    for (; it != end; ++it)
    {
        smatch match = *it;
        string varName = match.str(1);
        // cout << "varName: " << varName << " is " << match.str(2) << "\n";
        bool varValue = (match.str(2) == "true");
        // cout << varName << " " << varValue << " from " << match.str(2) << endl;
        model[varName] = varValue;
    }

    return model;
}

int main(int argc, char **argv)
{
    int maxdur=400; //max duration for each volunteer
    if(argc>1){
        maxdur=stoi(argv[1]);
    }

    ifstream volunteerFile("volunteers.csv");
    ifstream questFile("quests.csv");

    vector<Volunteer> volunteers;
    vector<Quest> quests;

    // Parse volunteer file
    string volunteerLine;
    // read starting time
    getline(volunteerFile, volunteerLine);
    time_t startingTime, endingTime;
    struct tm tm;
    strptime(volunteerLine.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
    startingTime = mktime(&tm);
    // add 1 hour to the starting time to get ending Time
    endingTime = startingTime + 3600;

    // use header line to get the hours
    getline(volunteerFile, volunteerLine);
    // store the hours in a vector
    istringstream iss(volunteerLine);
    string id, name, hour;
    getline(iss, id, ',');
    getline(iss, name, ',');
    // format : start - end. store the hours as time_t in two vectors start and end for each slot
    vector<time_t> startTimes;
    vector<time_t> endTimes;
    while (getline(iss, hour, ','))
    {
        // istringstream iss2(hour);
        // string startHour, endHour;
        // getline(iss2, startHour, '-');
        // getline(iss2, endHour, '-');
        // // convert the hours to time_t
        // struct tm tm;
        // strptime(startHour.c_str(), "%Hh", &tm);
        // time_t startHour_t = mktime(&tm);
        // strptime(endHour.c_str(), "%Hh", &tm);
        // time_t endHour_t = mktime(&tm);
        // startTimes.push_back(startHour_t);
        // endTimes.push_back(endHour_t);

        startTimes.push_back(startingTime);
        endTimes.push_back(endingTime);
        // shift by one hour
        startingTime = endingTime;
        endingTime = startingTime + 3600;
    }

    int n = 0;
    while (getline(volunteerFile, volunteerLine))
    {
        istringstream iss(volunteerLine);
        string id, name, hour;
        getline(iss, id, ',');
        getline(iss, name, ',');
        Volunteer volunteer;
        volunteer.name = name;
        volunteer.num = n;
        n++;
        while (getline(iss, hour, ','))
        {
            volunteer.hours.push_back(stoi(hour));
        }
        volunteers.push_back(volunteer);
    }

    cout << "parsed " << n << " volunteers" << endl;

    // Parse quest file
    n = 0;
    string questLine;
    string dum;
    getline(questFile, questLine); // Skip header line
    while (getline(questFile, questLine))
    {
        // cout << "questLine: " << questLine << endl;
        istringstream iss(questLine);
        string id, name, needed, start, end;
        getline(iss, id, ',');
        // for name, get what is between the quotes, for instance "a,b,c" -> a,b,c
        getline(iss, dum, '\"');
        getline(iss, name, '\"');
        getline(iss, dum, ',');
        getline(iss, needed, ',');
        getline(iss, start, ',');
        getline(iss, end, ',');
        Quest quest;
        quest.num = n;
        n++;
        quest.name = name;
        quest.needed = stoi(needed);
        //cout << "quest " << quest.name << " needed: " << quest.needed << endl;
        // convert the start and end to time_t
        struct tm tm;
        strptime(start.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
        quest.start = mktime(&tm);
        strptime(end.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
        quest.end = mktime(&tm);
        // compute duration in minutes
        quest.duration = difftime(quest.end, quest.start) / 60;
        //if quest.duration is bigger or equal than 2h, split the quest starting with a 1h quest
        while(quest.duration>=120){
            Quest quest2;
            quest2.num = n;
            n++;
            quest2.name = quest.name;
            quest2.needed = quest.needed;
            quest2.start = quest.start;
            quest2.end = quest.start + 3600;
            quest2.duration = 60;
            quest.duration -= 60;
            quest.start += 3600;
            quests.push_back(quest2);
        }
        //cout << "quest " << quest.name << " duration: " << quest.duration << endl;
        quests.push_back(quest);
    }

    // harmonie hours of timetable and hours of quests.

    cout << "parsed " << n << " quests" << endl;

    // create a vector storing the volunteers that are available for each quest
    for (int i = 0; i < quests.size(); i++)
    {
        for (int j = 0; j < volunteers.size(); j++)
        {
            // check if the volunteer is available for the quest.
            bool available = true;
            int inconvenience = 0; // max inconvenience
            for (int k = 0; k < volunteers[j].hours.size(); k++)
            {
                if (volunteers[j].hours[k] < 3) // available is <3
                {
                    inconvenience = max(inconvenience, volunteers[j].hours[k]);
                    continue;
                }
                // if not available, check intersection of the quest time with the volunteer's hours
                // overlap is when the quest start is before the end of the volunteer's hour and the quest end is after the start of the volunteer's hour
                if (quests[i].start < endTimes[k] && quests[i].end > startTimes[k])
                {
                    available = false;
                    break;
                }
            }
            if (available)
            {
                volunteers[j].availableQuests.push_back(i);
                volunteers[j].inconveniences.push_back(inconvenience);
            }
        }
    }

    // print avaible quests for each volunteer
    //  for (int i = 0; i < volunteers.size(); i++)
    //  {
    //      cout << "volunteer " << volunteers[i].name << " is available for "<< volunteers[i].availableQuests.size() << " quests: "<<endl;
    //  }

    // fill the quest compatibilities vectors
    for (int i = 0; i < quests.size(); i++)
    {
        for (int j = 0; j < quests.size(); j++)
        {
            if (i == j)
            {
                continue;
            }
            // check if the quests are incompatible, ie if intervals overlap
            // overlap is when the quest start is before the end of the other quest and the quest end is after the start of the other quest
            if (quests[i].start < quests[j].end && quests[i].end > quests[j].start)
            {
                quests[i].incompatibleQuests.push_back(j);
            }
        }
    }

    // generate the smt file for z3
    ofstream smtFile("constraints.smt2");

    smtFile << "(set-option :produce-models true)" << endl;

    // declare variables for each volunteer and each of his available quests
    for (int i = 0; i < volunteers.size(); i++)
    {
        for (int j : volunteers[i].availableQuests)
        {
            smtFile << "(declare-const " << "v" << i << "q" << j << " Bool)" << endl;
        }
    }

    // add constraints that the quests of each volunteer cannot be incompatible
    for (int i = 0; i < volunteers.size(); i++)
    {
        for (int j = 0; j < volunteers[i].availableQuests.size(); j++)
        {
            int iq = volunteers[i].availableQuests[j];
            Quest quest = quests[iq];
            for (int m = j + 1; m < volunteers[i].availableQuests.size(); m++)
            {
                int k = volunteers[i].availableQuests[m];
                // check if the quest appears in the available quests of the volunteer
                if (find(quest.incompatibleQuests.begin(), quest.incompatibleQuests.end(), k) != quest.incompatibleQuests.end())
                {
                    smtFile << "(assert (not (and v" << i << "q" << iq << " v" << i << "q" << k << ")))" << endl;
                }
            }
        }
    }

    // each quest must fill its number of needed volunteers
    for (int i = 0; i < quests.size(); i++)
    {
        smtFile << "(assert ((_ at-least " << quests[i].needed << ") ";
        for (int j = 0; j < volunteers.size(); j++)
        {
            //if quest appears in available
            if(find(volunteers[j].availableQuests.begin(), volunteers[j].availableQuests.end(), i) != volunteers[j].availableQuests.end())
            {
                smtFile << "v" << j << "q" << i << " ";
            }
        }
        smtFile << "))" << endl;
    }

    //also at-most to have exactly the right number of volunteers
    for (int i = 0; i < quests.size(); i++)
    {
        smtFile << "(assert ((_ at-most " << quests[i].needed << ") ";
        for (int j = 0; j < volunteers.size(); j++)
        {
            //if quest appears in available
            if(find(volunteers[j].availableQuests.begin(), volunteers[j].availableQuests.end(), i) != volunteers[j].availableQuests.end())
            {
                smtFile << "v" << j << "q" << i << " ";
            }
        }
        smtFile << "))" << endl;
    }

    // compute the total duration for each volunteer (in minutes)
    for (int i = 0; i < volunteers.size(); i++)
    {
        smtFile << "(declare-const v" << i << "duration Int)" << endl;
        smtFile << "(assert (= v" << i << "duration (+ ";
        for (int j = 0; j < volunteers[i].availableQuests.size(); j++)
        {
            int iq = volunteers[i].availableQuests[j];
            smtFile << "(ite v" << i << "q" << iq << " " << quests[iq].duration << " 0) ";
        }
        smtFile << ")))" << endl;
    }

    // ask a max duration per volunteer
    for (int i = 0; i < volunteers.size(); i++)
    {
        smtFile << "(assert (<= v" << i << "duration " << maxdur << "))" << endl;
    }

    // compute the inconvenience of each volunteer
    // for (int i = 0; i < volunteers.size(); i++)
    // {
    //     smtFile << "(declare-const v" << i << "inconv Int)" << endl;
    //     smtFile << "(assert (= v" << i << "inconv (+ ";
    //     for (int j = 0; j < volunteers[i].availableQuests.size(); j++)
    //     {
    //         int iq = volunteers[i].availableQuests[j];
    //         smtFile << "(ite v" << i << "q" << iq << " " << volunteers[i].inconveniences[j] << " 0) ";
    //     }
    //     smtFile << ")))" << endl;
    // }

    // minimize the sum of inconveniences
    // smtFile << "(declare-const totalInconv Int)" << endl;
    // smtFile << "(assert (= totalInconv (+ ";
    // for (int i = 0; i < volunteers.size(); i++)
    // {
    //     smtFile << "v" << i << "inconv ";
    // }
    // smtFile << ")))" << endl;
    // smtFile << "(minimize totalInconv)" << endl;

    // end of the file
    smtFile << "(check-sat)" << endl;
    smtFile << "(get-model)" << endl;
    // close the file
    smtFile.close();

    // run z3
    system("z3 constraints.smt2 > z3result");

    // parse result by printing the quests and the volunteers assigned to them
    ifstream resultFile("z3result");
    string resultLine;
    getline(resultFile, resultLine); // sat line
    if (resultLine != "sat")
    {
        cout << resultLine << endl;
        return 0;
    }

    // parse the model
    string output;
    while (getline(resultFile, resultLine))
    {
        output += resultLine + "\n";
    }
    map<string, bool> model = parseModelBool(output);

    // print the result
    for (int i = 0; i < quests.size(); i++)
    {
        cout << "Quest "  << i <<" " << quests[i].name << " (";
        //print start and end time, just the hours
        struct tm *tm = localtime(&quests[i].start);
        cout << tm->tm_hour << "h-";
        tm = localtime(&quests[i].end);
        cout << tm->tm_hour << "h) ";
        cout << "assigned to:";
        bool first=true;
        for (int j = 0; j < volunteers.size(); j++)
        {
            for (int k = 0; k < volunteers[j].availableQuests.size(); k++)
            {
                if (volunteers[j].availableQuests[k] == i)
                {
                    if (model["v" + to_string(j) + "q" + to_string(i)])
                    {
                        if(!first){
                            cout << ","; 
                        }
                        cout << " " << volunteers[j].name;
                        first=false;
                    }
                }
            }
        }
        cout << endl;


    }

    //parse durations for each volunteer
    for (int i = 0; i < volunteers.size(); i++)
    {
        int duration = 0;
        for (int j = 0; j < volunteers[i].availableQuests.size(); j++)
        {
            int iq = volunteers[i].availableQuests[j];
            if (model["v" + to_string(i) + "q" + to_string(iq)])
            {
                duration += quests[iq].duration;
            }
        }
        cout << "Volunteer " << volunteers[i].name << " duration: " << duration << endl;
    }

    // parse totalInconv
    int totalInconv = 0;
    for (int i = 0; i < volunteers.size(); i++)
    {
        for(int j = 0; j < volunteers[i].availableQuests.size(); j++)
        {
            int iq = volunteers[i].availableQuests[j];
            if (model["v" + to_string(i) + "q" + to_string(iq)])
            {
                totalInconv += volunteers[i].inconveniences[j];
            }
        }
    }
    cout << "Total inconvenience: " << totalInconv << endl;

    return 0;
}