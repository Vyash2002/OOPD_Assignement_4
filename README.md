🌐 University ERP Student Management System

IIIT-Delhi — OOPD Assignment 4 (Q1–Q5 Complete Implementation)

This project is a C++-based ERP system that manages 3000 student records and implements all requirements of the OOPD Assignment 4.
It showcases advanced concepts such as generic data representation, cross-institute course mapping, custom threading, parallel sorting, iterator-based views, and efficient indexing for queries.

The system includes both:

A unified menu-driven ERP program (erp_menu.cpp), and

Five standalone programs (erp_q1.cpp … erp_q5.cpp) for individual questions.

📂 Project Structure
OOPD_Assignment_4/
│
├── basicIO.h                   # Provided input–output utilities
├── basicIO.cpp
│
├── erp_menu.cpp                # Main unified Q1–Q5 menu-driven system
│
├── erp_q1.cpp                  # Individual solution for Question 1
├── erp_q2.cpp                  # Individual solution for Question 2
├── erp_q3.cpp                  # Individual solution for Question 3
├── erp_q4.cpp                  # Individual solution for Question 4
├── erp_q5.cpp                  # Individual solution for Question 5
│
├── mythread_noos.h             # Custom fallback threading layer
│
├── students_3000.csv           # Dataset of 3000 generated student records
│
├── makefile                    # Builds all executables
│
├── students_sorted_q3.csv      # Output (Q3 sorted records)
├── students_sorted_menu.csv    # Output (Q4 iterator-based sort)
├── mapping_report.csv          # Output (Q2 mappings)
├── high_grade_students.csv     # Output (Q5 grade index export)
│
└── README.md                   # Documentation

🚀 Features (Q1 — Q5)

Each part of the assignment is implemented both standalone and inside the unified menu program.

🔹 Q1 — Flexible Student Class & Sample Output

Universities differ in:

Roll number types → numeric / string

Course identification → integer / acronym

This system handles all variations using generic string-based parsing.

Q1 Features:

✔ Reads all 3000 students
✔ Displays 3–4 sample students showing:

Name

Roll number + detected type

Branch

Start year

Current courses

Previous courses & grades

❗ No CSV export (as requested)
❗ Only a clean sample is printed

🔹 Q2 — IIT–IIIT Course Code Mapping

IIIT-Delhi students can take IIT-Delhi courses.
Both use different naming conventions:

Institute	Course Code Type
IIT Delhi	integer (e.g., 101, 202)
IIIT Delhi	acronym (e.g., OOPS, DSA)
Q2 Capabilities:

✔ Automatic detection of code type (numeric/string)
✔ Default IIT↔IIIT mapping
✔ User can add/override custom mappings
✔ Shows mapped courses for students
✔ Optional export → mapping_report.csv

Example Mapping:

101 → OOPS
OOPS → 101

🔹 Q3 — Parallel Sorting (Custom Threads)

Sorting is done using:

branch → start_year → roll


However, your compiler does not support <thread> or <pthread>.
So a custom thread handler mythread_noos.h simulates multi-threading.

Q3 Features:

✔ “Threads” divide the array into chunks
✔ Per-thread timing logs
✔ Final sorted merge (k-way merge)
✔ Optional export → students_sorted_q3.csv

🔹 Q4 — Iterator-Based Views (Zero Copy)

Sorting without copying student objects, using:

vector<size_t> index mapping

Forward iterators (ascending order)

Reverse iterators (descending order)

Q4 Features:

✔ Original input order view
✔ Sorted ascending view
✔ Sorted descending view
✔ Zero-copy efficiency
✔ Optional export → students_sorted_menu.csv

🔹 Q5 — Fast Queries Using Grade Index

A precomputed unordered_map enables O(1) lookup:

course_code → vector of students with grade ≥ 9.0

Q5 Features:

✔ Instant search by course (e.g., OOPS, 101)
✔ Shows all students with grade ≥ 9
✔ Optional export → high_grade_students.csv

📸 Menu Interface Screenshot (Text View)
===== ERP Menu (Q1 - Q5) =====
1) Show sample students (Q1)
2) IIT <-> IIIT course mapping (Q2)
3) Parallel sorting with threads (Q3)
4) Iterator-based sorted views (Q4)
5) Fast grade lookup (Q5)
6) Reload CSV
0) Exit
----------------------------------------------
Enter choice:

🛠️ Compilation and Execution
✔ Build using Makefile
make


This generates executables:

erp_menu

erp_q1, erp_q2, erp_q3, erp_q4, erp_q5

✔ Run menu-driven ERP program
./erp_menu

✔ Run any specific question individually
./erp_q1
./erp_q2
./erp_q3
./erp_q4
./erp_q5

🧵 Threading Implementation Note

Since the compiler does not support std::thread and pthread,
we provide:

mythread_noos.h

A No-OS simulated threading interface that preserves the structure of:

thread.start(task);
thread.join();


But runs tasks sequentially so the program works everywhere.

📊 Dataset: students_3000.csv

The dataset contains:

3000 randomly generated students

Roll numbers of mixed formats

Randomized courses (IIT + IIIT)

Random GPA distributions

Clean CSV format with:

name, roll, branch, start_year, current_courses, previous_courses

🎓 Educational Concepts Demonstrated

✔ Object-Oriented Design (Classes, Encapsulation)
✔ File Handling & CSV Parsing
✔ Iterator-based architecture
✔ Custom threading abstraction
✔ Multi-way merge sorting
✔ Indexing & hashing for O(1) lookup
✔ Menu-driven user interface
✔ Clean modular project structure

👨‍💻 Author

Yash Verma
B.Tech CSE
IIIT-Delhi
