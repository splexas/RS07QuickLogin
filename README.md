# RS07QuickLogin
This is a simple program written in C for educational purposes that logs in accounts listed in the text file quickly in Old School RuneScape.
# Setup
- You must have C++ OSRS client installed on your computer. However, it is possible modify some lines of code so that the program becomes functional on Java clients (`FindWindow`, `FindWindowEx`).
- Add the accounts in `accounts.txt` with `username:password` syntax.
# Usage
To run this program, the first argument must be the path of your OSRS executable game file, e.g. `main.exe C:\OSRS\osclient.exe`.
# Build
`gcc main.c -o main`
