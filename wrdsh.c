#include<stdio.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<ctype.h>
#include<fcntl.h>

// testing I/O
#define MAX_SENTENCE_LENGTH 100


/*
    Command struct, has member args: array of strings which will hold the command arguments including
    the command itself
*/
typedef struct command{
    // array of arguments such as {"ls", "-l", "-a"}
    const char** args;
}CMD;

typedef struct commandInfo{
    CMD* arrayOfCommands;   // array of commands
    int numOfCommands;      // number of commands
}CMDI;


/*  
    Function description:
        Parse, and count how many output operator and pipes there are in the line.
    Parameters: pipePos a string to be modified to represent ordering of pipes/operators
                numPipes a pointer to an integer that represents total number of pipes and output operators
    Return: Returns the line of command that the user typed in
*/
char* getArgs(char* pipePos, int* numPipes){
    // set line to null for getline()'s allocation for storing line
    char *line = NULL;
    size_t autoBuf = 0;
    int MAX_ARGS = 30;
    
    int wordCount = getline(&line, &autoBuf, stdin); 
    
    if(wordCount == -1){        // getline() error check
        if(feof(stdin)){        // end of file reached, no errors occured, just return line
            return line;
        }
        // gives Error: error message
        perror("Error: ");
    }
    
    // count pipes and redirection operators
    int IOpos = 0;
    for(int i = 0; i < wordCount; i += 1){
        if(IOpos >= MAX_ARGS){
            pipePos = (char*) realloc(pipePos, (2*MAX_ARGS)-1);
            MAX_ARGS = 2*MAX_ARGS;
        }
        if(line[i] == '|' || line[i] == '>' || line[i] == '<'){
            pipePos[IOpos] = line[i];
            IOpos+=1;
        }
    }

    *numPipes = IOpos;
        
    return line;
}

// helper function
/*
    Function description:
        check if a string is all whitespace or not
    Parameters: str - a string
    Return: 0 if string has a nonwhitespace character, 1 otherwise
*/
int whitespaceOnly(char* str){
    for(int i = 0; i < strlen(str); i += 1){
        if(!isspace((int) str[i])){
            // if there's a non whitespace character, return 0 (false) to indicate string is not entirely whitespace
            return 0;
        }
    }
    return 1;
}

/*
    Function description:
        Split up all commands and arguments given by user.
        Split it up first by the pipe/redirection delimiters (split up based on |><).
        The strings split up based on |>< are saved in args. Each string in arg is a command.
        Then loop through args and split each string in args based on whitespace and \0.
        Result is the arguments of each command. Result is then saved into allCmd->arrayOfCommands.
    Function parameter: char* line, pointer to a string (user's shell string of command)
    Function output: pointer to CMDI, a structure that contains relevant information of the user's
        command, arguments, and number of commands.
*/
CMDI* splitArgs(char* line){
    int MAX_ARGS = 30;
    int MAX_CMDNUM = 30;
    int cmdNum = 0;           // number of arguments currently in args (will reset every pipe/redirection)


    CMDI* allCMD = (CMDI*) malloc(sizeof(CMDI));
    allCMD->arrayOfCommands = (CMD*) malloc(MAX_ARGS*sizeof(CMD));

    // Array of strings containing all arguments
    char** args = (char**) malloc(MAX_ARGS*sizeof(char*));
    
    // A string. Will use this as token for strtok. Holds an entire command phrase
    char* arg;

    // get first phrase of commands (like sort | uniq | ls -l, arg = sort)
    arg = strtok(line, "|><");

    // get remaining commands: "uniq", "ls -l"
    // While loop to tokenize the line based on IO
    while(arg != NULL){
        
        // place token (word) into array
        args[cmdNum] = (char*) malloc(strlen(arg)+1);
        strcpy(args[cmdNum], arg);
        cmdNum += 1;
        
        // allocate more space if args run out of space to store arguments (highly unlikely)
        if(cmdNum >= MAX_ARGS){
            // increase the amount of arguments that args can hold
            args = realloc(args, MAX_ARGS*sizeof(char*));
            MAX_ARGS = MAX_ARGS+MAX_ARGS;
        }
        // get next phrase of commands (like echo hello there)
        arg = strtok(NULL, "|><");
    }

    // breaking up each phrase of commands into arguments using strtoken and storing it into temp
    // (such as echo hello there -> "echo", "hello", "there")
    char* temp;
    for(int i = 0; i < cmdNum; i += 1){
        int argC = 0;
        // allocate memory for array of command string 
        allCMD->arrayOfCommands[i].args = (const char**) malloc(MAX_CMDNUM*sizeof(char*));
        // a single string representing an argument from the string of command
        temp = strtok(args[i], " \n\t\0");
        while(temp != NULL){
            // Adding more arguments if needed
            if(argC >= MAX_CMDNUM){
                // allocate more space for args array
                MAX_CMDNUM = MAX_CMDNUM+MAX_CMDNUM;
                allCMD->arrayOfCommands[i].args = (const char**) realloc(allCMD->arrayOfCommands[i].args, MAX_CMDNUM*sizeof(char*));
            }
            // allocate memory for each string in the array of strings
            allCMD->arrayOfCommands[i].args[argC] = (char*) malloc((strlen(temp)+2));
            strcpy((char* restrict) allCMD->arrayOfCommands[i].args[argC], temp);
            argC += 1;
            temp = strtok(NULL, " \n\t\0");
        }
        
        allCMD->arrayOfCommands[i].args[argC] = NULL;
    }

    allCMD->numOfCommands = cmdNum;
    for(int i = 0; i < cmdNum; i += 1){
        free(args[i]);
    }
    
    free(args);
    return allCMD;
}

/*
    Function description:
        Executes a single command, no pipes or output operators
    Parameters: cmdInfo - structure that contains relevant information of the commands
    Return: 1 if command was executed successfully
*/
int executeArgs(CMDI* cmdInfo){
    CMD* cmdArr = cmdInfo->arrayOfCommands;
    //cmdArr[0].args[0] is basically saying: go into the first array of cmdArr (cmdArr[0]), 
    //then take the first string of that array (.args[0])
    if(cmdArr[0].args[0] == NULL){      // if user entered a pipe only or something
        printf("You only entered a IO redirection operator (| or > or <)!\n");
        return 1;
    }
    else if(strcmp(cmdArr[0].args[0],"exit") == 0 || strcmp(cmdArr[0].args[0],"logout") == 0 || strcmp(cmdArr[0].args[0],"quit") == 0){
        printf("...Exiting shell...\n");
        return 0;
    }
    else if(strcmp(cmdArr[0].args[0],"cd") == 0){
        chdir(cmdArr[0].args[1]);
        return 1;
    }
    else if(strcmp(cmdArr[0].args[0],"help") == 0){
        // Later on, if possible, make this cat help.txt
        printf("All UNIX commands are supported\n");
        return 1;
    }

    int rc = fork();
    if(rc < 0){     // fork failed, exit error code 1
        fprintf(stderr, "forked failed, exiting program\n");
        exit(1);
    }
    else if(rc == 0){   // successfully created new child process
        if(execvp(cmdArr[0].args[0], (char* const*) cmdArr[0].args) == -1){
            perror("Error");
            exit(1);
        }
    }
    else{
        wait(NULL);
    }
    printf("\n");
    // usually I would return 0 here, but since loop continues if 1, must return 1 for mainLoop()
    return 1;
}

// helper function for executing and redirecting pipe stuff
void pipeExecute(CMD* cmdArr,int in, int out, int cmdNum){
    int rc = fork();
    if(rc == -1){
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if(rc == 0){
        // printf("%d\n", in);
        if(dup2(in, STDIN_FILENO) == -1){   // direct STDIN to read end of pipe
            printf("in fd: %d\n", in);
            printf("failed at: %s command\n", cmdArr[cmdNum].args[0]);
            printf("pipe to read end failed\n");
            exit(1);
        }
        if(dup2(out, STDOUT_FILENO) == -1){ // direct STDOUT to write end of pipe
            printf("out fd: %d\n", out);
            printf("failed at: %s command\n", cmdArr[cmdNum].args[0]);
            printf("pipe to write end failed\n");   
            exit(1);
        }
        close(in);          // no longer need read end pipe
        close(out);         // no longer need write end pipe
        if(execvp(cmdArr[cmdNum].args[0], (char* const*) cmdArr[cmdNum].args)){
            fprintf(stderr, "%s execution failed\n", cmdArr[cmdNum].args[0]);
            exit(1);
        }
    }
    else{
        wait(NULL);
    }
}

/*
    Function description:
        Execute commands that include piping and output operators.
    Parameters: cmdInfo - a pointer to CMDI structure that contains relevant information about the inputted commands
                pipePos - string of pipes and output operators that represents its ordering
                numOfPipes - integer pointer that represents 
    Returns: 1 if the commands were executed successfully


*/
int executeArgsIO(CMDI* cmdInfo, char* pipePos, int* numOfPipes){
    CMD* cmdArr = cmdInfo->arrayOfCommands;
    int fd[2];                  // 0 is read, 1 is write
    int finalPipe[2];
    pipe(finalPipe);
    int nextRead = 0;           // first read end of pipe comes from STDIN (console)
    int child = fork();         
    if(child == -1){
        fprintf(stderr, "child fork failed\n");
        exit(1);
    }
    else if(child == 0){
        // if there is < (output) operator at the end (a < b | c | d)
        if(pipePos[0] == '<'){
            for(int cmdNum = cmdInfo->numOfCommands-1; cmdNum >= 2; cmdNum -=1){
                pipe(fd);
                pipeExecute(cmdArr, nextRead, fd[1], cmdNum);
                close(fd[1]);             // finished writing
                nextRead = fd[0];         // sets up read end of pipe for next command
            }
            dup2(nextRead, STDIN_FILENO);
            close(STDOUT_FILENO);
            open(cmdArr[0].args[0], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
            close(fd[1]);
            execvp(cmdArr[1].args[0], (char* const*) cmdArr[1].args);
        }
        // else if it's just all pipes
        else if(pipePos[0] == '|'){
            // loops until n-1 command (last command will be executed separately)
            for(int cmdNum = cmdInfo->numOfCommands-1; cmdNum >= 1; cmdNum -=1){
                pipe(fd);
                pipeExecute(cmdArr, nextRead, fd[1], cmdNum);
                close(fd[1]);             // finished writing
                nextRead = fd[0];         // sets up read end of pipe for next command
            }

            close(fd[1]);                       // no longer need to pipe to write end
            dup2(nextRead, STDIN_FILENO);       // get input from the previous command in the for loop
            close(nextRead);
            
            if(dup2(finalPipe[1], STDOUT_FILENO) == -1){
                printf("Error, failed to write to final pipe\n");
                exit(1);
            }
            execvp(cmdArr[0].args[0], (char* const*) cmdArr[0].args);
        }
    }
    else{
        close(finalPipe[1]);
        // print out the duplicated chars
        while(1){
            char temp;
            int numOfBytesRead = read(finalPipe[0], &temp, 1);
            if( numOfBytesRead == 0){
                break;
            }
            else if(numOfBytesRead == -1){
                fprintf(stderr, "Error whilst duplicating characters");
            }
            if(temp == 'c' || temp == 'm' || temp == 'p' || temp == 't'){
                printf("%c", temp);
            }
            printf("%c", temp);
        }
        wait(NULL);
    }


    printf("\n");
    return 1;
}

void mainLoop(){
    int MAX_ARGS = 30;
    int sysState = 1;
    while(sysState){
        printf("wrdsh> ");
        CMDI* cmdInfo;
        // IO redirection check
        char* pipePos = (char*) malloc((MAX_ARGS-1)*sizeof(char));
        int* numOfPipes= (int*) malloc(sizeof(int));
        
        char* line;
        line = getArgs(pipePos, numOfPipes);
        if(strcmp(line, "\n") == 0 || whitespaceOnly(line)){
            // literally do nothing if user presses enter or only input white space
        }
        else{       // if user types in something
            cmdInfo = splitArgs(line);
            if(*numOfPipes >= cmdInfo->numOfCommands){
                printf("syntax error: too many pipes/stdout operator\n");
            }
            else if(*numOfPipes > 0){
                sysState = executeArgsIO(cmdInfo, pipePos, numOfPipes);
            }
            else{
                sysState = executeArgs(cmdInfo);
            }

            // free EVERYTHING inside cmdInfo
            for(int cmdNum = 0; cmdNum < cmdInfo->numOfCommands; cmdNum += 1){
                int arg = 0;
                while(cmdInfo->arrayOfCommands[cmdNum].args[arg] != NULL){
                    free((char*) cmdInfo->arrayOfCommands[cmdNum].args[arg]);
                    arg += 1;
                }
                free((char*) cmdInfo->arrayOfCommands[cmdNum].args[arg]);
                free(cmdInfo->arrayOfCommands[cmdNum].args);
            }
            free(cmdInfo->arrayOfCommands);
            free(cmdInfo);
        }
        free(pipePos);
        free(numOfPipes); 
        free(line);
    }
}



int main(){
    mainLoop();
}