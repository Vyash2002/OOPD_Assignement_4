// erp_q4_iterators.cpp
// Q4 demonstration: show records in entered order and sorted sequences
// using different iterator types, without copying the whole data.
//
// Compile:
//   g++ -std=c++17 erp_q4_iterators.cpp -O2 -o erp_q4_iterators
// Run:
//   ./erp_q4_iterators
//
// The program expects students_3000.csv in the same directory.

#include <bits/stdc++.h>
using namespace std;

struct Student {
    string name;
    string roll;
    string branch;
    int start_year = 0;
    vector<string> current_courses;
    vector<pair<string,double>> prev_courses;
};

// --- CSV helpers (robust enough for the generated CSV) ---
static inline string trim(const string &s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

static inline vector<string> split_csv_line(const string &line, char delim=',') {
    vector<string> out;
    string cur;
    bool inquotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') { inquotes = !inquotes; continue; }
        if (c == delim && !inquotes) { out.push_back(cur); cur.clear(); }
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

// comparator: same as earlier assignments (branch, start_year, roll)
bool student_cmp(const Student &a, const Student &b) {
    if (a.branch != b.branch) return a.branch < b.branch;
    if (a.start_year != b.start_year) return a.start_year < b.start_year;
    return a.roll < b.roll;
}

// print a brief record
void print_brief(const Student &s) {
    cout << s.name << " | roll: " << s.roll << " | " << s.branch << " | " << s.start_year << "\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string csvfile = "students_3000.csv";
    ifstream fin(csvfile);
    if (!fin) {
        cerr << "ERROR: Cannot open " << csvfile << ". Place it in the working directory.\n";
        return 1;
    }

    // Read header
    string header;
    getline(fin, header);

    // Store the actual student records exactly once in a vector.
    // This is the canonical storage (entered order = push_back order).
    vector<Student> students;
    students.reserve(3100);

    string line;
    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        auto cols = split_csv_line(line);
        if (cols.size() < 6) continue; // skip malformed
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

    cout << "Loaded " << students.size() << " student records (stored once in memory).\n\n";

    // ----------------------------
    // 1) Show records in entered order using vector<Student>::const_iterator
    //    (random-access iterator for vector, but we use it as const_iterator)
    // ----------------------------
    cout << "=== Entered order (using vector<Student>::const_iterator) ===\n";
    // Demonstrates const_iterator usage (read-only traversal)
    for (vector<Student>::const_iterator it = students.cbegin(); it != students.cend(); ++it) {
        print_brief(*it);
    }
    cout << "-----------------------------------------------------------\n\n";

    // ----------------------------
    // 2) Create a sorted view WITHOUT copying all Student data:
    //    - Build a vector<Student*> pointers to original objects (copies only pointers)
    //    - Sort the pointers according to the comparator applied to *ptr.
    //    - Traverse sorted view using vector<Student*>::const_iterator (a forward iterator)
    // ----------------------------
    vector<Student*> ptrs;
    ptrs.reserve(students.size());
    for (size_t i = 0; i < students.size(); ++i) ptrs.push_back(&students[i]);

    // Sort pointer view using comparator on underlying Student objects.
    sort(ptrs.begin(), ptrs.end(), [](const Student* a, const Student* b){
        return student_cmp(*a, *b);
    });

    cout << "=== Sorted ascending (using vector<Student*>::const_iterator) ===\n";
    // Use const_iterator over pointer-vector (forward iterator)
    for (vector<Student*>::const_iterator it = ptrs.cbegin(); it != ptrs.cend(); ++it) {
        const Student* sp = *it;
        print_brief(*sp);
    }
    cout << "-----------------------------------------------------------\n\n";

    // ----------------------------
    // 3) Show sorted descending using std::reverse_iterator over the pointer vector.
    //    Demonstrates reverse_iterator type (bidirectional iterator).
    // ----------------------------
    cout << "=== Sorted descending (using std::reverse_iterator) ===\n";
    using PtrConstIter = vector<Student*>::const_iterator;
    using ReversePtrIter = std::reverse_iterator<PtrConstIter>;
    ReversePtrIter rbegin(ptrs.cend()), rend(ptrs.cbegin());
    for (ReversePtrIter rit = rbegin; rit != rend; ++rit) {
        const Student* sp = *rit;
        print_brief(*sp);
    }
    cout << "-----------------------------------------------------------\n\n";

    // ----------------------------
    // 4) Demonstrate use of an output iterator (std::ostream_iterator)
    //    to print just the names of the first 20 students in the sorted view.
    //    (ostream_iterator is an OutputIterator)
    // ----------------------------
    cout << "=== First 20 names from sorted ascending (using ostream_iterator) ===\n";
    // We'll transform pointer vector entries to names via std::transform and use ostream_iterator.
    vector<string> first20names;
    first20names.reserve(20);
    size_t limit = min<size_t>(20, ptrs.size());
    for (size_t i = 0; i < limit; ++i) first20names.push_back(ptrs[i]->name);
    // Use ostream_iterator (output iterator) to stream names separated by newline
    copy(first20names.begin(), first20names.end(), ostream_iterator<string>(cout, "\n"));
    cout << "-----------------------------------------------------------\n\n";

    // ----------------------------
    // 5) Demonstrate random-access via iterator + std::advance
    //    (use iterator to move to 100th element in entered order without using indexing)
    // ----------------------------
    cout << "=== Random access demonstration using std::advance on iterator ===\n";
    if (!students.empty() && students.size() > 100) {
        auto it = students.cbegin(); // random-access iterator
        // advance by 100 elements
        std::advance(it, 100); // valid because vector iterator is random-access
        cout << "Record at position 101 in entered order (using iterator + advance):\n";
        print_brief(*it);
    } else {
        cout << "Not enough records to demonstrate random-access advance (need >100 records).\n";
    }
    cout << "-----------------------------------------------------------\n\n";

    // ----------------------------
    // 6) Demonstrate filtered iteration WITHOUT copying entire Student objects:
    //    Build a small vector of pointers to students who have any previous grade >= 9.0,
    //    then iterate using const_iterator to show them.
    // ----------------------------
    vector<Student*> highAchievers;
    for (auto &s : students) {
        bool ok = false;
        for (auto &pr : s.prev_courses) {
            if (pr.second >= 9.0) { ok = true; break; }
        }
        if (ok) highAchievers.push_back(&s);
    }

    cout << "=== Students with previous grade >= 9.0 (using pointer vector const_iterator) ===\n";
    for (auto it = highAchievers.cbegin(); it != highAchievers.cend(); ++it) {
        print_brief(**it); // *it is Student*, then dereference for Student&
    }
    cout << "Total high-achievers found: " << highAchievers.size() << "\n";
    cout << "-----------------------------------------------------------\n\n";

    // Final note to user
    cout << "Done. Note: the actual Student objects were stored exactly once in the 'students' vector.\n";
    cout << "Sorted and filtered sequences were views into the original data using pointers/iterators\n";
    cout << "— no full-record copying occurred (only pointer/index copies).\n";

    return 0;
}
