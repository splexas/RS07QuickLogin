#include <stdio.h>
#include <windows.h>

struct game_args {
    DWORD* target_process_id;
    HWND result;
};

int get_credentials(char* filename, char* credentials[][2], int* found);

int open_game(char* path, PROCESS_INFORMATION* process_info);
static BOOL CALLBACK enum_window_callback(HWND hwnd, LPARAM lparam);
void get_game_window(DWORD* process_id, HWND* root_window, HWND* game_window);

void setup(HWND root_window, HWND game_window);

void press_char(HWND root_window, HWND game_window, char* c);
void write(HWND root_window, HWND game_window, char* text);
void press_enter(HWND root_window, HWND game_window);

void login(HWND root_window, HWND game_window, char* username, char* password);

/* configurable delays */

#define GAME_WAIT_MS 2000
#define MAIN_WINDOW_WAIT_MS 4000
#define LOGIN_WINDOW_WAIT_MS 250

/*

getting valid hwnd:

    HWND root_window = FindWindowA("JagWindow", "Old School RuneScape");
    HWND game_window = FindWindowExA(root_window, NULL, "JagRenderView", NULL);

*/

int main(int argc, char** argv) {

    /* using SendMessage here instead of PostMessage to be more stable in pressing keys and be sure about valid inputs. */

    if (argc < 2) {
        fprintf_s(stderr, "Not enough arguments! Provide the OSRS game file path. Usage: %s <game_path>\n", argv[0]);
        return 1;
    }

    char* game_path = argv[1];

    char* credentials[1024][2];
    int credentials_found = 0;

    if (get_credentials("accounts.txt", credentials, &credentials_found) != 0) {
        fprintf_s(stderr, "Could not get any credentials.\n");
        return 1;
    }

    fprintf_s(stdout, "Found %d credentials.\n", credentials_found);

    for (int i = 0; i < credentials_found; i++) {
        PROCESS_INFORMATION process_info = {0};
        if (open_game(game_path, &process_info) != 0) {
            fprintf_s(stdout, "Failed to open the game. Aborting process.\n");
            return 1;
        }

        Sleep(GAME_WAIT_MS); // wait for the game to load until it creates the windows

        HWND root_window = NULL;
        HWND game_window = NULL;

        get_game_window(&process_info.dwProcessId, &root_window, &game_window);
        ShowWindow(root_window, SW_RESTORE);
        Sleep(MAIN_WINDOW_WAIT_MS); // wait for the game entry to load
        login(root_window, game_window, credentials[i][0], credentials[i][1]);
        ShowWindow(root_window, SW_FORCEMINIMIZE); // minimize to avoid other clients' popping up when they become logged in
    }

    return 0;
}

int get_credentials(char* filename, char* credentials[][2], int* found) {
    FILE* f = fopen(filename, "r");
    if (f == NULL)
        return 1;

    static char buf[1024];

    char ch;    
    int i = 0;

    do {
        ch = fgetc(f);
        if (ch != EOF) {
            buf[i] = ch;
            i++;
        }

    } while (ch != EOF);

    fclose(f);

    char* line_ctx = NULL;
    char* line = strtok_s(buf, "\n", &line_ctx);

    while (line != NULL) {
        char* cred_ctx = NULL;
        char* username = strtok_s(line, ":", &cred_ctx);
        if (username == NULL)
            break;

        char* password = strtok_s(NULL, "\0", &cred_ctx);
        if (password == NULL)
            break;

        credentials[*found][0] = username;
        credentials[*found][1] = password;

        (*found)++;
        line = strtok_s(NULL, "\n", &line_ctx);
    }

    return 0;
}

int open_game(char* path, PROCESS_INFORMATION* process_info) {
    STARTUPINFOA startup_info = {0};
    BOOL status = CreateProcessA(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startup_info, process_info);
    return status != 1;
}

static BOOL CALLBACK enum_window_callback(HWND hwnd, LPARAM lparam) {
    struct game_args* g_args = (struct game_args*)lparam;

    DWORD process_id;
    if (GetWindowThreadProcessId(hwnd, &process_id) == 0) {
        return TRUE;
    }

    char buffer[128];
    if (GetWindowTextA(hwnd, buffer, sizeof(buffer)) == 0) {
        return TRUE;
    }

    if (strcmp(buffer, "Old School RuneScape") == 0 && process_id == *g_args->target_process_id) {
        g_args->result = hwnd;
        return FALSE;
    }

    return TRUE;
}

void get_game_window(DWORD* process_id, HWND* root_window, HWND* game_window) {
    struct game_args g_args = {0};
    g_args.target_process_id = process_id;

    EnumWindows(enum_window_callback, (LPARAM)&g_args);

    *root_window = g_args.result;
    *game_window = FindWindowExA(g_args.result, NULL, "JagRenderView", NULL);
}

void setup(HWND root_window, HWND game_window) {
    if (GetForegroundWindow() != root_window)
        SetForegroundWindow(root_window);

    SetFocus(game_window); // set keyboard focus
}

void press_char(HWND root_window, HWND game_window, char* c) {
    setup(root_window, game_window);

    LPARAM lp = 1;  // Repeat count
    lp |= ((DWORD)MapVirtualKey(*c, MAPVK_VK_TO_VSC) << 16);  // Scan code
    lp |= (1 << 24);  // Extended key flag

    LRESULT res;
    
    do {
        res = SendMessageW(game_window, WM_CHAR, *c, lp);
        if (res == 0)
            setup(root_window, game_window);

    } while (res != 1);

}

void write(HWND root_window, HWND game_window, char* text) {
    while (*text != 0) {
        press_char(root_window, game_window, text);
        text++;
    }
}

void press_enter(HWND root_window, HWND game_window) {
    setup(root_window, game_window);

    LRESULT res;

    do {
        res = SendMessageW(game_window, WM_KEYDOWN, VK_RETURN, 0);
        if (res == 0)
            setup(root_window, game_window);

    } while (res != 1);

    do { 
        res = SendMessageW(game_window, WM_KEYUP, 0, 0);
        if (res == 0)
            setup(root_window, game_window);

    } while (res != 1);
}

void login(HWND root_window, HWND game_window, char* username, char* password) {
    /* get into login window */
    press_enter(root_window, game_window);
    Sleep(LOGIN_WINDOW_WAIT_MS); // wait for it to load

    /* write username */
    write(root_window, game_window, username);

    /* get to password field */
    press_enter(root_window, game_window);

    /* write password */
    write(root_window, game_window, password);

    /* login! */
    press_enter(root_window, game_window);
}