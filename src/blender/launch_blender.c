#include <Windows.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    // "C:\\Program Files\\Blender Foundation\\Blender 4.4\\blender.exe" --python "C:\\Users\\nihal\\Documents\\chizen\\projects\\blender_addon\\chizen.py"

    char cmd_line[1024];
    if (argc == 3)
    {
        strcat(cmd_line, argv[1]);
        strcat(cmd_line, " --python ");
        strcat(cmd_line, argv[2]);
    }
    else {
        printf("ERROR: Please pass correct number of parameters");
        return 0;
    }

    STARTUPINFOA startup_info = {0};
    PROCESS_INFORMATION process_information = {0};

    if (!CreateProcessA(NULL, cmd_line, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &startup_info, &process_information))
    {
        char buffer[1024];
        const char *msg = "Could not launch blender process";
        sprintf(buffer, "%s: %d", msg, GetLastError());
        printf("%s\n", buffer);
        return 1;
    }

    return 0;
}