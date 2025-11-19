// erp_q2.cpp
// Q2: Show sample students whose courses map between IIITD (string codes)
// and IITD (numeric codes). Optional CSV export of all mapped records.
//
// Compile:
//   g++ -std=c++17 erp_q2.cpp -O2 -o erp_q2
// Run:
//   ./erp_q2
//
// Expects students_3000.csv in current directory.

#include <bits/stdc++.h>
using namespace std;

// ---------- CSV parsing helpers ----------
static inline string trim(const string &s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

static vector<string> split_csv_line(const string &line, char delim=',') {
    vector<string> out;
    string cur;
    bool inq = false;
    for (char c : line) {
        if (c == '"') { inq = !inq; continue; } // drop quotes
        if (c == delim && !inq) { out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}

static vector<string> split_semis(const string &s) {
    vector<string> out; string cur;
    for (char c : s) {
        if (c == ';') { out.push_back(trim(cur)); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(trim(cur));
    return out;
}

static vector<pair<string,double>> parse_prev(const string &s) {
    vector<pair<string,double>> out;
    for (auto &p : split_semis(s)) {
        auto pos = p.find('|');
        if (pos == string::npos) continue;
        string code = trim(p.substr(0,pos));
        double grade = 0.0;
        try { grade = stod(trim(p.substr(pos+1))); } catch(...) {}
        out.emplace_back(code, grade);
    }
    return out;
}

// ---------- Student struct ----------
struct Student {
    string name;
    string roll;
    string branch;
    int start_year = 0;
    vector<string> current_courses;
    vector<pair<string,double>> prev_courses;
};

// ---------- default mapping (IIT int -> IIIT string) ----------
static unordered_map<int,string> default_iit2iiit() {
    return {
        {101,"OOPS"},{102,"DSA"},{103,"MTH"},{201,"DBMS"},{202,"OS"},
        {301,"CN"},{302,"NLP"},{401,"ML"},{402,"AI"},{501,"SE"}
    };
}

// determine whether token is numeric (pure digits)
static bool is_numeric_token(const string &t) {
    if (t.empty()) return false;
    for (char c : t) if (!isdigit((unsigned char)c)) return false;
    return true;
}

// ---------- main ----------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const string csvfile = "students_3000.csv";
    ifstream fin(csvfile);
    if (!fin) {
        cerr << "ERROR: cannot open " << csvfile << ". Place it in working dir.\n";
        return 1;
    }

    // read CSV
    string header;
    getline(fin, header);
    vector<Student> students;
    string line;
    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        auto cols = split_csv_line(line);
        if (cols.size() < 6) continue;
        Student s;
        s.name = trim(cols[0]);
        string roll = cols[1];
        if (!roll.empty() && roll.front()=='"') roll = roll.substr(1);
        if (!roll.empty() && roll.back()=='"')  roll.pop_back();
        s.roll = trim(roll);
        s.branch = trim(cols[2]);
        try { s.start_year = stoi(trim(cols[3])); } catch(...) { s.start_year = 0; }
        s.current_courses = split_semis(cols[4]);
        s.prev_courses = parse_prev(cols[5]);
        students.push_back(move(s));
    }
    fin.close();

    cout << "Loaded " << students.size() << " students from " << csvfile << ".\n";

    // build mapping
    auto iit2iiit = default_iit2iiit();
    unordered_map<string,int> iiit2iit;
    for (auto &kv : iit2iiit) iiit2iit[kv.second] = kv.first;

    // collect mapped records: for each student, record any mapping occurrences
    struct MapRecord {
        size_t student_idx;
        string student_name;
        string student_roll;
        string mapping_direction; // "IIIT->IIT" or "IIT->IIIT"
        string course_from;       // as in CSV
        string course_to;         // mapped counterpart
        double grade;             // for prev-course mapping (or -1)
        bool is_prev;
    };

    vector<MapRecord> mapped;

    for (size_t i = 0; i < students.size(); ++i) {
        auto &s = students[i];

        // current courses
        for (auto &c : s.current_courses) {
            string tok = trim(c);
            if (tok.empty()) continue;
            if (is_numeric_token(tok)) {
                // numeric token => IIT course; see if maps to IIIT
                int id = 0;
                try { id = stoi(tok); } catch(...) { continue; }
                auto it = iit2iiit.find(id);
                if (it != iit2iiit.end()) {
                    mapped.push_back({i, s.name, s.roll, "IIT->IIIT", tok, it->second, -1.0, false});
                }
            } else {
                // string token => IIIT course; see if maps to IIT
                auto it2 = iiit2iit.find(tok);
                if (it2 != iiit2iit.end()) {
                    mapped.push_back({i, s.name, s.roll, "IIIT->IIT", tok, to_string(it2->second), -1.0, false});
                }
            }
        }

        // previous courses (with grades)
        for (auto &p : s.prev_courses) {
            string tok = trim(p.first);
            double grade = p.second;
            if (tok.empty()) continue;
            if (is_numeric_token(tok)) {
                int id = 0;
                try { id = stoi(tok); } catch(...) { continue; }
                auto it = iit2iiit.find(id);
                if (it != iit2iiit.end()) {
                    mapped.push_back({i, s.name, s.roll, "IIT->IIIT", tok, it->second, grade, true});
                }
            } else {
                auto it2 = iiit2iit.find(tok);
                if (it2 != iiit2iit.end()) {
                    mapped.push_back({i, s.name, s.roll, "IIIT->IIT", tok, to_string(it2->second), grade, true});
                }
            }
        }
    }

    if (mapped.empty()) {
        cout << "No cross-system mappings found (using current default mapping table).\n";
        return 0;
    }

    cout << "Found " << mapped.size() << " mapping occurrences (current + previous courses).\n";

    // We'll show a sample: up to N distinct students with at least one mapping
    const size_t SAMPLE_STUDENTS = 8;
    unordered_set<size_t> shown_students;
    vector<size_t> sample_indices;
    for (auto &rec : mapped) {
        if (shown_students.size() >= SAMPLE_STUDENTS) break;
        if (shown_students.insert(rec.student_idx).second) {
            sample_indices.push_back(rec.student_idx);
        }
    }

    // Print samples with their mapping occurrences
    cout << "\n--- Sample mapped students (showing up to " << SAMPLE_STUDENTS << ") ---\n\n";
    for (auto si : sample_indices) {
        auto &s = students[si];
        cout << "Student: " << s.name << "  |  Roll: " << s.roll << "  | Branch: " << s.branch << " | Year: " << s.start_year << "\n";

        // gather their mappings
        for (auto &rec : mapped) {
            if (rec.student_idx != si) continue;
            cout << "  [" << rec.mapping_direction << "] " << rec.course_from << "  ->  " << rec.course_to;
            if (rec.is_prev) cout << "   (prev course, grade=" << fixed << setprecision(1) << rec.grade << ")";
            cout << "\n";
        }
        cout << "--------------------------------------------------\n";
    }

    // Ask user whether they want to export full mapped occurrences
    cout << "\nExport full mapping occurrences to 'q2_mapped_samples.csv'? (y/N): " << flush;
    string ans;
    if (!getline(cin >> ws, ans)) ans = "N";
    if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) {
        ofstream fout("q2_mapped_samples.csv");
        fout << "student_idx,name,roll,branch,mapping_dir,course_from,course_to,is_prev,grade\n";
        for (auto &rec : mapped) {
            fout << rec.student_idx << ",\"" << rec.student_name << "\",\"" << rec.student_roll << "\",\"" 
                 << students[rec.student_idx].branch << "\"," << rec.mapping_direction << ","
                 << "\"" << rec.course_from << "\"," << "\"" << rec.course_to << "\"," 
                 << (rec.is_prev ? "1" : "0") << "," << (rec.is_prev ? to_string(rec.grade) : "") << "\n";
        }
        fout.close();
        cout << "Exported q2_mapped_samples.csv (" << mapped.size() << " rows).\n";
    }

    cout << "\nDone.\n";
    return 0;
}
