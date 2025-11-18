// q5_high_grade_index.cpp
// Compile: g++ -std=c++17 q5_high_grade_index.cpp -O2 -o q5_index
// Run: ./q5_index
// Make sure students_3000.csv (3000 records) is in the same directory.
//
// Program:
//  - Reads students_3000.csv
//  - Builds unordered_map<string, vector<size_t>> index for students with grade >= 9.0 per course
//  - Interactive prompt: query course code (e.g., OOPS or 110) -> returns matching students quickly
//  - Demonstrates update of a student's grade and incremental maintenance of the index

#include <bits/stdc++.h>
using namespace std;

struct Student {
    string name;
    string roll;
    string branch;
    int start_year = 0;
    vector<string> current_courses;
    vector<pair<string,double>> prev_courses; // course_code | grade
};

static inline string trim(const string &s) {
    size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size(); while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

static inline vector<string> split_csv_line(const string &line, char delim=',') {
    vector<string> out; string cur; bool inquotes=false;
    for (size_t i=0;i<line.size();++i) {
        char c=line[i];
        if (c=='"') { inquotes = !inquotes; continue; }
        if (c==delim && !inquotes) { out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}

static inline vector<string> parse_semis(const string &s) {
    vector<string> out; string cur;
    for (char c : s) {
        if (c == ';') { out.push_back(trim(cur)); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(trim(cur));
    return out;
}

static inline vector<pair<string,double>> parse_prev(const string &s) {
    vector<pair<string,double>> out;
    auto parts = parse_semis(s);
    for (auto &p : parts) {
        auto pos = p.find('|');
        if (pos == string::npos) continue;
        string code = trim(p.substr(0,pos));
        string gradeS = trim(p.substr(pos+1));
        double g = 0.0;
        try { g = stod(gradeS); } catch(...) { g = 0.0; }
        out.emplace_back(code, g);
    }
    return out;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const string csvfile = "students_3000.csv";
    ifstream fin(csvfile);
    if (!fin) {
        cerr << "ERROR: cannot open " << csvfile << ". Place it in working dir.\n";
        return 1;
    }

    // read header
    string header;
    getline(fin, header);

    vector<Student> students;
    students.reserve(3100);

    string line;
    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        auto cols = split_csv_line(line);
        if (cols.size() < 6) continue;
        Student s;
        string name = cols[0];
        if (!name.empty() && name.front() == '"') name = name.substr(1);
        if (!name.empty() && name.back() == '"') name.pop_back();
        s.name = name;

        string roll = cols[1];
        if (!roll.empty() && roll.front() == '"') roll = roll.substr(1);
        if (!roll.empty() && roll.back() == '"') roll.pop_back();
        s.roll = trim(roll);

        s.branch = trim(cols[2]);
        try { s.start_year = stoi(trim(cols[3])); } catch(...) { s.start_year = 0; }
        s.current_courses = parse_semis(cols[4]);
        s.prev_courses = parse_prev(cols[5]);
        students.push_back(move(s));
    }
    fin.close();

    cout << "Loaded " << students.size() << " students.\n";

    // Build index: course_key -> vector of student indices who got grade >= 9.0 in that course
    unordered_map<string, vector<size_t>> high_grade_index;
    high_grade_index.reserve(1024);

    for (size_t i = 0; i < students.size(); ++i) {
        const auto &prevs = students[i].prev_courses;
        for (const auto &pg : prevs) {
            // normalize key by trimming; If course identifiers can be numeric, the CSV uses numeric tokens,
            // we keep the string representation (so "110" and 110 map to "110").
            string key = trim(pg.first);
            if (pg.second >= 9.0) {
                high_grade_index[key].push_back(i);
            }
        }
    }

    // Show a few sample index sizes
    cout << "Built index. Sample entries (course -> count):\n";
    int sample = 0;
    for (auto &kv : high_grade_index) {
        cout << "  " << kv.first << " -> " << kv.second.size() << "\n";
        if (++sample >= 8) break;
    }
    cout << "Use the interactive prompt to query a course (type 'exit' to quit).\n";

    // Interactive prompt
    while (true) {
        cout << "\nEnter course id to query (e.g. OOPS or 110) > ";
        string q;
        if (! (cin >> q) ) break;
        if (q == "exit" || q == "quit") break;
        // Normalize
        string key = trim(q);

        auto it = high_grade_index.find(key);
        if (it == high_grade_index.end() || it->second.empty()) {
            cout << "No students found with grade >= 9.0 in course '" << key << "'.\n";
            continue;
        }
        const auto &vec = it->second;
        cout << "Found " << vec.size() << " student(s) with grade >= 9.0 in '" << key << "'.\n";
        // Print up to first 30 matches
        size_t to_show = min<size_t>(vec.size(), 30);
        for (size_t k = 0; k < to_show; ++k) {
            const Student &s = students[vec[k]];
            // Show name, roll, branch, start, and the grade for that course
            double grade_for_course = -1.0;
            for (const auto &pg : s.prev_courses) if (trim(pg.first) == key) { grade_for_course = pg.second; break; }
            cout << setw(3) << k+1 << ". " << s.name << " | roll: " << s.roll
                 << " | branch: " << s.branch << " | start: " << s.start_year
                 << " | grade: " << (grade_for_course >= 0 ? to_string(grade_for_course) : "N/A") << "\n";
        }
        if (vec.size() > to_show) cout << "  ... and " << (vec.size() - to_show) << " more\n";
    }

    // Demonstrate incremental update: change a student's grade for a course and update index
    // (This block is illustrative and won't run unless you want to run it manually;
    //  you can remove or adapt it for your system integration.)
    //
    // Example: update student 0's grade in course "OOPS" to 9.5 (and add to index if needed)
    //
    // string upd_course = "OOPS";
    // size_t stu_idx = 0;
    // double new_grade = 9.5;
    // // search prev_courses vector for the course and update grade; if not present, add it
    // bool found = false;
    // for (auto &pg : students[stu_idx].prev_courses) {
    //     if (trim(pg.first) == upd_course) { pg.second = new_grade; found = true; break; }
    // }
    // if (!found) students[stu_idx].prev_courses.emplace_back(upd_course, new_grade);
    // // Now update index:
    // if (new_grade >= 9.0) {
    //     auto &list = high_grade_index[upd_course];
    //     // add stu_idx if not already present
    //     if (find(list.begin(), list.end(), stu_idx) == list.end()) list.push_back(stu_idx);
    // } else {
    //     // if student's new grade fell below threshold, remove from index
    //     auto &list = high_grade_index[upd_course];
    //     list.erase(remove(list.begin(), list.end(), stu_idx), list.end());
    // }

    cout << "Exiting.\n";
    return 0;
}
