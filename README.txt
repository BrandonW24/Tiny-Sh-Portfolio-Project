This can be done either on the os1 server and or in the bash terminal.

To create an executable from my file enter this command into the command prompt:

gcc -o smallsh withingb3.c

There are two warnings but they are not detrimental

then to execute the grading script for the assignment; 
the grading script must first be placed into the same directory as the
executable for my smallsh program.

Then
type into the command prompt : 
chmod +x ./p3testscript

Please execute the grading script in the following manner!
type the following line into the command prompt to execute the script:

./p3testscript 2>&1 | more

This is how I tested my program as I was making it.

It will output to the terminal from there and create a file named 1 that contains my
test results.
