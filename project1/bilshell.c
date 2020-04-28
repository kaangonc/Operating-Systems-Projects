#include <stdio.h>
#include <stdlib.h>

int MAX_CHAR_AMOUNT = 128;

char* readLine()
{
    char* line = NULL;
    size_t size = 0;
    size_t ch = getline(&line, &size, stdin);
    return line;
}
/*
char* getTokensFromLine()
{

}
*/
/*
char** readLine(int *compound)
{
    char* tokens[maxCharAmount];
    char c = getchar();
    printf("%c", c);
    char * token = malloc();
    int tokenCount = 0;
    compound = 0;
    int tokenLength =0;

    while ( c != EOF && c != '\n')
    {
        printf("%c", c);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\a')
        {
            tokens[tokenCount] = token;
            printf("%s", token);
            tokenCount++;
            token = "";
            tokenLength = 0;
        }
        else if (c == '|' && *compound == 0)
        {
            *compound = 1;
            getchar();
        }
        else
        {
            token[tokenLength] = c;
            printf("%d\n", tokenLength);
            printf("%s\n", token);
            tokenLength++;
        }
        c = getchar();
    }
    return tokens;

}
*/
int main(int argc, char **argv)
{
    char* line = readLine();
    printf("%s", line);
    return 0;
}
