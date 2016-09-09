#define CMD_EXACT true
#define CMD_BEGIN false
#ifndef MenuHandler_h
#define MenuHandler_h
#include <Arduino.h>
#include <LinkedList.h>



typedef void (*MenuHandlerFunction)(const char *line);
class MenuEntry {
  public:
    MenuEntry(const __FlashStringHelper *_cmd, bool _cmdExactMatch, MenuHandlerFunction _handler, const __FlashStringHelper *_description) :
      cmd(_cmd), cmdExactMatch(_cmdExactMatch), handler(_handler), description(_description) {};

    const __FlashStringHelper *cmd;
    bool cmdExactMatch;
    MenuHandlerFunction handler;
    const __FlashStringHelper *description;
};

class MenuHandler {
  public:
    MenuHandler();
    //id setup(MenuHandler *handler){};
    void loop();
    void registerCommand(MenuEntry *command);
    void listCommands();
    void scheduleCommand(const char *cmd);
    void handleCommand(const char *line);

  private:
    bool processUserInput();
    static void cmdHelp(const char *ignore);
    void processCommands();

    byte readLine(int);
    static const int LINE_LEN=250;
    char line[LINE_LEN];
    LinkedList<MenuEntry*> *commands;
    LinkedList<String*> pendingCommands = LinkedList<String*>();

};

#endif
