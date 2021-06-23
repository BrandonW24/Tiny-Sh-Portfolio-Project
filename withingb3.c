/***********************************************************************************
By Brandon Withington
email : withingb@oregonstate.edu
File : withingb3.c

General description :
This program is a small simple unix shell that will be capable of
conducting the following commands :
   * exit
   * cd
   * statusNum
While also supporting comments and blank inputLine inputs that begin with the "#" character.

It provides an expansion for the variable $$.
It executes other commands by creating new processes using a function from the
exec family of functions.
It shall support input and output redirection and have custom headers for
two signals, SIGINT and SIGTSTP

General syntax of a command inputLine is :
command [arg1 arg2 ...] [< input_file] [> output_file] [&]

Where items in the square brackets are optional.
Assuming that a command is made up of words separated by spaces.
The following symbols :
<, > and & are recognized by must be surrounded by spaces (like other words).

Note : If the command is to bexecuted in the background, the last word must be "&"
If the & character appears anywhere else it will be treated as a normal text.

If standard input or output has to be redirected, > < symbols followed bya filename
word have to appear after all of the arguments. Input redirection can appear before or
after output redirection

This shell does not need to support any kind of quoting.

It must however support command inputLines with a max length of 2048 characters and a max of
512 arguments.

Error checking is not needed on the syntax of the command inputLine.

***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>


#define MAXIMUM_NUM_CHARACTERS    2048
#define MAXIMUM_NUM_ARGS          512
#define STRING_BUFFER             64

//Globals

//Counts how many signals were taken in.
int numSignals = 0;
char   inputLine [MAXIMUM_NUM_CHARACTERS];
char   inputBuffer[STRING_BUFFER];
char * tempStr;
int    smallShellpid, inputCmd, outputCmd, j;
char   smallShellpidString[MAXIMUM_NUM_ARGS];
pid_t forkProcess = 24;
bool backgroundFlag = true;
int statusNum;
int setExStatus;
//Process array
int processArrBG[40];
int overallProcessCount;
//Input array for the program
char proginp[MAXIMUM_NUM_CHARACTERS];
bool backgrFlag = false;
bool TSTPflag = false;
int exitTheChild = -2;

//Function Prototypes

void readIn();
void initSigs();
void searchBackg();
void checkInput(char out[], char in[]);
bool strEquals(const char* a, const char* b);
void attachChildpid();
void checkParents();
void processTracker();
void shiftValues(int a, int b, bool c, int d);
void bCMDs();
void runDUP(int a, int b, int c, char* cArg[MAXIMUM_NUM_ARGS]);
void removeEndNewLine(char *removeItem);
void progFork();
void redirectionCms();
void catchSIGTSTP();


/*************************************************
 * Function Name : catchSIGTSTP
 * 
 * Description : This function helps catch the signal
 *  SIGTSTP. By catching that signal, it will then stop
 *  processes that hang in the background. It will then tell
 *  the user whether or not they are entering the foreground
 *  or background of the program (by telling them they are exiting the foreground.)
 * 
 * How it is done : I will basically check to see if SIGTSTP has been evoked.
 * This is done by creating a flag variable that will either be set or not set
 * depending on whether or not the process has signaled.
 * 
 * This will help us redirect the ^Z command and instead will act
 *  similar to a keyboard macro for entering or exiting the foreground.
 * 
 * Input : None
 * 
 * Output : foregroundMsg, bckgroundMsg
 * 
**************************************************/
void catchSIGTSTP(){

	char* foregroundMsg = "Entering foreground-only mode (& is now ignored)\n";
	char bckgroundMsg[31]  = "Exiting foreground-only  mode \n";
	//Entering foreground message
	if (TSTPflag == false){
		
		numSignals = 0;
		backgrFlag = false;
		TSTPflag = true;
		printf("\n");
		write(1, foregroundMsg, 49);

		fflush(stdout);
		numSignals++;
	}
	//Exit foreground message
	else if(TSTPflag == true){

		TSTPflag = false;						
		write(1, bckgroundMsg, 31);
		printf("\n");
		fflush(stdout);
		
		numSignals++;
	}

}

/**************************************************
 * 
 * Function name : searchBackg()
 * 
 * Description   :
 * This function searches through the background to check to
 * see if the background flag has been set.
 * 
 * Input : None
 * 
 * Output : Background processes, strings
 * 
***************************************************/
void searchBackg(){

	char bckgroundArr[MAXIMUM_NUM_CHARACTERS];
    //When there is no signal to catch we switch into background mode.
	if(TSTPflag == false){
		//Set background flag.
		backgrFlag = true;
	}
	//Utilize strcpy to capture bckgroundArr and put that into proginp
	strcpy(proginp, strncpy(bckgroundArr, proginp, (strlen(proginp) - 2)));
}

/*************************************************
* Function name : readIn
* Description : gets input from the user.
*
*    This is from my previous attempt at this assignment.
*    This is messy and sloppy. It requires a globally
*    declared variable to store the input stream and check
*    on it. It worked before but it was hard to truly keep track
*    of what was going on with it. I have refactored this to
*    be more adaptful. I would like to check the input from THIS function
*    without having to pass it to another.
*
**************************************************/
/*
void readIn(){
    //Prompt for user
    fflush(stdout);
    printf(": ");
    fflush(stdout);
    if (fgets(inputLine, MAXIMUM_NUM_CHARACTERS, stdin) == NULL){
        printf("ERROR OCCURED IN THE READ-IN FUNCTION.");
        exitFlag.flag = true;
        checkifExit();                //statusNum
  }
  inputLine[strlen(inputLine) - 1] = NULL;
}*/

// colon stopped printing out, unknown reason.
// Colon is back! 
void readIn (){

	char isOutput[5] = { 0 };
    fflush(stdout);
    printf(": ");
	fgets(proginp, sizeof(proginp), stdin);
    fflush(stdout);
	//Grab input line and clean it. (remove the null terminator : '\0')
	proginp[strcspn(proginp, "\n")] = '\0';

	//Run the check input Function outside of this
	//This will check the input for any instances of
	// echo, & signals to stop, or $$

	checkInput(strncpy(isOutput, proginp, 4), proginp);

}

/*****************************************************************
 * Function name ; strEquals
 * 
 * Description:
 * String comparison helper function
 * This helps ensure and WRAP the strcmp function
 * This also helps absolutely ensure that correct
 * comparisons and judgements on those are being made.
 * 
 * Input : 
 *  string1 and string2.
 * 
 * Output : 
 *  Bool. (True or false). 
 * 
*****************************************************************/

bool strEquals(const char* a, const char* b) {

	if(strcmp(a , b) < 0){
		return false;
	} else if (strcmp(a , b) > 0){
		return false;
	} else if (strcmp(a , b) == 0){
		return true;
	}
}

/*****************************************************************
 * Function name : checkInput
 * 
 * Description :
 * This function checks the input from both the user and or
 * scripts that are being read into the program. It searches for
 * cases where the echo command can be found, it then looks for
 * the TSTP signal and $$ signals. The case where $$ happens
 * the stop signal should not be allowed to activate.
 * 
 * Input  : 
 * char out[], char in[]
 * 
 * Output :
 *  No output
 * 
 * Other functions called :
 *  searchBackg() will be called if the required parameters for it
 *  are made. Namely when the echo command is initiated, there
 *  exists an '&' within the string
 * 
 *  catchSIGTSTP(); will be called if the stop signal is initiated.
 * 
*****************************************************************/


void checkInput(char out[], char in[]){

	//Initialize Comparison variables
	const char inputChar[2]    = "<";
    const char outputChar[2]   = ">";
    const char poundChar[2]    = "#";
    const char doubleMoney[2]  = "$$";
    const char statusStr[6]    = "status";
    const char cdStr[2]        = "cd";
    const char empty[2]        = "";
    const char andSymb[2]      = "&";
    const char echoS[5]        = "echo";
    const char spaceChar[2]    = " ";
    const char stpsignal[5]    = "TSTP";
	const char exit[6]         = "exit";

	//In the case of "echo"
	if (strEquals(out, echoS) != true){
		char* search;
		search = strchr(in, '&');
		if (search != NULL){
			searchBackg();
			}
		}
	//In the case of "$$"
	char* search;
	search = strstr(in, stpsignal);
	if (search == NULL && strstr(in, "$$") != NULL){
		char expComd[MAXIMUM_NUM_CHARACTERS] = { 0 };
		int temp_pid = getppid();
		//Utilize strlen to get the length of our input.
		// subtracting it by 2 will remove the $$
		strcpy(proginp, strncpy(expComd, proginp, (strlen(proginp) - 2)));
		//Attach shell PID at the end
		sprintf(expComd, "%d", temp_pid);
		strcat(proginp, expComd);
	}
	//In the case of "TSTP" run the catchsigtstp function.
	search = strstr(in, "TSTP");
	if (search != NULL){
		catchSIGTSTP();
	}
}

/*****************************************************************
* Function name : bCMDs
* Description :
* 
*	This function checks for built in functions
*	along the lines of cd, status, exit, comments
*   There are multiple if and else if clauses
*   in this function that are used to check
*   the program input. 
*
* This program effects  :
*  proginp (not directly passed in), exitTheChild, backgrFlag

* Outputs : 
*  output statements, status statements, 
*  directories (home and newly created),
*  
* This program will call progFork(); if certain requirements are met
*
* TODO : Refactor & clean up.
*****************************************************************/
void bCMDs(){

	//Initialize our comparison character strings
	const char cdStr[4]        = "cd";
	const char statusStr[6]    = "status";
	const char exitStr[6]      = "exit";
	const char endOp[6]      = "/";
	const char poundDefine[4]  = "#";
	const char homeDefine[6]  = "HOME";

	//Need to initialize current working directory arr--
	//Getting ready to gather the new pathway.
	char currentDir[MAXIMUM_NUM_CHARACTERS];
	char* pathParse;

    //First we search for the comment command : Nothing happens here.
	if (strncmp(proginp, poundDefine, 1) == 0){
		
	}

	//Then we search for the "status" command
	else if (strEquals(proginp, statusStr) == true){
		//Should change over the course of the execution of code
		// at run time.
		printf("exit value %d\n", WEXITSTATUS(exitTheChild));
		fflush(stdout);
	}

	// Then look for the "cd" command
	else if (strncmp(proginp, cdStr, 2) == 0){
		if (pathParse = strstr(proginp, " ")){
			//Gather the current directory
			strcat(strncat(getcwd(currentDir, sizeof(currentDir)), 
			endOp, 1), pathParse + 1);
			chdir(currentDir);
		}else if (pathParse = strstr(proginp, " ") == NULL ){
			//use the built in function getenv to get our home directory.
			chdir(getenv(homeDefine));
		}
	}

	//Last search we conduct is for the exit command
	else if (strEquals(proginp, exitStr) == true){
		//If an exit command is found we set the program's exit flag.
		//ezpz
		backgroundFlag = false;									
	}

	//If and only if none of those appear in the proginp we fork our process.
	//Reduces unexpected behavior.
	else if (strEquals(proginp, statusStr) == false && strncmp(proginp, cdStr, 2) != 0 
	        && strEquals(proginp, exitStr) == false && 
			strncmp(proginp, poundDefine, 1) != 0 ){
	
			progFork();
	}
    
	backgrFlag = false;
}

/*****************************************************************
*
* Function nmae : processtracker()
*
* Description   :
*	This is the process tracker. it checks to see if children
*	from forked processes and the rest of the program have recieved
*	exit signals or other signals to end those processes. Once the
*	check is done on them the process for that background process
*	is revealed with a print statement. This also will print the
*	terminating signal and OR the exit value for that process once
*	it is terminated.
*
* inputs       : None
*
* outputs      : Print statements based on the background process ID
*				 exit statements based on WEXITSTATUS(exitTheChild)
*
*****************************************************************/
void processTracker(){

	int i;
	bool processExists = false;
	int processId = waitpid(processArrBG[i], &exitTheChild, WNOHANG);
	if(processId > 0){
		processExists = true;
	}
	//Loop through each element
	for(i = 0; i < overallProcessCount; i++){
		if (processExists == true){
			int checkifexitmethod = WIFEXITED(exitTheChild);
			int checkifSign = WIFSIGNALED(exitTheChild);
			int setSigStatus = 0;

			if (checkifSign == true){
				printf("\n");
				printf("______________________________________\n");
				printf("| background pid for the process : %d \n", processArrBG[i]);
				setSigStatus = WTERMSIG(exitTheChild);
				printf("| process was terminated by signal : %d \n", setSigStatus);
				printf("|_____________________________________| \n");
				fflush(stdout);

			}
			//TODO: Fix this
			//This prints the correct exit value approximately
			// 3/4's of the time? the rest of the time
			// it just does not print anything at all.
			// TODO complete I believe!
			else if (setSigStatus != 0){
				setExStatus = WEXITSTATUS(exitTheChild);
				printf("exit value for that process : %d\n", setExStatus);
				fflush(stdout);
			//Should act as a catcher for the rest of the cases
			// exit value would be printed.
			} else if (checkifexitmethod){
				setExStatus = WEXITSTATUS(exitTheChild);
				printf("exit value for that process : %d\n", setExStatus);
				fflush(stdout);
			}
			
		}
	}
}

/*****************************************************************
*
* Function name : progfork
*
*
* Description   :
*				This will fork the current running program. 
*               This process spawns children processes from it. 
*				The new children made from this function
*				can run built-in redirectional commands. Once this
*				is done running it will then check the parents.
*
*
******************************************************************/
void progFork(){

	const char killstr[6]        = "kill";
	//Create child process with fork
	forkProcess = fork();
	if (numSignals > 0 && strstr(proginp, killstr) != NULL){
		attachChildpid();
	}
	//Roll through child commands
	if(forkProcess == 0){
		redirectionCms();								
	}
	//Check Parent case
	else{
		checkParents();
	}
}


/*****************************************************************
*
* Function name : checkParents
*
*
* Description   :
*				 This rolls through parent processes and either
*				  will set them to sleep and wait for child processes
*				  to finish. It keeps track of the parent process count
*				  each time this function is ran it will increase that count.
*				  It will then output the background process ID for any process
*				  that passes through this function. If the background processes falg
*				  is found to not be set throughout the execution of the program / process
*				  then it will make the parent process wait for the child to be executed.
*
*
* Input  : None
* Output : process ID numbers in the form of a print statement.
*
******************************************************************/
void checkParents(){

	if (backgrFlag == true){
		processArrBG[overallProcessCount] = forkProcess;
		overallProcessCount++;
		waitpid(forkProcess, &exitTheChild, WNOHANG);
		backgrFlag = false;
		printf("\n");
		printf(" background pid for the process : %d \n", forkProcess);
		setExStatus = WEXITSTATUS(exitTheChild);

		fflush(stdout);
	}else if(backgrFlag == false){
		//Make the parent wait.
		waitpid(forkProcess, &exitTheChild, 0);
		}
}


/*****************************************************************
*
* Function name : redirectionCms
*
*
* Description   :
*           This takes care of redirections when they are found
*           in the input of our program. It searches for the appearance
*			of either "<" or ">" and makes judgements on the validity
*			of a file based on the redirection signal and or whether or
*			not the program can open the specified file. Two do while
*			loops are used in this to firstly gather our program's input
*			into the checker array and then to run our checking processes
*			over that array while there still are elements in that array
*			to check.
*
*	Note : I followed a little bit of a tutorial to utilize strtok
*		  correctly with this one.
*
* Inputs : None
*
* Outputs : Depending on the appearance of redirection symbols "<" ">"
*			and depending on the validity of our file either we will get
*			an error message if the validty of the file is questionable.
*			DUP2 and execvp are called on valid files.
*
******************************************************************/
void redirectionCms(){

	//Comparison strings
	const char inputChar[2]    = "<";
    const char outputChar[2]   = ">";

	char* cmdArg[MAXIMUM_NUM_ARGS];
	int arrayCount, fname, i = 0;
	bool redirectionFlag = false;
	bool cmdArgcheck = false;
	int track = 0;

	//Retrieves commands from input
	// Following example of :
	// https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
	// for the usage of strtok here
	cmdArg[0] = strtok(proginp, " ");

	do {
		arrayCount++;
		cmdArg[arrayCount] = strtok(NULL, " ");
		track = cmdArg[arrayCount];
		cmdArgcheck = true;
	}while(cmdArg[arrayCount] != NULL);
    //Keep going as long as data exists
	//Do while loop for this
	do {
		//Search for redirections
		if (strEquals(cmdArg[i], inputChar) == true && open(cmdArg[i + 1], O_RDONLY, 0) < 0){
			//Here we open the file utilizing read only.
			fname = open(cmdArg[i + 1], O_RDONLY, 0);
			//case where open file was unsuccessful
			perror("Error : ");
			printf("A bad file has been given to the program. ' %s ' cannot be opened. \n", cmdArg[i + 1]);	//Output error
			fflush(stdout);
			exit(1);
		}else if (strEquals(cmdArg[i], inputChar) == true && open(cmdArg[i + 1], O_RDONLY, 0) > 0){
			//Make file pointer point towards stdout
			fname = open(cmdArg[i + 1], O_RDONLY, 0);
			//Set redirect then dup2
			redirectionFlag = true;	
			runDUP(fname, 0, i, cmdArg);					
		}
		//Search for redirects in the case were the second clause
		// of the above else if is not true.
		else if (strEquals(cmdArg[i], outputChar) == true){
			//Write only file made.
			fname = open(cmdArg[i + 1], O_CREAT | O_WRONLY, 0755);
			redirectionFlag = true;
			//Set redirect then dup2
			runDUP(fname, 1, i, cmdArg);
		}

		arrayCount = arrayCount - 1;
		i = i + 1;
		redirectionFlag = false;
		
	} while (arrayCount != 0);

	//If there is no redirection; execute and print an error statement.
	if(execvp(cmdArg[0], cmdArg)){
		perror("Error : ");
		printf(" ' %s ' does not exist as a file or directory and cannot be found. \n", proginp);	//If error then ouput issue
		fflush(stdout);
		exit(1);
	}

}

/*****************************************************************
*
* Function name :
*		runDUP
*
* Description :
*		runs dup2 and execvp on processes passed into it.
*		it then utilizes fcntl to close the file.
*
* Input : fname, (0 or 1 depending on the context), i, cmdArg
*				 (0 = standard in), (1 = standard output)
*       
* Output : None.
*
*****************************************************************/

void runDUP(int a, int b, int c, char* cArg[MAXIMUM_NUM_ARGS]){

	dup2(a, b);
	cArg[c] = 0;						
	execvp(cArg[0], cArg);
	fcntl(a, F_SETFD, FD_CLOEXEC);
	
}


/*****************************************************************
*
* Function name : attachChildpid
*
*
* Description : 
*	This function is used in the case where :
*	if (numSignals > 0 && strstr(proginp, killstr) != NULL){
		attachChildpid();
	}
	
*  Terminates. When the number of signals gets to be greater than zero
*  and when the kill command is found in the stdin of our program's input
*  we then gather the child's process id number and attach it to the
*  process it belongs to.
*
*
*****************************************************************/
void attachChildpid(){

	//Initialize the command line and kill line
	char cmdLine[MAXIMUM_NUM_CHARACTERS] = { 0 };
	//Get pid to add later.
	char temp_pid = getpid();

	strcpy(proginp, strncpy(cmdLine, proginp, (strlen(proginp) - 11)));
	//Here we add the PID to the end of the cmdLine
	sprintf(cmdLine, "%d" , temp_pid);
	//We then copy it into our proginp
	strcpy(proginp, cmdLine);
	
}

/***************************************************************
*
* Function name : initSigs
*
* Description : 
*  This initializes our signals for the rest of the
*  execution of the program. This was given during lecture
*
* Input  : None
*
* Output : None
*
************************************************************/

void initSigs(){

	struct sigaction SIGINT_action = { 0 };		
	struct sigaction SIGTSTP_action = { 0 };
    struct sigaction ignore_action = { 0 };		

	SIGINT_action.sa_handler = SIG_IGN;		
	sigfillset(&SIGINT_action.sa_mask);
	sigaction(SIGINT, &SIGINT_action, NULL);

	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

}

int main(){

    initSigs();

    do{
  		processTracker();
		readIn();
		bCMDs();
	}while(backgroundFlag == true);
}