// erp_q1_no_os_threads.cpp
// Compile: g++ -std=c++17 erp_q1_no_os_threads.cpp -O2 -o erp_q1_noos
// No -pthread required. This uses a no-OS-thread fallback (mythread_noos.h).
//
// Behavior: same as prior ERP program, but without any real threading support.
// It still logs "worker" timings (they measure execution time of each synchronous worker).

#include <bits/stdc++.h>
#include "mythread_noos.h"

using namespace std;
using Clock = chrono::high_resolution_clock;
using ms = chrono::duration<double, milli>;

// -----------------------------
// Flexible Student template
// -----------------------------
template<typename RollT, typename CourseT, typename GradeT>
struct CourseRecord {
    CourseT course;
    GradeT grade;
};

template<typename RollT, typename CourseT, typename GradeT>
class Student {
private:
    string name_;
    RollT roll_;
    string branch_;
    int start_year_;
    vector<CourseT> current_courses_;
    vector<CourseRecord<RollT, CourseT, GradeT>> prev_courses_;
public:
    Student() = default;
    Student(const string &name, const RollT &roll, const string &branch, int start_year)
        : name_(name), roll_(roll), branch_(branch), start_year_(start_year) {}

    void addCurrentCourse(const CourseT &c) { current_courses_.push_back(c); }
    void addPrevCourse(const CourseT &c, const GradeT &g) {
        CourseRecord<RollT, CourseT, GradeT> cr;
        cr.course = c;
        cr.grade = g;
        prev_courses_.push_back(cr);
    }
    const string& name() const { return name_; }
    const RollT& roll() const { return roll_; }
    const string& branch() const { return branch_; }
    int start_year() const { return start_year_; }
    const vector<CourseT>& current_courses() const { return current_courses_; }
    const vector<CourseRecord<RollT, CourseT, GradeT>>& prev_courses() const { return prev_courses_; }

    string toCSVRow() const {
        ostringstream oss;
        oss << '"' << name_ << '"' << ",";
        { ostringstream r; r << roll_; oss << '"' << r.str() << '"' << ","; }
        oss << branch_ << "," << start_year_ << ",";
        for (size_t i = 0; i < current_courses_.size(); ++i) {
            if (i) oss << ";";
            ostringstream c; c << current_courses_[i];
            oss << c.str();
        }
        oss << ",";
        for (size_t i = 0; i < prev_courses_.size(); ++i) {
            if (i) oss << ";";
            ostringstream c; c << prev_courses_[i].course;
            ostringstream g; g << prev_courses_[i].grade;
            oss << c.str() << "|" << g.str();
        }
        return oss.str();
    }
};

// -----------------------------
// SimpleThread using MyThreadNoOS::Thread (synchronous fake thread)
// -----------------------------
class SimpleThread {
public:
    using Task = function<void()>;

    SimpleThread() : started_(false) {}

    void start(Task t, const string &name = "worker") {
        using namespace MyThreadNoOS;
        if (started_) throw runtime_error("already started");
        started_ = true;
        // Wrap task to measure time and set log
        auto wrapper = [t = move(t), name, this]() {
            auto t0 = Clock::now();
            t();
            auto t1 = Clock::now();
            double dur = chrono::duration_cast<ms>(t1 - t0).count();
            log_ = name + " finished in " + to_string(dur) + " ms";
        };
        // Start synchronous "thread"
        thread_impl_ = MyThreadNoOS::Thread(wrapper);
    }

    void join() {
        // since tasks ran synchronously, join is no-op
    }

    string getLog() const { return log_; }

private:
    bool started_;
    MyThreadNoOS::Thread thread_impl_;
    string log_;
};

// -----------------------------
// CSV helpers
// -----------------------------
static inline vector<string> split(const string &s, char delim) {
    vector<string> out;
    string cur;
    bool inquotes = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '"') { inquotes = !inquotes; continue; }
        if (c == delim && !inquotes) {
            out.push_back(cur);
            cur.clear();
        } else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}
static inline string trim(const string &s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size();
    while (b> a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}
static inline vector<string> parseSemis(const string &s) {
    vector<string> out;
    if (s.empty()) return out;
    string cur;
    for (char c : s) {
        if (c == ';') { out.push_back(trim(cur)); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(trim(cur));
    return out;
}
template<typename CourseT, typename GradeT>
static inline vector<CourseRecord<unsigned long long, CourseT, GradeT>>
parsePrevCourses(const string &s) {
    vector<CourseRecord<unsigned long long, CourseT, GradeT>> out;
    vector<string> parts = parseSemis(s);
    for (auto &p : parts) {
        size_t pos = p.find('|');
        if (pos == string::npos) continue;
        string code = trim(p.substr(0,pos));
        string gradeS = trim(p.substr(pos+1));
        CourseRecord<unsigned long long, CourseT, GradeT> cr;
        if constexpr (is_convertible<string,CourseT>::value) {
            cr.course = code;
        } else {
            try { cr.course = static_cast<CourseT>(stoll(code)); } catch(...) { cr.course = CourseT(); }
        }
        GradeT g = GradeT();
        try { g = static_cast<GradeT>(stod(gradeS)); } catch(...) {}
        cr.grade = g;
        out.push_back(cr);
    }
    return out;
}

// -----------------------------
// Parallel sort (two-way) using SimpleThread (which will run sync here)
// -----------------------------
template<typename StudentT, typename Comparator>
void parallel_sort_two_way(vector<StudentT> &data, Comparator cmp, string &logA, string &logB) {
    size_t n = data.size();
    if (n <= 1) return;
    size_t mid = n/2;
    SimpleThread t1, t2;
    auto w1 = [&data, mid, &cmp]() { sort(data.begin(), data.begin() + (ptrdiff_t)mid, cmp); };
    auto w2 = [&data, mid, &cmp, n]() { sort(data.begin() + (ptrdiff_t)mid, data.end(), cmp); };
    t1.start(w1, "sorter-left");
    t2.start(w2, "sorter-right");
    // join no-ops
    t1.join();
    t2.join();
    logA = t1.getLog();
    logB = t2.getLog();
    // merge
    vector<StudentT> aux;
    aux.reserve(n);
    size_t i = 0, j = mid;
    while (i < mid && j < n) {
        if (cmp(data[i], data[j])) aux.push_back(data[i++]);
        else aux.push_back(data[j++]);
    }
    while (i < mid) aux.push_back(data[i++]);
    while (j < n) aux.push_back(data[j++]);
    for (size_t k = 0; k < n; ++k) data[k] = move(aux[k]);
}

// -----------------------------
// Main
// -----------------------------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "ERP Q1 demo (NO OS THREADS): loading students_3000.csv\n";
    string csvfile = "students_3000.csv";
    ifstream fin(csvfile);
    if (!fin) {
        cerr << "Cannot open " << csvfile << " - please place it in working dir.\n";
        return 2;
    }
    string headerLine;
    getline(fin, headerLine);

    using RollT = string;
    using CourseT = string;
    using GradeT = double;
    using MyStudent = Student<RollT, CourseT, GradeT>;

    vector<MyStudent> students;
    students.reserve(3100);
    string line;
    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        vector<string> cols = split(line, ',');
        if (cols.size() < 6) continue;
        string name = cols[0];
        if (!name.empty() && name.front() == '"') name = name.substr(1);
        if (!name.empty() && name.back() == '"') name.pop_back();
        string roll = cols[1];
        if (!roll.empty() && roll.front() == '"') roll = roll.substr(1);
        if (!roll.empty() && roll.back() == '"') roll.pop_back();
        string branch = trim(cols[2]);
        int starty = stoi(trim(cols[3]));
        string curCoursesRaw = cols[4];
        string prevRaw = cols[5];
        MyStudent s(name, roll, branch, starty);
        vector<string> curs = parseSemis(curCoursesRaw);
        for (auto &cc : curs) s.addCurrentCourse(cc);
        auto prevs = parsePrevCourses<CourseT,GradeT>(prevRaw);
        for (auto &p : prevs) s.addPrevCourse(p.course, p.grade);
        students.push_back(move(s));
    }
    fin.close();

    cout << "Loaded " << students.size() << " students.\n";
    cout << "\nFirst 5 students in entered order:\n";
    for (size_t k = 0; k < students.size() && k < 5; ++k) {
        cout << k+1 << ". " << students[k].name() << " | roll: " << students[k].roll() << " | branch: " << students[k].branch() << " | start: " << students[k].start_year() << "\n";
    }

    auto cmp = [](const MyStudent &a, const MyStudent &b) {
        if (a.branch() != b.branch()) return a.branch() < b.branch();
        if (a.start_year() != b.start_year()) return a.start_year() < b.start_year();
        return a.roll() < b.roll();
    };

    string logA, logB;
    auto t0 = Clock::now();
    parallel_sort_two_way<MyStudent>(students, cmp, logA, logB);
    auto t1 = Clock::now();
    double dur_total = chrono::duration_cast<ms>(t1 - t0).count();

    cout << "\nSorting complete. Total wall time: " << dur_total << " ms\n";
    cout << "Worker logs:\n" << logA << "\n" << logB << "\n";

    cout << "\nFirst 5 students in sorted order:\n";
    for (size_t k = 0; k < students.size() && k < 5; ++k) {
        cout << k+1 << ". " << students[k].name() << " | roll: " << students[k].roll() << " | branch: " << students[k].branch() << " | start: " << students[k].start_year() << "\n";
    }

    ofstream fout("students_sorted.csv");
    fout << "name,roll,branch,start_year,current_courses,previous_courses_with_grades\n";
    for (auto it = students.begin(); it != students.end(); ++it) {
        fout << it->toCSVRow() << "\n";
    }
    fout.close();
    cout << "\nWrote students_sorted.csv (" << students.size() << " rows).\n";

    // build simple index
    unordered_map<string, vector<size_t>> high_grade_index;
    for (size_t idx = 0; idx < students.size(); ++idx) {
        const auto &prevs = students[idx].prev_courses();
        for (const auto &pr : prevs) {
            ostringstream ss; ss << pr.course;
            string key = ss.str();
            if (pr.grade >= 9.0) high_grade_index[key].push_back(idx);
        }
    }
    cout << "\nBuilt quick index for students with grade >= 9 per course (sample sizes):\n";
    size_t cnt = 0;
    for (auto &kv : high_grade_index) {
        cout << "Course " << kv.first << " -> " << kv.second.size() << " students\n";
        if (++cnt >= 5) break;
    }
    cout << "\nDemo complete.\n";
    return 0;
}
