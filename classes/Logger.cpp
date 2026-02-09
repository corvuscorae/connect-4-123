#include "Logger.h"

Logger* Logger::instance = nullptr;

void Logger::ToggleConsoleLog(bool b)
{
    this->to_console_enabled = b;
}

void Logger::WriteLogToFile(const std::string &_filename, bool show_info, bool show_warn, bool show_error)
{
    // OPEN FILE
    if (this->file.is_open())
    {
        this->file.close();
    }

    this->filename = _filename;
    this->file.open(this->filename, std::ios::out);

    // WRITE
    if (this->file.is_open())
    {
        for (int i = 0; i < this->log.size(); i++)
        {
            LogItem item = this->log.at(i);

            // skip deselected levels
            if(!show_info && item.level == level_text[INFO]){
                continue;
            }
            if(!show_warn && item.level == level_text[WARN]){
                continue;
            }
            if(!show_error && item.level == level_text[ERROR]){
                continue;
            }
            this->file << item.print() << "\n";
        }
        this->file.flush();
    }
}

void Logger::Log(const char *message, int lvl, int type){
    LogItem new_item(level_text[lvl], message, color[lvl], type_text[type]);
    this->log.push_back(new_item);

    if (this->to_console_enabled)
    {
        std::cout << new_item.print() << std::endl;
    }

    this->log_size++;
}

void Logger::Log(char *message, int lvl, int type){
    LogItem new_item(level_text[lvl], message, color[lvl], type_text[type]);
    this->log.push_back(new_item);

    if (this->to_console_enabled)
    {
        std::cout << new_item.print() << std::endl;
    }

    this->log_size++;
}

void Logger::Log(std::string message, int lvl, int type){
    LogItem new_item(level_text[lvl], message, color[lvl], type_text[type]);
    this->log.push_back(new_item);

    if (this->to_console_enabled)
    {
        std::cout << new_item.print() << std::endl;
    }

    this->log_size++;
}

void Logger::clear()
{
    this->log.clear();
    this->log_size = 0;
}

LogItem Logger::get(int i)
{
    return this->log.at(i);
}

std::string Logger::print_last()
{
    return this->log.back().print();
}

std::string Logger::print(int i)
{
    return this->log.at(i).print();
}

void Logger::initUI()
{
    //-- GAME CONTROL WINDOW --//
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGui::Begin("Game Control", &show_game_panel, ImGuiWindowFlags_MenuBar);

        ImGui::Text("This is the game control panel");

        // MENU
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("log options"))
            {
                // show which levels?
                if (ImGui::BeginMenu("show"))
                {
                    ImGui::MenuItem("info", "", &show_info);
                    ImGui::MenuItem("warn", "", &show_warn);
                    ImGui::MenuItem("error", "", &show_error);

                    ImGui::EndMenu();
                }

                // print levels to...?
                if(ImGui::MenuItem("console", "", &log_to_console)){
                    ToggleConsoleLog(log_to_console);
                }
                if (ImGui::MenuItem("file", "")) { 
                    WriteLogToFile("game_log.txt", show_info, show_warn, show_error);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // BUTTONS
        if (ImGui::Button("Log Event"))
        {
            Log("This is a test event.", INFO, GAME);
        }
        ImGui::SameLine();
        if (ImGui::Button("Log Game Warn"))
        {
            Log("This is a test game warning.", WARN, GAME);
        }
        ImGui::SameLine();
        if (ImGui::Button("Log Game Error"))
        {
            Log("This is a test game error.", ERROR, GAME);
        }

        // FRAMERATE
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        // TOGGLE WINDOWS
        ImGui::Checkbox("Game Log", &show_game_log);
        // ImGui::Checkbox("Demo Window", &show_imgui_demo);      
        // ImGui::Checkbox("Imgui Log Demo", &show_log_demo);

        ImGui::End();

    //--- GAME LOG WINDOW ---//
        if(show_game_log){
            ImGui::Begin("Game Log", &show_game_log);

            // BUTTONS
            if (ImGui::Button("Clear"))
            {
                clear(); // ???
            }
            ImGui::SameLine();
            if (ImGui::Button("Test Info"))
            {
                Log("This is test info.");
            }
            ImGui::SameLine();
            if (ImGui::Button("Test Warn"))
            {
                Log("This is a test warning.", WARN);
            }
            ImGui::SameLine();
            if (ImGui::Button("Test Error"))
            {
                Log("This is a test error.", ERROR);
            }

            // PRINT LOG ITEMS
            if(log_to_console){
                for (int i = 0; i < log_size; i++)
                {
                    LogItem item = get(i);

                    // skip deselected levels
                    if(!show_info && item.level == level_text[INFO]){
                        continue;
                    }
                    if(!show_warn && item.level == level_text[WARN]){
                        continue;
                    }
                    if(!show_error && item.level == level_text[ERROR]){
                        continue;
                    }

                    ImVec4 text_color = item.color;
                    ImGui::TextColored(text_color, print(i).c_str());
                }
            }

            ImGui::End();

    }

}