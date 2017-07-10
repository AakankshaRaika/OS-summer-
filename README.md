Welcome to the Bazinga team's “virtualmem” project for the CSE421 summer course. Below you will find instructions on how to run the program, and all the options it comes with. 

Installation : 
Download the zip file. 
Unzip it.
Copy the virtualmem directory to your user home directory i,e. (/home/<user>/)

Server setup (Tried on timberlake and ubuntu):
Navigate to the virtualmem directory
Type 'make' to compile. 
Once you have finished compiling you should have a executable file in your directory. 
Check if the file has execute permissions using the ls command. If not, run “chmod 777 virtualmem” command
If this still has file permission errors, navigate to the /home/<user>/ directory where the project2 file sits and type the command: chmod +x virtualmem
If the it still says permission denied. Navigate to project2 folder and do chmod +x virtualmem. Then delete any .o and executable files in the project.Remake the project  with the make command. And try running the server again. If fails repeat step 6, it might take a few times before it lets you run it.  

Run Server : 
Type './virtualmem (options)' 
You can chose from the following options.

−h : Print a usage summary with all options and exit. 
−f available-frames : Set the number of available frames. By default it should be 5. 
−r replacement-policy : Set the page replacement policy. It can be either 
FIFO (First-in-first-out) 
LFU (Least-frequently-used) 
LRU-STACK (Least-recently-used stack implementation) 
LRU-CLOCK (Least-recently-used clock implementation – second chance alg.). LRU-REF8 (Least-recently-used Reference-bit Implementation, using 8 reference bits) The default will be FIFO. 
−i input file : Read the page reference sequence from a specified file. If not given, the sequence should be read from STDIN (ended with ENTER). 



