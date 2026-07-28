#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char *value;
    size_t length;
    size_t limit;
} String;

int getStringLength(char *string){
    int i = 0;
    char character = ' ';
    while(character != '\0'){
        character = string[i];
        i++;
    }
    return i-1;
}

int indexOf(char *string, char *searchString, int startIndex, bool frontToBack){
    int searchPosition = startIndex;
    int extractPosition = 0;
    int searchStringPosition = 0;
    int stringLength = getStringLength(string);
    int searchStringLength = getStringLength(searchString);
    char extract[searchStringLength + 1];
    int returnValue = 0;
    printf("string: %s, string length: %i, start index: %i, search string: %s, search string length: %i\n", string, stringLength, startIndex, searchString, searchStringLength);
    if(frontToBack == true){
        printf("-> frontToBack == true\n");
        if((startIndex + searchStringLength) > (stringLength - 1)){
            return -2;
        }
        while(searchPosition < (stringLength - searchStringLength +1)){
            printf("-> searchPosition == %i\n", searchPosition);
            while(extractPosition < searchStringLength){
                printf("-> extractPosition == %i, searchStringPosition: %i\n", extractPosition, searchStringPosition);
                extract[extractPosition] = string[searchPosition + extractPosition];
                searchStringPosition++;
                extractPosition++;
            }
            extract[searchStringLength] = '\0';
            printf("-> extract == %s\n", extract);
            if(strcmp(extract, searchString) == 0){
                return searchPosition;
            }
            searchStringPosition = 0;
            searchPosition++;
            extractPosition = 0;
        }
    }else{
        if((stringLength - startIndex - searchStringLength) < 0){
            return -3;
        }
        while((stringLength - searchStringLength - searchPosition) > -1){
            while(searchStringLength -1 -extractPosition > -1){
                extract[searchStringLength -1 - extractPosition] = string[stringLength -1 - searchPosition - extractPosition];
                extractPosition++;
            }
            extract[searchStringLength] = '\0';
            if(strcmp(extract, searchString) == 0){
                return searchPosition;
            }
            searchPosition++;
            extractPosition = 0;
        }
    }
    return -1;
}

String createString(char string[], int limit){
    String returnValue;
    int i = 0;
    char character = ' ';
    while(character != '\0'){
        character = string[i];
        i++;
    };
    returnValue.value = string;
    if(limit < i-1){
        returnValue.limit = i-1;
    }else{
        returnValue.limit = limit;
    };
    returnValue.length = i-1;
    return returnValue;
}

String getFilePath(char filePathArgument[]){
    return createString(filePathArgument, 0);
}

int checkArgs(int argc, char *argv[]){
    if(argc < 2){
        printf("-- %s: Missing argument: provide the file (path/to/filename.extension) you wan't to use as a practice template as an argument\n", argv[0]);
        return 1;
    }
    if(argc > 2){
        printf("-- %s: Too many arguments -> pass only the typing template file as an argument\n", argv[0]);
        return 1;
    }
    if(fopen(argv[1], "r") == NULL){
        printf("-- %s: Error -> Could not find or open file '%s'\n", argv[0], argv[1]);
        return 1;
    }
    return 0;
}

FILE * getFile(String filePath){
    return fopen(filePath.value, "r");
}

void printFile(FILE *file){
    bool eof = false;
    char character = ' ';
    while(eof == false){
        character = fgetc(file);
        if(character == EOF){
            eof = true;
        }else{
            putchar(character);
        }
    }
}

int main(int argc, char *argv[]) {
    if(checkArgs(argc, argv) == 1){
        return 1;
    }
    printf("Program name: %s\n", argv[0]);
//    String filePath = getFilePath(argv[1]);
    char *filePath = argv[1];
    printf("File Path parameter: %s\n", filePath);
    char testString[] = "ooxoxxo";
    char testSearchString[] = "xx";
    printf("Index of '%s', in string '%s' = %i\n", testSearchString, testString, indexOf(testString, testSearchString, 0, true));
    printf("Same in reverse: %i\n", indexOf(testString, testSearchString, 0, false));
    FILE *practiceFile = fopen(filePath, "r");
    printFile(practiceFile);
    printf("\033[2J");
    return 0;
}
