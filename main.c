#include "header.h"

char prompt[100] = "minishell$";
char input_string[100];
int main()
{
    
    system("clear");
    /*calling scan input function*/
    scan_input(prompt,input_string);
}

