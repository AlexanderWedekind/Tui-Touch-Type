#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <malloc.h>

typedef struct panel {
    char *name;
    bool open;
    struct {
        int row;
        int column;
    } topLeft;
    struct {
        int rows;
        int columns;
    } size;
    char *printArea[100];
    bool hasNested;
    struct panel *nested;
} panel;

void printfPanel(panel *target){
    char *open;
    char *hasNested;
    char *nested;
    if(target->nested != NULL){
        nested = "Not NULL";
    }else{
        nested = "NULL";
    }
    if(target->open == true){
        open = "true";
    }else{
        open = "false";
    }
    if(target->hasNested == true){
        hasNested = "true";
    }else{
        hasNested = "false";
    }
    char *info = "-- panel --\nname: %s\nopen: %s\nhasNested: %s\nnested: %s\ntopLeft row, col: %i, %i\nsize rows, cols: %i, %i\n-- end --\n";
    printf(info, target->name, open, hasNested, nested, target->topLeft.row, target->topLeft.column, target->size.rows, target->size.columns);
}

struct {
    bool quit;
    struct termios originalTerminalSettings;
    struct termios newTerminalSettings;
    char *cwd;
    struct {
        int columns;
        int lines;
    } terminalSize;
    struct {
        char *background;
        char *text;
        char *panelBorder;
        char *menuTitleText;
        char *menuTitleBackground;
    } colours;
    struct {
        panel root;
    } panels;
} applicationState;

typedef struct {
    char keyPress;
    char *displayText;
    void (*action)(void);
} menuOption;

typedef struct {
    menuOption *options;
    size_t length;
    size_t maxLength;
} menuOptions;

int getStringLength(char *string){
    int i = 0;
    char character = ' ';
    while(character != '\0'){
        character = string[i];
        i++;
    }
    return i-1;
}

menuOption createMenuOption(char keyPress, char *message, void (*action)(void)){
    int textLength = getStringLength(message);
    char *template = " [%c] - %s";
    int templateLength = getStringLength(template);
    char displayText[textLength + templateLength + 2];
    snprintf(displayText, textLength + templateLength + 2, template, keyPress, message);
    menuOption returnValue;
    returnValue.keyPress = keyPress;
    returnValue.displayText = displayText;
    returnValue.action = action;
    return returnValue;
}

menuOptions createMenuOptionsArrayStruct(size_t maxSize){
    menuOptions returnValue;
    returnValue.options = malloc(sizeof(menuOption) * maxSize);
    returnValue.length = 0;
    returnValue.maxLength = maxSize;
    return returnValue;
}

void freeMenuOptionsArray(menuOptions *menu){
    free(menu->options);
    menu->options = NULL;
    menu->length = 0;
    menu->maxLength = 0;
}

void addOptionToMenuArr(menuOptions *menu, menuOption option){
    if(menu->length < menu->maxLength){
        menu->options[menu->length] = option;
        menu->length++;
    }
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

char *MALLconcatenateStrings(char *one, char *two){
    int lengthOne = getStringLength(one);
    int lengthTwo = getStringLength(two);
    char *returnValue = malloc(lengthOne + lengthTwo + 1);
    int i = 0;
    while(i < lengthOne){
        returnValue[i] = one[i];
        i++;
    }
    while(i < lengthOne + lengthTwo){
        returnValue[i] = two[i - lengthOne];
        i++;
    }
    returnValue[lengthOne + lengthTwo] = '\0';
    return returnValue;
}

void emptyAndFreeString(char *string){
    int i = 0;
    char character = ' ';
    while(character != '\0'){
        character = string[i];
        string[i] = 0;
        i++;
    }
    free(string);
}

void addPanelBodyTextLine(panel *target, char *line){
    target->printArea[target->size.rows - 1] = line;
    target->size.rows++;
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

void setUpTerminal(void){
    tcgetattr(STDIN_FILENO, &applicationState.originalTerminalSettings);
    tcgetattr(STDIN_FILENO, &applicationState.newTerminalSettings);
    applicationState.newTerminalSettings.c_lflag &= ~(ICANON |ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &applicationState.newTerminalSettings);
}

void restoreOriginalTerminalSettings(void){
    tcsetattr(STDIN_FILENO, TCSANOW, &applicationState.originalTerminalSettings);
}

void clearScreen(void){
    printf("\033[2J");
}

void showAltScreenBuffer(void){
    setUpTerminal();
    printf("\033[?1049h");
    fflush(stdout);
}

void exitAltScreenBuffer(void){
    clearScreen();
    restoreOriginalTerminalSettings();
    printf("\033[?1049l");
    fflush(stdout);
}

void stayInAltScreenBufferUnlessQ(void){
    printf("-- cwd: %s\n", applicationState.cwd);
    char character;
    while(applicationState.quit == false){
        printf("This is the alternate screen buffer. Press 'q' to exit.\n");
        character = getchar();
        if(character == 'q'){
            applicationState.quit = true;
            printf("\nThank you. Now leaving...\n");
            fflush(stdout);
        }else{
            printf("Thank you. You pressed: '%c', (%d)", character, character);
        }
    }
    restoreOriginalTerminalSettings();
}

struct winsize winSize;

void getWinDimensions(int eventNumber){
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &winSize);
    applicationState.terminalSize.lines = winSize.ws_row;
    applicationState.terminalSize.columns = winSize.ws_col;
}

char* padBothSides(char *string){
//    printf("-- padBothSides --\n");
//    printf("string parameter: %s\n", string);
    int stringLength = getStringLength(string);
//    printf("string parameter length: %i\n", stringLength);
    int columns = applicationState.terminalSize.columns;
//    printf("number of columns: %i\n", columns);
    int prepend;
    int append;
    int i = 0;
    if((columns - stringLength) % 2 == 0){
        prepend = (columns - stringLength) / 2;
        append = (columns - stringLength) / 2;
    }else{
        prepend = ((columns - stringLength) + 1) / 2;
        append = ((columns - stringLength) - 1) / 2;
    }
//    printf("prepend length: %i\n", prepend);
//    printf("append length: %i\n", append);
    char *returnValue = malloc(columns + 1);
    while(i < prepend){
        returnValue[i] = ' ';
        i++;
    }
    while(i < (prepend + stringLength)){
        returnValue[i] = string[i - prepend];
        i++;
    }
    while(i < columns){
        returnValue[i] = ' ';
        i++;
    }
//    printf("return string length: %i\n", getStringLength(returnValue));
    returnValue[columns] = '\0';
//    printf("-- End padBothSides --\n");
    return returnValue;
}

void centreText(int panelWidth, char *text){
    int textLength = getStringLength(text);
    int sizeBefore;
    int sizeAfter;
    if((panelWidth - textLength) % 2 == 0){
        sizeBefore = (panelWidth - textLength) / 2;
        sizeAfter = sizeBefore;
    }else{
        sizeBefore = (panelWidth - textLength - 1) / 2;
        sizeAfter = (panelWidth - textLength + 1) / 2;
    }
    int textLineLength = sizeBefore + textLength + sizeAfter;
    char textLine[textLineLength + 1];
    int i = 0;
    while(i < sizeBefore){
        textLine[i] = ' ';
        i++;
    }
    while(i < sizeBefore + textLength){
        textLine[i] = text[i - sizeBefore];
        i++;
    }
    while(i < textLineLength){
        textLine[i] = ' ';
        i++;
    }
    textLine[textLineLength] = '\0';
    printf("%s", textLine);
}

char *MALLfullLine(void){
    char *returnValue = malloc(applicationState.terminalSize.columns + 1);
    int i = 0;
    while(i < applicationState.terminalSize.columns){
        returnValue[i] = '-';
        i++;
    }
    returnValue[applicationState.terminalSize.columns] = '\0';
    return returnValue;
}

char *MALLwrapText(char *prepend, char *text, char *append){
    char *prependPlusText = MALLconcatenateStrings(prepend, text);
    char *returnValue = MALLconcatenateStrings(prependPlusText, append);
    emptyAndFreeString(prependPlusText);
    return returnValue;
}

void applyAppColours(void){
    printf("\033[48;2;%sm", applicationState.colours.background);
    printf("\033[38;2;%sm", applicationState.colours.text);
}

char *MALLredText(char *text){
    return MALLwrapText("\033[31m", text, "\033[0m");
}

void redText(void){
    printf("\033[31m");
}

void borderColours(){
    printf("\033[48;2;%sm", applicationState.colours.panelBorder);
}

void menuTitleColours(){
    printf("\033[48;2;%sm", applicationState.colours.menuTitleBackground);
    printf("\033[38;2;%sm", applicationState.colours.menuTitleText);
}

void cursorGoTo(int line, int character){
    printf("\033[%i;%iH", line, character);
}

void printHorizBorder(panel *nester){
    cursorGoTo(nester->topLeft.row + nester->size.rows, nester->topLeft.column);
    borderColours();
    int i = 0;
    while(i < nester->size.columns){
        printf("%c", ' ');
        i++;
    }
    applyAppColours();
}

void printAndFreeLine(char *line){
    printf("%s\n", line);
    free(line);
}

void addMenuOptionsToPanel(panel *target, menuOptions *menu){
    for(int i = 0; i < menu->length; i++){
        addPanelBodyTextLine(target, menu->options[i].displayText);
    }
}

void testMenuAction(void){
    printf("Called a test menu action!\n");
}

void quitMenu(void){
    applicationState.quit = true;
}

void getMenuChoice(menuOptions *options){
    bool done = false;
    char choice;
    while(done == false){
        choice = getchar();
        for(int i = 0; i < options->length; i++){
            if(choice == options->options[i].keyPress){
                done = true;
                options->options[i].action();
            }
        }
    }
}

void menu(menuOptions options){
    applicationState.quit = false;
    while(applicationState.quit == false){
//        clearScreen();
//        printMenuOptions(options);
        getMenuChoice(&options);
    }
    applicationState.quit = false;
}

void testAction(void){
    printf("Test action ran!\n");
}

char *quitIsTrue(void){
    if(applicationState.quit == true){
        return "applicationState.quit: 'TRUE'";
    }else{
        return "applicationState.quit: 'FALSE'";
    }
}

panel *findLastPanel(void){
    bool found = false;
    panel *current = &applicationState.panels.root;
    while(found == false){
        if(current->hasNested == true){
            current = current->nested;
        }else{
            found = true;
        }
    }
    return current;
}

panel *createNewPanel(char *name){
    panel *new = malloc(sizeof(panel));
    new->name = name;
    new->open = true;
    new->hasNested = false;
    new->nested = NULL;
    new->size.rows = 1;
    return new;
}

void emptyAndFreePanel(panel *oldPanel){
    memset(oldPanel, 0, sizeof(panel));
    free(oldPanel);
}

void insertPanelBelow(panel *nester, panel *nested){
    nested->topLeft.row = nester->topLeft.row + nester->size.rows + 1;
    nested->topLeft.column = nester->topLeft.column;
    nested->size.columns = nester->size.columns;
    nester->nested = nested;
    nester->hasNested = true;
}

void renderPanel(panel *target){
    cursorGoTo(target->topLeft.row, target->topLeft.column);
    menuTitleColours();
    centreText(target->size.columns, target->name);
    applyAppColours();
    cursorGoTo(target->topLeft.row + 1, target->topLeft.column);
    int i = 0;
    while(i < target->size.rows - 1){
        printf("%s", target->printArea[i]);
        cursorGoTo(target->topLeft.row + 1 + i, target->topLeft.column);
        i++;
    }
    if(target->hasNested == true){
        printHorizBorder(target);
    }
}

void renderAllPanels(){
    applicationState.panels.root.size.columns = applicationState.terminalSize.columns;
    if(applicationState.panels.root.hasNested == true){
        bool done = false;
        panel *currentPanel = applicationState.panels.root.nested;
        while(done == false){
            renderPanel(currentPanel);
            if(currentPanel->hasNested == true){
                currentPanel = currentPanel->nested;
            }else{
                done = true;
            }
        }
    }
}

void storePanelTopLeft(panel *targetPanel){
    
}

void cursorDown(int rows){
    printf("\033[%iB", rows);
}

void cursorToPanelTopLeft(panel *currentPanel){
    cursorGoTo(currentPanel->topLeft.row, currentPanel->topLeft.column);
}

void mainMenuPanel(void){
    panel *mainMenu = createNewPanel("main menu");
    insertPanelBelow(findLastPanel(), mainMenu);
    menuOptions mainMenuOptions = createMenuOptionsArrayStruct(12);
    addOptionToMenuArr(&mainMenuOptions, createMenuOption('c', "choose a file", testAction));
    addOptionToMenuArr(&mainMenuOptions, createMenuOption('q', "leave this menu", quitMenu));
    addMenuOptionsToPanel(mainMenu, &mainMenuOptions);
    getchar();
//    menuOptions myMenu = createMenuOptionsArrayStruct(3);
//    addOptionToMenuArr(&myMenu, createMenuOption('a', "this is a test menu option", testAction));
//    addOptionToMenuArr(&myMenu, createMenuOption('b', "second test menu option", testAction));
//    addOptionToMenuArr(&myMenu, createMenuOption('q', "exit menu test option", quitMenu));
//    menu(myMenu);
}

void titlePanel(void){
    clearScreen();
    panel *title = createNewPanel("tui touch type");
    insertPanelBelow(findLastPanel(), title);
    addPanelBodyTextLine(title, "Practice your touch typing skills here...");
    mainMenuPanel();
}

void run(void){
    titlePanel();
    renderAllPanels();
    getchar();
}

void setupApplicationState(void){
    applicationState.quit = false;
    applicationState.cwd = getcwd(NULL, 0);
    applicationState.colours.background = "50;50;50";
    applicationState.colours.text = "210;210;210";
    applicationState.colours.panelBorder = "25;25;25";
    applicationState.colours.menuTitleText = "250;250;250";
    applicationState.colours.menuTitleBackground = "100;100;100";
    getWinDimensions(1);
    applyAppColours();
    applicationState.panels.root.open = true;
    applicationState.panels.root.topLeft.row = 1;
    applicationState.panels.root.topLeft.column = 1;
    applicationState.panels.root.size.rows = -1;
    applicationState.panels.root.size.columns = applicationState.terminalSize.columns;
    applicationState.panels.root.name = "root";
}

int main(int argc, char *argv[]) {
    if(checkArgs(argc, argv) == 1){
        return 1;
    }
    signal(SIGWINCH, getWinDimensions);
    showAltScreenBuffer();
    setupApplicationState();
    clearScreen();
    printf("in alt-screen-buffer; press any key to continue...\n");
    getchar();
    run();
    printf("all finished; press any key to exit...\n");
    getchar();
    exitAltScreenBuffer();
    return 0;
}
