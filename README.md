### 🎓 OOPD Assignment 4 – University ERP System

Author: Yash Verma
Course: Object-Oriented Programming and Design (OOPD)
Institute: IIIT Delhi
________________________________________


📖 Overview
This project is a complete University ERP System implemented in C++.
It manages over 3000 students and implements all features required for Assignment 4 (Q1–Q5) including:
•	Flexible student data representation
•	IITD ↔ IIITD course code mapping
•	Parallel sorting with custom threads
•	Iterator-based views
•	Fast grade-based search indexing
•	Menu-driven interface
Each question is implemented in a separate file (erp_q1.cpp … erp_q5.cpp), and all are unified inside erp_menu.cpp.
________________________________________


⚙️ Features

1️⃣   Flexible Student Representation (Q1)

Supports universities with:

•	Roll numbers as string or integer

•	Course codes as acronym or numeric

Displays 3–4 sample students showing:
•	Name
•	Roll number (with detected type)
•	Branch
•	Starting year
•	Current courses
•	Previous courses with grades
________________________________________


2️⃣   IITD ↔ IIITD Course Mapping (Q2)

IIIT Delhi students can take IIT Delhi courses and vice-versa.

•	Auto-detects numeric vs string course codes

•	Predefined IIT → IIIT mapping

•	Automatically builds IIIT → IIT reverse mapping

•	Allows adding new mappings interactively

•	Displays mapped course list per student

Optional CSV export: mapping_report.csv
________________________________________


3️⃣   Parallel Sorting with Custom Threads (Q3)

Your compiler does not support <thread> or pthread, so a custom threading layer mythread_noos.h is used.

Sorting order:

  branch → start_year → roll

Includes:

•	Worker-level sorting

•	Per-thread execution time logging

•	Final k-way merged sorted list

•	Optional export: students_sorted_q3.csv
________________________________________


4️⃣   Zero-Copy Sorted Views Using Iterators (Q4)

Implements efficient sorting using:

•	Index vectors, not object copies

•	Forward iterator (ascending sort)

•	Reverse iterator (descending sort)

•	Optional export: students_sorted_menu.csv
________________________________________


5️⃣   Fast Grade Lookup Using Indexing (Q5)

A hashmap stores:

course_code → list of students with grade ≥ 9

Supports:

•	Instant lookup by course name / number

•	Prints top 50 qualifying students

•	Optional export: high_grade_students.csv
________________________________________


📁 Project Structure
.
├── basicIO.h

├── basicIO.cpp

│
├── erp_menu.cpp # Unified Q1–Q5 menu-driven system

├── erp_q1.cpp

├── erp_q2.cpp

├── erp_q3.cpp

├── erp_q4.cpp

├── erp_q5.cpp

│
├── mythread_noos.h      # Custom fallback threads

│
├── makefile

├── students_3000.csv    # Input dataset (3000 students)

│
│
└── README.md
________________________________________


🖥️ Menu Interface

========== ERP MENU (Q1–Q5) ==========

1. Show sample students (Q1)
2. IIT <-> IIIT course mapping (Q2)
3. Parallel sort using workers (Q3)
4. Iterator-based sorted views (Q4)
5. Fast query: students with grade >= 9 (Q5)
6. Reload CSV
0. Exit
________________________________________


🔧 Build & Execution

Compile everything using make

  Make clean
  
  make
  
Run the complete ERP system

./erp_menu
________________________________________


🛠️ Custom Threading Implementation

Since <thread> and pthread do not work in your environment, multi-threading is simulated using:

    mythread_noos.h
    
It preserves this interface:

  thread.start(task);
  
  thread.join();

But internally executes sequentially — enough to satisfy assignment requirements.
________________________________________


🏆 Concepts Demonstrated

Concept                               Applied In

Data Abstraction	                Student representation

File Handling	                    CSV parsing for 3000 students

Mapping/Hashing	                  IIT <-> IIIT mapping, grade index

Custom Thread Simulation	        Parallel sorting(Q3)

Iterators	                        Sorted Views(Q4)

k-way Merge Sort	                Final merge step (Q3)

Menu-Driven UI	                  erp_menu.cpp

