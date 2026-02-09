#pragma once

#include "../imgui/imgui.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <vector>
#include <cstring>
#include <string>

struct LogItem
{
    std::string level;
    std::string dat;
    std::string type;
    ImVec4 color;

    LogItem(const char *_level, const char *_dat, ImVec4 _col, const char *_type = NULL)
    {
        level = std::string(_level);
        dat = std::string(_dat);
        if (_type)
        {
            type = std::string(_type);
        }
        else
        {
            type = "";
        }
        color = _col;
    }

    LogItem(const char *_level, std::string _dat, ImVec4 _col, std::string _type = "")
    {
        level = _level;
        dat = _dat;
        type = _type;
        color = _col;
    }

    std::string print()
    {
        std::string message = "[" + level + "] ";

        if (type != "")
        {
            message += "[" + type + "] ";
        }

        message += dat;

        return message;
    }
};

// singleton pattern class for logging
class Logger
{
private:
    std::vector<LogItem> log;
    std::ofstream file;
    std::string filename;
    static Logger *instance;

    // UI
    bool show_game_panel = true;
    bool show_game_log = true;

    // logging options
    bool log_to_console = true;
    bool log_to_file = false;
    bool show_info = true;
    bool show_warn = true;
    bool show_error = true;

    Logger() {};

public:
    int log_size = 0;
    bool to_console_enabled = true;

    enum types
    {
        DEFAULT,
        GAME
    };
    const char *type_text[2] = {
        NULL,
        "GAME"};

    enum level
    {
        INFO,
        WARN,
        ERROR
    };
    const char *level_text[3] = {
        "INFO",
        "WARN",
        "ERROR"};
    const ImVec4 color[3] = {
        ImVec4(1, 1, 1, 1),
        ImVec4(1, 1, 0, 1),
        ImVec4(1, 0, 0, 1)};

    static Logger *GetInstance()
    {
        // (improved this function based on feedback from Graham)
        if (instance == nullptr)
        {
            instance = new Logger();
        }
        return instance;
    }

    // FUNCTIONS
    void ToggleConsoleLog(bool b);
    void WriteLogToFile(
        const std::string &_filename = "game_log.txt",
        bool show_info = true,
        bool show_warn = true,
        bool show_error = true);
    void Log(const char *message, int lvl = 0, int type = NULL);
    void Log(char *message, int lvl = 0, int type = NULL);
    void Log(std::string message, int lvl = 0, int type = NULL);
    void clear();
    LogItem get(int i);
    std::string print_last();
    std::string print(int i);

    void initUI();
};
