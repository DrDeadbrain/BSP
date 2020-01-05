#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define USER_INPUT_SIZE 50
#define CHAR_COUNT ((int) ('z' - 'a') + 1) //NOC in Alphabet

void encode(char *input, char *output, int outputSize, int shiftNum);
void decode(char *input, char *output, int outputSize, int shiftNum);

int main() {
    bool exit = false;
    char userInput[USER_INPUT_SIZE] = {0};
    while (!exit) {
        fgets(userInput, USER_INPUT_SIZE, stdin);
        if (userInput[0] == 'x') {
            exit = true;
        }

        char buff[USER_INPUT_SIZE] = {0};
        encode(userInput, buff, USER_INPUT_SIZE, 6);
        printf("encoded: %s [%d]\n", buff, buff[0]);
        decode(buff, userInput, USER_INPUT_SIZE, 6);
        printf("decoded: %s [%d]\n", userInput, userInput[0]);
    }
}

void encode(char *input, char *output, int outputSize, int shiftNum) {
    if (shiftNum < 0) {
        shiftNum = shiftNum * -1;
    }
    int lastIdx = 0;
    for (int i = 0; (input[i] != '\0') && (i < (outputSize - 1) && (input[i] != '\n')); i++) {
        char nxtChar = input[i];
        if ((nxtChar >= 'a') && (nxtChar <= 'z')) {
            output[i] = ((input[i] - 'a' + shiftNum) % (CHAR_COUNT)) + 'a';
        } else if ((nxtChar >= 'A') && (nxtChar <= 'Z')) {
            output[i] = ((input[i] - 'A' + shiftNum) % (CHAR_COUNT)) + 'A';
        } else {
            output[i] = input[i];
        }
        lastIdx = i;
    }
    output[lastIdx + 1] = '\0';
}

void decode(char *input, char *output, int outputSize, int shiftNum) {
    encode(input, output, outputSize, CHAR_COUNT - shiftNum);
}