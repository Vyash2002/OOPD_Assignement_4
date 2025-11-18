// erp_q2_mapping.cpp
// Compile:
//   g++ -std=c++17 -O2 erp_q2_mapping.cpp -o erp_q2_mapping
//
// Run:
//   ./erp_q2_mapping
//
// The program expects students_3000.csv in the working directory.
// It prints every student's details and the mapped courses (IIT <-> IIIT) when available.

#include <bits/stdc++.h>
using namespace std;

// ---------------- CourseId ----------------
// A compact type that can hold either an integer course id or a string course id.
class CourseId {
public:
    enum class Kind { Int, Str };

    CourseId() : kind(Kind::Str), intv(0), strv("") {}
    CourseId(int x) : kind(Kind::Int), intv(x), strv() {}
    CourseId(long x) : CourseId(static_cast<int>(x)) {}
    CourseId(const string &s) : kind(Kind::Str), intv(0), strv(s) {}
    CourseId(const char *s) : CourseId(string(s)) {}

    bool isInt() const { return kind == Kind::Int; }
    bool isStr() const { return kind == Kind::Str; }

    int asInt() const {
        if (isInt()) return intv;
        // try to convert string to int if possible (throws if impossible)
        try {
            return stoi(strv);
        } catch(...) {
            throw runtime_error("CourseId::asInt(): not an integer");
        }
    }

    string asString() const {
        if (isStr()) return strv;
        return to_string(intv);
    }

    string toString() const {
        if (isInt()) return to_string(intv);
        return strv;
    }

    bool operator==(const CourseId &o) const noexcept {
        if (kind != o.kind) return false;
        return isInt() ? intv == o.intv : strv == o.strv;
    }

    bool operator<(const CourseId &o) const noexcept {
        if (kind == o.kind) {
            return isInt() ? (intv < o.intv) : (strv < o.strv);
        }
        // order Int before Str for stable sorting
        return kind == Kind::Int;
    }

private:
    Kind kind;
    int intv;
    string strv;
};

// ---------------- Student ----------------
struct Student {
    string name;
    string roll;
    string branch;
    int start_year = 0;
    vector<CourseId> current_courses;                   // can contain ints or strings
    vector<pair<CourseId,double>> prev_courses_with_grades;
};

// ---------------- CSV parsing helpers ----------------
static inline string trim(const string &s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

// split CSV line by top-level commas (handles quotes around name/roll)
static vector<string> split_csv_line(const string &line, char delim=',') {
    vector<string> out;
    string cur;
    bool inquotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') { inquotes = !inquotes; continue; } // drop quotes
        if (c == delim && !inquotes) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

// parse semicolon-separated list
static vector<string> parse_semis(const string &s) {
    vector<string> out;
    string cur;
    for (char c : s) {
        if (c == ';') {
            out.push_back(trim(cur));
            cur.clear();
        } else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(trim(cur));
    return out;
}

// parse a course token into CourseId: if token is digits -> int, else string
static CourseId parse_course_token(const string &tok) {
    string t = trim(tok);
    if (t.empty()) return CourseId(string(""));
    bool alldig = true;
    for (char ch : t) if (!isdigit((unsigned char)ch)) { alldig = false; break; }
    if (alldig) {
        // safe stoi
        try { return CourseId(stoi(t)); } catch(...) { return CourseId(t); }
    } else {
        return CourseId(t);
    }
}

// parse previous courses "code|grade;code2|grade2"
static vector<pair<CourseId,double>> parse_prev_courses(const string &s) {
    vector<pair<CourseId,double>> out;
    auto parts = parse_semis(s);
    for (auto &p : parts) {
        auto pos = p.find('|');
        if (pos == string::npos) continue;
        string code = trim(p.substr(0,pos));
        string gs = trim(p.substr(pos+1));
        double grade = 0.0;
        try { grade = stod(gs); } catch(...) { grade = 0.0; }
        out.emplace_back(parse_course_token(code), grade);
    }
    return out;
}

// ---------------- Course mapping ----------------
// Default mapping you can extend. Map int->string and string->int (reverse mapping).
// You can extend this dictionary based on your course catalogue.
static unordered_map<int,string> default_iit_to_iiit_map() {
    return {
        {101, "OOPS"},
        {102, "DSA"},
        {201, "DBMS"},
        {202, "OS"},
        {301, "CN"},
        {302, "NLP"},
        {401, "ML"},
        {402, "AI"},
        {501, "SE"},
        {502, "CNTR"}
    };
}

// Build reverse mapping string->int for quick lookup
static unordered_map<string,int> build_reverse_map(const unordered_map<int,string> &m) {
    unordered_map<string,int> rev;
    for (auto &kv : m) {
        rev[kv.second] = kv.first;
    }
    return rev;
}

// ---------------- Print helpers ----------------
static void print_student_full(const Student &s) {
    cout << "Name : " << s.name << "\n";
    cout << "Roll : " << s.roll << "\n";
    cout << "Branch: " << s.branch << " | Start Year: " << s.start_year << "\n";
    cout << "Current courses: ";
    if (s.current_courses.empty()) cout << "[none]";
    for (size_t i = 0; i < s.current_courses.size(); ++i) {
        if (i) cout << "; ";
        cout << s.current_courses[i].toString();
    }
    cout << "\nPrevious courses (course | grade):\n";
    for (auto &pg : s.prev_courses_with_grades) {
        cout << "  - " << pg.first.toString() << " | " << fixed << setprecision(1) << pg.second << "\n";
    }
}

// For a CourseId, try to map IIT->IIIT or IIIT->IIT using provided maps
static void print_mapped_for_course(const CourseId &c, 
    const unordered_map<int,string> &iit2iiit, const unordered_map<string,int> &iiit2iit) 
{
    if (c.isInt()) {
        int x = c.asInt();
        auto it = iit2iiit.find(x);
        if (it != iit2iiit.end()) {
            cout << c.toString() << "  => IIITD: " << it->second;
        } else {
            cout << c.toString() << "  => [no IIIT mapping]";
        }
    } else {
        string s = c.asString();
        auto it = iiit2iit.find(s);
        if (it != iiit2iit.end()) {
            cout << s << "  => IITD: " << it->second;
        } else {
            cout << s << "  => [no IIT mapping]";
        }
    }
}

// ---------------- Main program ----------------
int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "ERP Q2: IIIT-OOPD students with IITD/IIITD course mapping\n";
    cout << "Reading students_3000.csv (expected in CWD)...\n";

    ifstream fin("students_3000.csv");
    if (!fin) {
        cerr << "Error: cannot open students_3000.csv in current directory.\n";
        return 2;
    }

    string header;
    getline(fin, header); // skip header

    vector<Student> students;
    students.reserve(3100);

    string line;
    size_t line_no = 0;
    while (getline(fin, line)) {
        ++line_no;
        if (trim(line).empty()) continue;
        auto cols = split_csv_line(line);
        if (cols.size() < 6) {
            // skip malformed lines but warn
            cerr << "Warning: skipping malformed CSV line " << line_no << "\n";
            continue;
        }
        Student s;
        // name
        string name = cols[0];
        if (!name.empty() && name.front() == '"') name = name.substr(1);
        if (!name.empty() && name.back() == '"') name.pop_back();
        s.name = trim(name);
        // roll
        string roll = cols[1];
        if (!roll.empty() && roll.front() == '"') roll = roll.substr(1);
        if (!roll.empty() && roll.back() == '"') roll.pop_back();
        s.roll = trim(roll);
        s.branch = trim(cols[2]);
        try { s.start_year = stoi(trim(cols[3])); } catch(...) { s.start_year = 0; }
        // current courses: semicolon separated tokens, parse each token into CourseId
        auto curTokens = parse_semis(cols[4]);
        for (auto &tok : curTokens) {
            if (!trim(tok).empty()) s.current_courses.push_back(parse_course_token(tok));
        }
        // previous courses with grades
        s.prev_courses_with_grades = parse_prev_courses(cols[5]);

        students.push_back(move(s));
    }
    fin.close();

    cout << "Loaded " << students.size() << " students.\n\n";

    // default mapping (you can modify/augment this map)
    auto iit2iiit = default_iit_to_iiit_map();
    auto iiit2iit = build_reverse_map(iit2iiit);

    // For convenience, allow user to optionally add mappings from stdin (simple interactive).
    cout << "Default IIT->IIIT mapping contains " << iit2iiit.size() << " entries.\n";
    cout << "Would you like to add or override mappings now? (y/N): ";
    string resp;
    if (!getline(cin, resp)) resp = "N";
    if (!resp.empty() && (resp[0]=='y' || resp[0]=='Y')) {
        cout << "Enter mappings one per line in the format: <IIT_number> <IIIT_acronym>\n";
        cout << "Empty line to finish.\n";
        while (true) {
            cout << "> ";
            string linein;
            if (!getline(cin, linein)) break;
            linein = trim(linein);
            if (linein.empty()) break;
            // parse
            stringstream ss(linein);
            int iitnum; string iiitcode;
            if (!(ss >> iitnum >> iiitcode)) {
                cout << "Invalid line; expected: <int> <string>\n";
                continue;
            }
            iit2iiit[iitnum] = iiitcode;
            iiit2iit[iiitcode] = iitnum;
            cout << "Added mapping: " << iitnum << " -> " << iiitcode << "\n";
        }
        cout << "Mapping updated. Total entries: " << iit2iiit.size() << "\n";
    }

    cout << "\n--- Printing each student and mapped courses ---\n\n";

    // We'll print every student with mapping of their current & previous courses
    for (size_t i = 0; i < students.size(); ++i) {
        cout << "========== Student " << (i+1) << " ==========\n";
        print_student_full(students[i]);

        // Mapped current courses
        cout << "Mapped current courses:\n";
        if (students[i].current_courses.empty()) cout << "  [none]\n";
        for (auto &c : students[i].current_courses) {
            cout << "  - ";
            print_mapped_for_course(c, iit2iiit, iiit2iit);
            cout << "\n";
        }

        // Mapped previous courses (with grades)
        cout << "Mapped previous courses (with grades):\n";
        if (students[i].prev_courses_with_grades.empty()) cout << "  [none]\n";
        for (auto &pg : students[i].prev_courses_with_grades) {
            cout << "  - ";
            print_mapped_for_course(pg.first, iit2iiit, iiit2iit);
            cout << "  | grade: " << fixed << setprecision(1) << pg.second << "\n";
        }
        cout << "\n";
    }

    cout << "Done. Printed " << students.size() << " students with mappings.\n";
    cout << "If you want a summary view (for example: show only IIT-course-mapped students), run the program again and\n";
    cout << "provide additional filters or ask me to modify this tool to produce CSV/JSON outputs.\n";
    return 0;
}
