📘 ERP Student Management System (OOPD Assignment — IIIT-Delhi)

This project is a menu-driven ERP system developed as part of the Object-Oriented Programming & Design (OOPD) course.
It loads and manages student data from a CSV file (students_3000.csv) and demonstrates advanced C++ concepts including:

1. Generic data handling for different universities

2. Course code mapping between IIT-Delhi & IIIT-Delhi

3. Multi-threaded sorting

4. Iterator-based views

5. fficient grade-based indexing

6. Clean menu-driven UI

The application integrates all features from Q1 to Q5 into a single executable: erp_menu.


🔹 Features Overview

✅ Q1 — Flexible Student Representation + Sample Display

Universities use different types for roll numbers (string / integer) and course codes.
This system stores these flexibly and prints 3–4 sample students showing:

1. Name

2. Roll number (and its type: numeric / string)

3. Branch

4. Start year

5. Current courses

6. Previous courses with grades

❗ Q1 does not export to CSV — only prints samples to screen.


✅ Q2 — IIT–IIIT Course Mapping

IIIT-Delhi students may take IIT-Delhi courses where:

1. IIIT course codes are strings (e.g., OOPS, DSA).

2. IIT course codes are integers (e.g., 101, 202).

The program:

1. Detects whether a course is IIT-style or IIIT-style

2. Maps courses between IIT and IIIT (default + user-defined mappings)

3. Prints mapped courses for each student

4. Optionally exports a full mapping_report.csv

5. Mapping examples:

  101 -> OOPS
  OOPS -> 101
  

✅ Q3 — Parallel Sorting (Multi-threaded)

Students are sorted based on:

  branch → starting year → roll number


Supports:

1. Choosing number of worker threads

2. Per-thread timing

3. k-way merge of sorted partitions

4. Optional export to students_sorted_q3.csv


✅ Q4 — Iterator-Based Views (Zero Copy)

Sorts students without copying objects using:

1. Index vectors

2. Iterator-style traversal

3. Reverse iteration for descending order

Also offers optional export to:
students_sorted_menu.csv

✅ Q5 — Fast Queries Using Pre-built Grade Index

An efficient lookup table indexes all students who scored:

  grade ≥ 9.0


Features:

Instant search by course (e.g., OOPS, 110)

Optional full export to high_grade_students.csv

📂 Project Structure
OOPD_Assignment_4/
│
├── basicIO.h
├── basicIO.cpp
│
├── erp_menu.cpp           # Main unified menu for Q1–Q5
├── erp_q1.cpp             # Standalone code for Q1 (if run separately)
├── erp_q2.cpp             # Standalone code for Q2
├── erp_q3.cpp             # Standalone code for Q3
├── erp_q4.cpp             # Standalone code for Q4
├── erp_q5.cpp             # Standalone code for Q5
│
├── mythread_noos.h        # Custom thread abstraction (fallback threading)
│
├── students_3000.csv      # Required dataset (3000 student records)
├── makefile 


🚀 How to Build & Run
Compile
make clean
make

Run
./erp_menu


Make sure students_3000.csv is in the same directory as the executable.

🧩 How the Menu Looks
===== ERP Menu (Q1 - Q5) =====
1) Q1: Show sample students (3-4) with roll types, courses & grades
2) Q2: IIT<->IIIT course mapping (view / export / edit)
3) Q3: Parallel sort (per-worker times) and export sorted CSV
4) Q4: Entered/sorted views using iterators and export
5) Q5: Fast query / export students with grade >= 9.0
6) Reload CSV
0) Exit
----------------------------------------------
Enter choice:


🛠 Concepts Demonstrated

This project showcases multiple advanced C++ concepts:

🧵 Threading

1. std::thread, pthread, or fallback simulated threads

2. Thread-safe timing logs

3. K-way merge of sorted partitions

📦 Templates & Flexibility

1. Roll numbers accepted as both numeric and string types

2. Course codes stored generically

🔍 Parsing & CSV Handling

1. Robust CSV parsing (quoted fields supported)

2. Semicolon-delimited list parsing

🗂 Efficient Data Structures

1. unordered_map index for fast grade lookups

2. Vector-based student store

3. Iterator-based sorted views


📌 Requirements

C++17

Linux / macOS / Windows (MinGW / WSL)

Input file: students_3000.csv

📜 Example Output Snippet (Q1)
----- Sample Student #1 -----
Name: Riya Sharma
Roll: 102034
(type: numeric)
Branch: CSE | Start Year: 2020
Current courses: OOPS, DBMS
Previous courses with grades:
  - MATH101 | grade: 9.5
  - PHY101 | grade: 8.7

🤝 Contribution Guidelines

Feel free to:

1. Add new mappings

2. Improve UI

3. Extend multithreading features

4. Add new query modules

📄 License

This project is part of the IIIT-Delhi OOPD Assignment and is for academic use only.
