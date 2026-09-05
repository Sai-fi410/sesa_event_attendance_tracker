# Event Attendance Tracker

A simple web-based attendance management system built with a C++ backend server and a responsive HTML/CSS/JavaScript frontend. The application manages student registration data, verifies attendance at the event entrance, saves data persistently to a CSV file, and provides real-time visual statistics.

---

## Core Requirements & Features

1. **Registration Data**
   - Stores participant details: College ID, Name, Email, Phone Number, Branch, and Year.
   - Allows adding new registered students through a simple form.

2. **Participant Directory & Search**
   - Displays all registered participants in a searchable list.
   - Supports filtering by Branch (CSE, ECE, ME, IT, CE) and Status (Present, Absent).
   - Allows searching by Name, Email, or College ID.

3. **Entrance Verification**
   - Search student by College ID, Name, Email, or Phone at the entrance gate.
   - Displays student details and entry status.
   - Provides a single button to mark attendance as Present with a timestamp.
   - Displays a clear notification if the student is not registered.

4. **Data Persistence**
   - All participant records and attendance statuses are stored in `data/participants.csv`.
   - Data persists across server restarts and browser reloads.

5. **Analytics Dashboard**
   - Interactive visual charts using Chart.js.
   - Single dynamic chart view with toggle options for:
     - Overall Attendance Breakdown (Present vs Absent)
     - Year-Wise Attendance Breakdown (1st, 2nd, 3rd, 4th Year)
     - Branch-Wise Attendance Breakdown (CSE, ECE, ME, IT, CE)

---

## Project File Structure

- `main.cpp`: C++ source code containing data structures, CSV file handling, and the Winsock HTTP web server.
- `index.html`: Main HTML interface containing the header stats, mode switcher, search form, directory table, and chart container.
- `styles.css`: CSS stylesheet defining layout alignments, card styles, and table formatting.
- `app.js`: JavaScript code managing API requests, search filtering, and chart rendering.
- `event_tracker.exe`: Compiled executable program.
- `data/participants.csv`: Persistent CSV file storing participant records.

---

## System Requirements

- Operating System: Windows 10 or Windows 11
- Compiler: MinGW / GCC (`g++`) or MSVC (`cl`) with C++11 support
- Socket Library: Winsock2 (`ws2_32.lib`)
- Web Browser: Any standard web browser

---

## Compilation Instructions

To compile the C++ backend server using GCC (`g++`):

```cmd
g++ -O2 main.cpp -lws2_32 -o event_tracker.exe
