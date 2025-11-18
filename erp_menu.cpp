// erp_menu.cpp
// Unified menu program implementing Q1..Q5 features (integrates erp_q1..erp_q5).
// Compile:
//   g++ -std=c++17 erp_menu.cpp -O2 -o erp_menu
// Or use Makefile with THREAD=std or THREAD=pthread to enable real threads.
//
// Exports (when chosen):
//   - students_all_details.csv
//   - students_sorted_q3.csv
//   - students_sorted_menu.csv
//   - high_grade_students.csv
//
// Requires students_3000.csv in current directory.

#include <bits/stdc++.h>
#include "mythread_noos.h"
using namespace std;
using Clock = chrono::high_resolution_clock;
using ms = chrono::duration<double, milli>;

// ---------------- Threading abstraction ----------------
#if defined(USE_STD_THREAD)
  #include <thread>
  #include <mutex>
  struct ThreadWrapper { using Task = function<void()>; std::thread thr; bool started=false;
    template<typename F> void start(F&& f){ thr = std::thread(std::forward<F>(f)); started=true; }
    void join(){ if(started && thr.joinable()) thr.join(); started=false; }
  };
  using MutexWrapper = std::mutex;
  struct LockGuard { explicit LockGuard(MutexWrapper &m):mref(m){ mref.lock(); } ~LockGuard(){ mref.unlock(); } MutexWrapper &mref; };
#elif defined(USE_POSIX)
  #include <pthread.h>
  struct MutexWrapper { pthread_mutex_t m; MutexWrapper(){ pthread_mutex_init(&m,nullptr);} ~MutexWrapper(){ pthread_mutex_destroy(&m);} void lock(){ pthread_mutex_lock(&m);} void unlock(){ pthread_mutex_unlock(&m);} };
  struct LockGuard { explicit LockGuard(MutexWrapper &m):mref(m){ mref.lock(); } ~LockGuard(){ mref.unlock(); } MutexWrapper &mref; };
  struct ThreadWrapper {
    using Task = function<void()>;
    pthread_t thr; bool started=false;
    template<typename F> void start(F&& f){
      auto fn = new Task(std::forward<F>(f));
      if (pthread_create(&thr,nullptr, &ThreadWrapper::entry, fn)!=0){ delete fn; throw runtime_error("pthread_create failed"); }
      started = true;
    }
    void join(){ if(started){ pthread_join(thr,nullptr); started=false; } }
    static void* entry(void* arg){ Task* t = static_cast<Task*>(arg); try{ (*t)(); }catch(...){ } delete t; return nullptr; }
  };
#else
  // fallback: no-OS threads (synchronous)
  using MutexWrapper = MyThreadNoOS::Mutex;
  struct LockGuard { explicit LockGuard(MutexWrapper &m):mref(m){ mref.lock(); } ~LockGuard(){ mref.unlock(); } MutexWrapper &mref; };
  struct ThreadWrapper { template<typename F> void start(F&& f){ MyThreadNoOS::Thread t(std::forward<F>(f)); (void)t; } void join(){} };
#endif

// ---------------- Student struct ----------------
struct Student {
    string name;
    string roll;
    string branch;
    int start_year = 0;
    vector<string> current_courses;           // semicolon-separated tokens parsed into vector
    vector<pair<string,double>> prev_courses; // each pair is (course_code, grade)
};

// ---------------- CSV helpers ----------------
static inline string trim(const string &s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

static inline vector<string> split_csv_line(const string &line, char delim=',') {
    vector<string> out; string cur; bool inquotes=false;
    for (size_t i=0;i<line.size();++i) {
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
    vector<pair<string,double>> out; auto parts = parse_semis(s);
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

// ---------------- Global canonical storage / index ----------------
static vector<Student> students; // canonical store
static unordered_map<string, vector<size_t>> high_grade_index; // course -> list of student indices with grade>=9
static MutexWrapper index_mtx;

// ---------------- Load CSV ----------------
bool load_csv(const string &filename = "students_3000.csv") {
    students.clear();
    high_grade_index.clear();
    ifstream fin(filename);
    if (!fin) {
        cerr << "ERROR: cannot open '" << filename << "'\n";
        return false;
    }
    string header;
    getline(fin, header);
    string line;
    size_t idx = 0;
    while (getline(fin, line)) {
        if (trim(line).empty()) continue;
        auto cols = split_csv_line(line);
        if (cols.size() < 6) continue;
        Student s;
        string name = cols[0];
        if (!name.empty() && name.front() == '"') name = name.substr(1);
        if (!name.empty() && name.back() == '"') name.pop_back();
        s.name = trim(name);
        string roll = cols[1];
        if (!roll.empty() && roll.front() == '"') roll = roll.substr(1);
        if (!roll.empty() && roll.back() == '"') roll.pop_back();
        s.roll = trim(roll);
        s.branch = trim(cols[2]);
        try { s.start_year = stoi(trim(cols[3])); } catch(...) { s.start_year = 0; }
        s.current_courses = parse_semis(cols[4]);
        s.prev_courses = parse_prev(cols[5]);
        students.push_back(move(s));
        ++idx;
    }
    fin.close();
    // build high-grade index
    for (size_t i = 0; i < students.size(); ++i) {
        for (auto &pg : students[i].prev_courses) {
            if (pg.second >= 9.0) high_grade_index[trim(pg.first)].push_back(i);
        }
    }
    return true;
}

// ---------------- Utilities ----------------
bool student_cmp(const Student &a, const Student &b) {
    if (a.branch != b.branch) return a.branch < b.branch;
    if (a.start_year != b.start_year) return a.start_year < b.start_year;
    return a.roll < b.roll;
}

void print_student_full(const Student &s) {
    cout << "Name : " << s.name << "\n";
    cout << "Roll : " << s.roll << "\n";
    cout << "Branch: " << s.branch << " | Start Year: " << s.start_year << "\n";
    cout << "Current courses: ";
    if (s.current_courses.empty()) cout << "[none]";
    for (size_t i = 0; i < s.current_courses.size(); ++i) {
        if (i) cout << "; ";
        cout << s.current_courses[i];
    }
    cout << "\nPrevious courses (course | grade):\n";
    for (auto &pg : s.prev_courses) {
        cout << "  - " << pg.first << " | " << fixed << setprecision(1) << pg.second << "\n";
    }
}

// Default IIT->IIIT mapping; user can augment interactively
static unordered_map<int,string> default_iit_to_iiit_map() {
    return {
        {101, "OOPS"},{102,"DSA"},{201,"DBMS"},{202,"OS"},{301,"CN"},
        {302,"NLP"},{401,"ML"},{402,"AI"},{501,"SE"},{502,"CNTR"}
    };
}
static unordered_map<int,string> iit2iiit = default_iit_to_iiit_map();
static unordered_map<string,int> iiit2iit;
static void build_reverse_map() {
    iiit2iit.clear();
    for (auto &kv : iit2iiit) iiit2iit[kv.second] = kv.first;
}

// ---------------- Actions for menu options ----------------

// Q1: show number of students + optionally export full details
void action_q1_count_and_export() {
    cout << "\n[Q1] Total students loaded: " << students.size() << "\n";
    cout << "Do you want to export all student details to students_all_details.csv? (y/N): " << flush;
    string resp; getline(cin >> ws, resp);
    if (!resp.empty() && (resp[0]=='y' || resp[0]=='Y')) {
        ofstream fout("students_all_details.csv");
        fout << "name,roll,branch,start_year,current_courses,previous_courses_with_grades\n";
        for (auto &s : students) {
            fout << "\"" << s.name << "\"," << "\"" << s.roll << "\"," << s.branch << "," << s.start_year << ",";
            // current courses
            for (size_t i=0;i<s.current_courses.size();++i) {
                if (i) fout << ";";
                fout << s.current_courses[i];
            }
            fout << ",";
            // prev courses
            for (size_t i=0;i<s.prev_courses.size();++i) {
                if (i) fout << ";";
                fout << s.prev_courses[i].first << "|" << s.prev_courses[i].second;
            }
            fout << "\n";
        }
        fout.close();
        cout << "Exported students_all_details.csv\n";
    }
}

// Q2: mapping demo and detailed mapped view + export
void action_q2_mapping_and_export() {
    build_reverse_map();
    cout << "\n[Q2] IIT↔IIIT Mapping Demo\n";
    cout << "Default mapping size: " << iit2iiit.size() << "\n";
    cout << "Would you like to add/override mappings interactively? (y/N): " << flush;
    string r; getline(cin >> ws, r);
    if (!r.empty() && (r[0]=='y' || r[0]=='Y')) {
        cout << "Enter lines like: <IIT_int> <IIIT_code> (empty line to stop)\n";
        while (true) {
            cout << "> " << flush;
            string line;
            if (!getline(cin, line)) break;
            line = trim(line);
            if (line.empty()) break;
            stringstream ss(line);
            int iit; string iiit;
            if (!(ss >> iit >> iiit)) { cout << "Invalid format\n"; continue; }
            iit2iiit[iit] = iiit;
            iiit2iit[iiit] = iit;
            cout << "Added mapping " << iit << " -> " << iiit << "\n";
        }
    }
    cout << "Choose output mode:\n1) Print mapped courses per student\n2) Export mapping report CSV\nChoice (1/2, default 1): " << flush;
    string ch; getline(cin >> ws, ch);
    if (ch.empty()) ch = "1";
    if (ch == "2") {
        ofstream fout("mapping_report.csv");
        fout << "name,roll,branch,current_mapped,previous_mapped\n";
        for (auto &s : students) {
            // current mapped: for each current course show mapping if exists
            string curmap;
            for (size_t i=0;i<s.current_courses.size();++i) {
                if (i) curmap += ";";
                string c = s.current_courses[i];
                bool numeric = !c.empty();
                for (char ch : c) if (!isdigit((unsigned char)ch)) { numeric=false; break; }
                if (numeric) {
                    int id = stoi(c);
                    auto it = iit2iiit.find(id);
                    if (it != iit2iiit.end()) curmap += c + "->" + it->second;
                    else curmap += c + "->";
                } else {
                    auto it = iiit2iit.find(c);
                    if (it != iiit2iit.end()) curmap += c + "->" + to_string(it->second);
                    else curmap += c + "->";
                }
            }
            string prevmap;
            for (size_t i=0;i<s.prev_courses.size();++i) {
                if (i) prevmap += ";";
                string c = s.prev_courses[i].first;
                bool numeric = !c.empty();
                for (char ch : c) if (!isdigit((unsigned char)ch)) { numeric=false; break; }
                if (numeric) {
                    int id = stoi(c);
                    auto it = iit2iiit.find(id);
                    if (it != iit2iiit.end()) prevmap += c + "->" + it->second;
                    else prevmap += c + "->";
                } else {
                    auto it = iiit2iit.find(c);
                    if (it != iiit2iit.end()) prevmap += c + "->" + to_string(it->second);
                    else prevmap += c + "->";
                }
            }
            fout << "\"" << s.name << "\"," << "\"" << s.roll << "\"," << s.branch << "," << "\"" << curmap << "\"," << "\"" << prevmap << "\"" << "\n";
        }
        fout.close();
        cout << "Exported mapping_report.csv\n";
    } else {
        for (size_t i=0;i<students.size(); ++i) {
            cout << "---- Student: " << students[i].name << " (" << students[i].roll << ") ----\n";
            cout << "Current courses mapping:\n";
            for (auto &c : students[i].current_courses) {
                bool numeric = !c.empty();
                for (char ch : c) if (!isdigit((unsigned char)ch)) { numeric=false; break; }
                if (numeric) {
                    int id = stoi(c);
                    auto it = iit2iiit.find(id);
                    if (it != iit2iiit.end()) cout << "  " << c << " -> IIIT: " << it->second << "\n";
                    else cout << "  " << c << " -> [no mapping]\n";
                } else {
                    auto it = iiit2iit.find(c);
                    if (it != iiit2iit.end()) cout << "  " << c << " -> IIT: " << it->second << "\n";
                    else cout << "  " << c << " -> [no mapping]\n";
                }
            }
            cout << "Previous courses mapping:\n";
            for (auto &p : students[i].prev_courses) {
                auto c = p.first;
                bool numeric = !c.empty();
                for (char ch : c) if (!isdigit((unsigned char)ch)) { numeric=false; break; }
                if (numeric) {
                    int id = stoi(c);
                    auto it = iit2iiit.find(id);
                    if (it != iit2iiit.end()) cout << "  " << c << " -> IIIT: " << it->second << " | grade: " << p.second << "\n";
                    else cout << "  " << c << " -> [no mapping] | grade: " << p.second << "\n";
                } else {
                    auto it = iiit2iit.find(c);
                    if (it != iiit2iit.end()) cout << "  " << c << " -> IIT: " << it->second << " | grade: " << p.second << "\n";
                    else cout << "  " << c << " -> [no mapping] | grade: " << p.second << "\n";
                }
            }
            if ((i+1) % 25 == 0) {
                cout << "(printed " << (i+1) << " students, press Enter to continue)"; getline(cin, r);
            }
        }
        cout << "Done printing mapping details.\n";
    }
}

// Q3: parallel sort + per-worker timings; option to export full sorted CSV
void parallel_sort_workers(vector<Student> &arr, int workers, vector<double> &worker_times_ms) {
    if (workers < 1) workers = 1;
    size_t n = arr.size();
    if (n <= 1) return;
    vector<size_t> starts(workers), ends(workers);
    for (int i=0;i<workers;++i){ starts[i] = (n*i)/workers; ends[i] = (n*(i+1))/workers; }
    worker_times_ms.assign(workers, 0.0);
    vector<unique_ptr<ThreadWrapper>> th(workers);
    for (int i=0;i<workers;++i) th[i] = make_unique<ThreadWrapper>();
    MutexWrapper mtx;
    for (int i=0;i<workers;++i) {
        size_t s = starts[i], e = ends[i];
        th[i]->start([i,s,e,&arr,&worker_times_ms,&mtx](){
            auto t0 = Clock::now();
            sort(arr.begin() + (ptrdiff_t)s, arr.begin() + (ptrdiff_t)e, student_cmp);
            auto t1 = Clock::now();
            double dur = chrono::duration_cast<ms>(t1 - t0).count();
            LockGuard lg(mtx);
            worker_times_ms[i] = dur;
        });
    }
    for (int i=0;i<workers;++i) th[i]->join();
    // k-way merge
    vector<Student> aux; aux.reserve(n);
    struct Item { const Student* s; int part; size_t idx; };
    struct Cmp { bool operator()(const Item &a, const Item &b) const { return student_cmp(*b.s, *a.s); } };
    priority_queue<Item, vector<Item>, Cmp> pq;
    vector<size_t> pos(workers);
    for (int p=0;p<workers;++p) {
        if (starts[p] < ends[p]) { pq.push(Item{ &arr[starts[p]], p, starts[p] }); pos[p] = starts[p]; }
        else pos[p] = ends[p];
    }
    while (!pq.empty()) {
        auto it = pq.top(); pq.pop();
        aux.push_back(*it.s);
        int p = it.part;
        pos[p]++;
        if (pos[p] < ends[p]) pq.push(Item{ &arr[pos[p]], p, pos[p] });
    }
    for (size_t i=0;i<n;++i) arr[i] = move(aux[i]);
}

void action_q3_parallel_and_export() {
    cout << "\n[Q3] Parallel sort and export\nEnter number of workers (>=2, default 2): " << flush;
    int workers = 2;
    if (!(cin >> workers)) { cin.clear(); workers = 2; }
    string rest; getline(cin, rest);
    if (workers < 2) workers = 2;
    cout << "Sorting with " << workers << " workers...\n" << flush;
    vector<Student> arr = students; // copy
    vector<double> times_ms;
    auto t0 = Clock::now();
    parallel_sort_workers(arr, workers, times_ms);
    auto t1 = Clock::now();
    double total = chrono::duration_cast<ms>(t1 - t0).count();
    cout << "Total wall time: " << total << " ms\n";
    for (int i=0;i<(int)times_ms.size();++i) cout << " Worker " << i << " time: " << times_ms[i] << " ms\n";
    cout << "Export full sorted CSV? (y/N): " << flush;
    string r; getline(cin >> ws, r);
    if (!r.empty() && (r[0]=='y' || r[0]=='Y')) {
        ofstream fout("students_sorted_q3.csv");
        fout << "name,roll,branch,start_year,current_courses,previous_courses_with_grades\n";
        for (auto &s : arr) {
            fout << "\"" << s.name << "\"," << "\"" << s.roll << "\"," << s.branch << "," << s.start_year << ",";
            for (size_t i=0;i<s.current_courses.size();++i){ if (i) fout << ";"; fout << s.current_courses[i]; }
            fout << ",";
            for (size_t i=0;i<s.prev_courses.size();++i){ if (i) fout << ";"; fout << s.prev_courses[i].first << "|" << s.prev_courses[i].second; }
            fout << "\n";
        }
        fout.close();
        cout << "Exported students_sorted_q3.csv\n";
    }
}

// Q4: iterator views without copying entire Student objects; export sorted pointer view if requested
void action_q4_iterators_and_export() {
    cout << "\n[Q4] Views using iterators (no full-data copy)\n";
    cout << "First 5 in entered order:\n";
    for (size_t i=0;i<min<size_t>(5, students.size()); ++i) {
        cout << " " << (i+1) << ". "; print_student_full(students[i]); cout << "\n";
    }
    // build index vector (vector<size_t> acts like pointer view)
    vector<size_t> idxs(students.size());
    iota(idxs.begin(), idxs.end(), 0);
    sort(idxs.begin(), idxs.end(), [](size_t a, size_t b){
        const Student &A = students[a], &B = students[b];
        if (A.branch != B.branch) return A.branch < B.branch;
        if (A.start_year != B.start_year) return A.start_year < B.start_year;
        return A.roll < B.roll;
    });
    cout << "\nFirst 5 in sorted ascending (using index iterator):\n";
    for (size_t i=0;i<min<size_t>(5, idxs.size()); ++i) {
        cout << " " << (i+1) << ". "; print_student_full(students[idxs[i]]); cout << "\n";
    }
    cout << "\nFirst 5 in sorted descending (using reverse_iterator):\n";
    for (size_t i=0;i<min<size_t>(5, idxs.size()); ++i) {
        cout << " " << (i+1) << ". "; print_student_full(students[idxs[idxs.size()-1-i]]); cout << "\n";
    }
    cout << "Export sorted view to students_sorted_menu.csv? (y/N): " << flush;
    string r; getline(cin >> ws, r);
    if (!r.empty() && (r[0]=='y' || r[0]=='Y')) {
        ofstream fout("students_sorted_menu.csv");
        fout << "name,roll,branch,start_year,avg_prev_grade,num_prev_courses\n";
        for (auto id : idxs) {
            auto &s = students[id];
            vector<double> grades;
            for (auto &p : s.prev_courses) grades.push_back(p.second);
            string avg = "";
            if (!grades.empty()) {
                double a = accumulate(grades.begin(), grades.end(), 0.0) / grades.size();
                avg = to_string((double)round(a*100)/100.0);
            }
            fout << "\"" << s.name << "\"," << "\"" << s.roll << "\"," << s.branch << "," << s.start_year << "," << avg << "," << grades.size() << "\n";
        }
        fout.close();
        cout << "Exported students_sorted_menu.csv\n";
    }
}

// Q5: query index for students with grade >= 9.0; also export all high-grade students
void action_q5_query_and_export() {
    cout << "\n[Q5] Fast queries for students with grade >= 9.0\n";
    cout << "1) Interactive query for a course\n2) Export all high-grade students to high_grade_students.csv\nChoice (1/2, default 1): " << flush;
    string ch; getline(cin >> ws, ch);
    if (ch.empty()) ch = "1";
    if (ch == "2") {
        ofstream fout("high_grade_students.csv");
        fout << "course,name,roll,branch,start_year,grade\n";
        for (auto &kv : high_grade_index) {
            string course = kv.first;
            for (auto idx : kv.second) {
                const Student &s = students[idx];
                double grade = -1;
                for (auto &p : s.prev_courses) if (trim(p.first) == course) { grade = p.second; break; }
                fout << "\"" << course << "\"," << "\"" << s.name << "\"," << "\"" << s.roll << "\"," << s.branch << "," << s.start_year << "," << grade << "\n";
            }
        }
        fout.close();
        cout << "Exported high_grade_students.csv\n";
    } else {
        cout << "Enter course id (e.g. OOPS or 110): " << flush;
        string course;
        if (!getline(cin >> ws, course)) { cout << "No input\n"; return; }
        course = trim(course);
        if (course.empty()) { cout << "Empty\n"; return; }
        auto it = high_grade_index.find(course);
        if (it == high_grade_index.end() || it->second.empty()) {
            cout << "No students with grade >=9.0 for '" << course << "'\n";
            return;
        }
        cout << "Found " << it->second.size() << " students (showing up to 50):\n";
        size_t shown = 0;
        for (auto idx : it->second) {
            auto &s = students[idx];
            double grade = -1;
            for (auto &p : s.prev_courses) if (trim(p.first) == course) { grade = p.second; break; }
            cout << " - " << s.name << " | " << s.roll << " | " << s.branch << " | grade: " << grade << "\n";
            if (++shown >= 50) break;
        }
    }
}

// ---------------- Menu & main ----------------
void show_menu() {
    cout << "\n===== ERP Menu (Q1 - Q5) =====\n";
    cout << "1) Q1: Count & export all student details\n";
    cout << "2) Q2: IIT<->IIIT course mapping (view / export / edit)\n";
    cout << "3) Q3: Parallel sort (per-worker times) and export sorted CSV\n";
    cout << "4) Q4: Entered/sorted views using iterators (no copying) and export\n";
    cout << "5) Q5: Fast query / export students with grade >= 9.0\n";
    cout << "6) Reload CSV\n";
    cout << "0) Exit\n";
    cout << "Enter choice: " << flush;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "ERP Menu (integrated Q1..Q5) starting...\n" << flush;

    if (!load_csv("students_3000.csv")) {
        cerr << "Failed to load students_3000.csv. Place it in working directory and retry.\n";
        return 1;
    }
    build_reverse_map(); // build iiit2iit mapping from default iit2iiit
    cout << "Loaded " << students.size() << " students.\n" << flush;

    while (true) {
        show_menu();
        string choice;
        if (!(cin >> choice)) { cout << "\nInput closed, exiting.\n"; break; }
        string rest; getline(cin, rest); // eat newline
        if (choice == "0") { cout << "Exiting.\n"; break; }
        else if (choice == "1") action_q1_count_and_export();
        else if (choice == "2") action_q2_mapping_and_export();
        else if (choice == "3") action_q3_parallel_and_export();
        else if (choice == "4") action_q4_iterators_and_export();
        else if (choice == "5") action_q5_query_and_export();
        else if (choice == "6") {
            cout << "Reloading CSV...\n";
            if (load_csv("students_3000.csv")) cout << "Reloaded " << students.size() << " students.\n";
            else cout << "Reload failed.\n";
        } else {
            cout << "Unknown option '" << choice << "'. Try again.\n";
        }
        cout << "\n(press Enter to continue...) " << flush;
        string dummy; getline(cin, dummy);
    }

    return 0;
}
