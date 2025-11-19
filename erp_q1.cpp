// erp_q1.cpp
// Q1: Universal Student class supporting multiple field types (no <variant>)
// Demonstrates extracting 3–4 students and printing their roll numbers,
// courses, and grades exactly as requested.

#include <bits/stdc++.h>
using namespace std;

// ------------------ Flexible Roll Number Type ------------------

class RollNumber {
    bool is_numeric;
    unsigned int numeric_value;
    string string_value;

public:
    RollNumber() : is_numeric(false), numeric_value(0), string_value("") {}

    // Auto-detect type
    RollNumber(const string &raw) {
        bool num = !raw.empty();
        for(char c : raw) if(!isdigit((unsigned char)c)) num = false;

        if(num) {
            is_numeric = true;
            numeric_value = (unsigned int)stoul(raw);
            string_value = "";
        } else {
            is_numeric = false;
            string_value = raw;
            numeric_value = 0;
        }
    }

    string toString() const {
        return is_numeric ? to_string(numeric_value) : string_value;
    }
};

// ------------------ Flexible CourseId Type ------------------

class CourseId {
    bool is_int;
    int int_value;
    string str_value;

public:
    CourseId() : is_int(false), int_value(0), str_value("") {}

    CourseId(const string &raw) {
        bool num = !raw.empty();
        for(char c : raw) if(!isdigit((unsigned char)c)) num = false;

        if(num) {
            is_int = true;
            int_value = stoi(raw);
            str_value = "";
        } else {
            is_int = false;
            str_value = raw;
            int_value = 0;
        }
    }

    string toString() const {
        return is_int ? to_string(int_value) : str_value;
    }
};

// ------------------ Universal Student Class ------------------

class Student {
public:
    string name;
    RollNumber roll;
    string branch;
    int start_year;

    vector<CourseId> current_courses;
    vector<pair<CourseId, double>> previous_courses;

    Student() : start_year(0) {}
};

// ------------------ CSV Utilities ------------------

string trim(const string &s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b - a);
}

vector<string> split_semicolon(const string &s) {
    vector<string> out; string cur;
    for(char c: s) {
        if(c == ';') {
            out.push_back(trim(cur));
            cur.clear();
        } else cur.push_back(c);
    }
    if(!cur.empty()) out.push_back(trim(cur));
    return out;
}

vector<pair<CourseId,double>> parse_prev(const string &s) {
    vector<pair<CourseId,double>> out;
    auto parts = split_semicolon(s);

    for(auto &p : parts) {
        auto pos = p.find('|');
        if(pos == string::npos) continue;

        string course = trim(p.substr(0,pos));
        string grade  = trim(p.substr(pos+1));

        double g = 0;
        try { g = stod(grade); } catch(...) {}

        out.emplace_back(CourseId(course), g);
    }
    return out;
}

// ------------------ Main ------------------

int main() {
    ifstream fin("students_3000.csv");
    if(!fin) {
        cerr << "ERROR: Unable to open students_3000.csv\n";
        return 1;
    }

    string header;
    getline(fin, header);

    vector<Student> students;
    string line;

    while(getline(fin, line)) {
        if(trim(line).empty()) continue;

        // CSV split (simple)
        vector<string> cols;
        string cur; bool inq = false;
        for(char c : line) {
            if(c == '"') { inq = !inq; continue; }
            if(c == ',' && !inq) {
                cols.push_back(cur);
                cur.clear();
            } else cur.push_back(c);
        }
        cols.push_back(cur);

        if(cols.size() < 6) continue;

        Student s;
        s.name  = trim(cols[0]);
        s.roll  = RollNumber(trim(cols[1]));
        s.branch = trim(cols[2]);
        s.start_year = stoi(trim(cols[3]));

        // Current courses
        for(auto &c : split_semicolon(cols[4]))
            if(!c.empty()) s.current_courses.push_back(CourseId(c));

        // Previous courses with grades
        s.previous_courses = parse_prev(cols[5]);

        students.push_back(s);
    }

    fin.close();

    // ------------------ PRINT SAMPLE 3–4 STUDENTS ------------------

    cout << "===== SAMPLE STUDENTS (Q1 Demonstration) =====\n\n";

    for(int i = 0; i < 4 && i < students.size(); i++) {
        auto &s = students[i];

        cout << "Student #" << (i+1) << "\n";
        cout << "Name: " << s.name << "\n";
        cout << "Roll Number: " << s.roll.toString() << "\n";
        cout << "Branch: " << s.branch << "\n";
        cout << "Starting Year: " << s.start_year << "\n";

        cout << "Current Courses: ";
        if(s.current_courses.empty()) cout << "[None]\n";
        else {
            for(auto &c : s.current_courses)
                cout << c.toString() << " ";
            cout << "\n";
        }

        cout << "Previous Courses with Grades:\n";
        if(s.previous_courses.empty()) cout << "   [None]\n";
        else {
            for(auto &p : s.previous_courses)
                cout << "   " << p.first.toString()
                     << " | Grade: " << p.second << "\n";
        }

        cout << "---------------------------------------\n\n";
    }

    return 0;
}
