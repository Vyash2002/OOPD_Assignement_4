// q3_parallel_sort.cpp
// Q3: Read 3000 student records from CSV and perform parallel sorting using at least two threads.
// Supports three backends selected at compile time:
//   -USE_STD_THREAD  : use C++ std::thread, std::mutex (recommended)
//   -DUSE_POSIX      : use POSIX pthreads (compile with -pthread)
//   -DNO_THREADS     : no real threads; workers run synchronously (fallback)
//
// Compile examples:
// 1) Using std::thread (Linux/Windows compilers that support it):
//    g++ -std=c++17 q3_parallel_sort.cpp -O2 -DUSE_STD_THREAD -o q3_qsort
//    ./q3_qsort
//
// 2) Using POSIX pthreads (Linux/macOS):
//    g++ -std=c++17 q3_parallel_sort.cpp -O2 -DUSE_POSIX -pthread -o q3_qsort
//    ./q3_qsort
//
// 3) If neither are available (fallback synchronous workers):
//    g++ -std=c++17 q3_parallel_sort.cpp -O2 -DNO_THREADS -o q3_qsort
//    ./q3_qsort
//
// Place students_3000.csv in the same folder before running.
// Output: students_sorted_q3.csv and console logs showing per-worker times.
//

#include <bits/stdc++.h>
using namespace std;
using Clock = chrono::high_resolution_clock;
using ms = chrono::duration<double, milli>;

// ------------------------
// Minimal portable threading abstraction
// ------------------------
// We provide a tiny Thread and Mutex wrapper that maps to std::thread, pthread, or a no-threads fallback.
// The rest of the program uses only Thread, Mutex, LockGuard types.

#if defined(USE_STD_THREAD)

  // C++ standard threading
  #include <thread>
  #include <mutex>
  struct ThreadWrapper {
    using Task = function<void()>;
    ThreadWrapper() = default;
    template<typename F>
    void start(F&& f) { thr = std::thread(std::forward<F>(f)); started = true; }
    void join() { if (started && thr.joinable()) thr.join(); started = false; }
  private:
    std::thread thr;
    bool started = false;
  };
  using MutexWrapper = std::mutex;
  struct LockGuard { explicit LockGuard(MutexWrapper &m):m(m){ m.lock(); } ~LockGuard(){ m.unlock(); } private: MutexWrapper &m; };

#elif defined(USE_POSIX)

  // POSIX pthreads
  #include <pthread.h>
  #include <errno.h>
  struct MutexWrapper {
    MutexWrapper(){ pthread_mutex_init(&m, nullptr); }
    ~MutexWrapper(){ pthread_mutex_destroy(&m); }
    void lock(){ pthread_mutex_lock(&m); }
    void unlock(){ pthread_mutex_unlock(&m); }
    pthread_mutex_t* native(){ return &m; }
  private:
    pthread_mutex_t m;
  };
  struct LockGuard { explicit LockGuard(MutexWrapper &m):mref(m){ mref.lock(); } ~LockGuard(){ mref.unlock(); } private: MutexWrapper &mref; };

  struct ThreadWrapper {
    using Task = function<void()>;
    ThreadWrapper():started(false) {}
    template<typename F>
    void start(F&& f) {
        // heap-allocate function object and pass to pthread_create
        auto fn = new Task(std::forward<F>(f));
        int rc = pthread_create(&thr, nullptr, &ThreadWrapper::entry, fn);
        if (rc != 0) { delete fn; throw runtime_error("pthread_create failed"); }
        started = true;
    }
    void join(){ if (started) { pthread_join(thr, nullptr); started = false; } }
    static void* entry(void* arg){
        Task* task = static_cast<Task*>(arg);
        try { (*task)(); } catch(...) { /* swallow */ }
        delete task;
        return nullptr;
    }
  private:
    pthread_t thr;
    bool started;
  };

#else

  // NO_THREADS fallback: execute tasks synchronously (preserve interface & logs)
  struct MutexWrapper {
    void lock() {}
    void unlock() {}
  };
  struct LockGuard { explicit LockGuard(MutexWrapper &m){} ~LockGuard(){} };

  struct ThreadWrapper {
    using Task = function<void()>;
    template<typename F>
    void start(F&& f) {
        // run synchronously
        f();
    }
    void join(){} // no-op
  };

#endif

// ------------------------
// Student representation (simple, flexible)
// We'll parse CSV rows generated earlier: name,roll,branch,start_year,current_courses,previous_courses_with_grades
// course lists semicolon-separated; previous courses are "course|grade" semicolon-separated.
// ------------------------
struct Student {
    string name;
    string roll;
    string branch;
    int start_year = 0;
    vector<string> current_courses;
    vector<pair<string,double>> prev_courses;
};

// CSV helpers
static inline string trim(const string &s) {
    size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size(); while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}
static inline vector<string> split_csv_line(const string &line, char delim=',') {
    vector<string> out; string cur; bool inquotes=false;
    for (size_t i=0;i<line.size();++i){
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

// comparator for sorting students
bool student_cmp(const Student &a, const Student &b) {
    if (a.branch != b.branch) return a.branch < b.branch;
    if (a.start_year != b.start_year) return a.start_year < b.start_year;
    return a.roll < b.roll;
}

// ------------------------
// Parallel sorting: split into N parts, sort each part in a worker thread, measure time per worker, then k-way merge
// No race conditions: each worker sorts a distinct subrange; writes its timing into a protected array with a mutex.
// Merge happens after all worker joins.
// ------------------------
void parallel_sort(vector<Student> &arr, int workers, vector<double> &worker_times_ms) {
    if (workers < 1) workers = 1;
    size_t n = arr.size();
    if (n <= 1) return;

    // compute boundaries
    vector<size_t> starts(workers), ends(workers);
    for (int i = 0; i < workers; ++i) {
        starts[i] = (n * i) / workers;
        ends[i] = (n * (i+1)) / workers;
    }

    worker_times_ms.assign(workers, 0.0);

    MutexWrapper log_mtx; // protect writes to logs (for backends requiring locking)

    vector<unique_ptr<ThreadWrapper>> threads(workers);
    for (int i = 0; i < workers; ++i) {
        threads[i] = make_unique<ThreadWrapper>();
    }

    // Launch workers
    for (int i = 0; i < workers; ++i) {
        size_t s = starts[i], e = ends[i];
        auto task = [i, s, e, &arr, &worker_times_ms, &log_mtx]() {
            auto t0 = Clock::now();
            // sort the subrange in-place
            sort(arr.begin() + (ptrdiff_t)s, arr.begin() + (ptrdiff_t)e, student_cmp);
            auto t1 = Clock::now();
            double dur = chrono::duration_cast<ms>(t1 - t0).count();
            // protect write to shared vector to be safe across all backends
            {
                LockGuard lg(log_mtx);
                worker_times_ms[i] = dur;
            }
        };
        threads[i]->start(task);
    }

    // Join workers
    for (int i = 0; i < workers; ++i) threads[i]->join();

    // Merge sorted partitions: use k-way merge via min-heap
    // Each partition is arr[starts[i] .. ends[i]-1]
    // We'll build iterators for each partition and merge into aux
    vector<Student> aux;
    aux.reserve(n);

    // Min-heap element: (Student, partition_index, index_within_partition)
    struct Item {
        const Student* s;
        int part;
        size_t idx; // absolute index in arr
    };
    struct Cmp {
        bool operator()(const Item &a, const Item &b) const {
            // want min-heap => return true if a > b
            return student_cmp(*b.s, *a.s);
        }
    };
    priority_queue<Item, vector<Item>, Cmp> pq;
    // initialize
    for (int p = 0; p < workers; ++p) {
        if (starts[p] < ends[p]) {
            pq.push(Item{ &arr[starts[p]], p, starts[p] });
        }
    }
    // track current positions
    vector<size_t> pos = starts;

    while (!pq.empty()) {
        auto it = pq.top(); pq.pop();
        aux.push_back(*it.s);
        int p = it.part;
        pos[p]++;
        if (pos[p] < ends[p]) {
            pq.push(Item{ &arr[pos[p]], p, pos[p] });
        }
    }

    // Move merged result back
    for (size_t i = 0; i < n; ++i) arr[i] = move(aux[i]);
}

// ------------------------
// Program main: read CSV, perform parallel sort, log per-thread times, write output
// ------------------------
int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string csvfile = "students_3000.csv";
    ifstream fin(csvfile);
    if (!fin) {
        cerr << "ERROR: Could not open " << csvfile << " in current directory.\n";
        cerr << "Please place students_3000.csv in the working folder and run again.\n";
        return 1;
    }

    string header;
    getline(fin, header); // skip header

    vector<Student> students;
    students.reserve(3500);
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

    cout << "Loaded " << students.size() << " students from " << csvfile << ".\n";
    if (students.empty()) return 1;

    // default worker count
    int workers = 2;
    if (argc >= 2) {
        try { workers = stoi(argv[1]); } catch(...) { workers = 2; }
    }
    if (workers < 2) workers = 2;

    cout << "Using " << workers << " worker(s) to sort.\n";

    // show first 3 before sort
    cout << "\nSample (first 3) before sort:\n";
    for (size_t i = 0; i < students.size() && i < 3; ++i) {
        cout << i+1 << ". " << students[i].name << " | " << students[i].roll << " | " << students[i].branch << " | " << students[i].start_year << "\n";
    }

    // perform parallel sort and capture per-worker times
    vector<double> times_ms;
    auto tt0 = Clock::now();
    parallel_sort(students, workers, times_ms);
    auto tt1 = Clock::now();
    double total_ms = chrono::duration_cast<ms>(tt1 - tt0).count();

    cout << "\nParallel sorting finished. Total wall time: " << total_ms << " ms\n";
    for (int i = 0; i < (int)times_ms.size(); ++i) {
        cout << "Worker " << i << " time: " << times_ms[i] << " ms\n";
    }

    // show first 3 after sort
    cout << "\nSample (first 3) after sort:\n";
    for (size_t i = 0; i < students.size() && i < 3; ++i) {
        cout << i+1 << ". " << students[i].name << " | " << students[i].roll << " | " << students[i].branch << " | " << students[i].start_year << "\n";
    }

    // write sorted CSV
    string out = "students_sorted_q3.csv";
    ofstream fout(out);
    fout << "name,roll,branch,start_year,current_courses,previous_courses_with_grades\n";
    for (auto &s : students) {
        // produce same CSV row format as input
        // quote name and roll
        fout << "\"" << s.name << "\"" << ",";
        fout << "\"" << s.roll << "\"" << ",";
        fout << s.branch << ",";
        fout << s.start_year << ",";
        // current courses joined by ;
        for (size_t i = 0; i < s.current_courses.size(); ++i) {
            if (i) fout << ";";
            fout << s.current_courses[i];
        }
        fout << ",";
        // previous courses code|grade ; ...
        for (size_t i = 0; i < s.prev_courses.size(); ++i) {
            if (i) fout << ";";
            fout << s.prev_courses[i].first << "|" << s.prev_courses[i].second;
        }
        fout << "\n";
    }
    fout.close();
    cout << "\nWrote sorted file: " << out << "\n";

    return 0;
}
