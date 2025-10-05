#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <ctype.h>

#define BLUE "\033[38;2;91;206;250m"
#define PINK "\033[38;2;255;105;180m"
#define WHITE "\033[38;2;255;255;255m"
#define RESET "\033[0m"
#define DATE_TEXT_COLOR WHITE

#define DEFAULT_FLAG_SIZE "small"
#define DEFAULT_FLAG_CHAR "█"
#define DEFAULT_REQ_WIDTH -1
#define DEFAULT_REQ_HEIGHT -1
#define DEFAULT_AUTO_CENTER 0
#define DEFAULT_BORDER_ENABLED 0
#define DEFAULT_BORDER_CHAR "*"
#define DEFAULT_BORDER_COLOR "\033[38;2;180;180;180m"
#define DEFAULT_MESSAGE_TEXT ""
#define DEFAULT_MESSAGE_COLOR PINK
#define DEFAULT_DATE_ENABLED 0
#define DEFAULT_DATE_FORMAT "%Y-%m-%d %H:%M"
#define DEFAULT_VERTICAL_ENABLED 0

#define MAX_LINE_WIDTH 1024
#define MAX_MESSAGE_LEN 256
#define MAX_FORMAT_LEN 128
#define MAX_PATH_LEN 512

typedef struct {
    char flag_size[16];
    char flag_char[8];
    int req_width;
    int req_height;
    int auto_center;
    int border_enabled;
    char border_char[8];
    char border_color[64];
    char message_text[MAX_MESSAGE_LEN];
    char message_color[64];
    int date_enabled;
    char date_format[MAX_FORMAT_LEN];
    int vertical_enabled;
} Config;

typedef struct {
    int blue1, pink1, white, pink2, blue2;
} StripeHeights;

void print_usage(const char *prog) {
    printf("Usage: %s [options] [tiny|small|medium|big|huge|banner]\n", prog);
    printf("Draws the transgender pride flag in the terminal.\n\n");
    printf("Options:\n");
    printf("  -w W, --width=W         Set specific width (positive integer).\n");
    printf("  -he H, --height=H       Set specific height (positive integer, min 5).\n");
    printf("  -c C, --char=C          Use character C for drawing (default: '█').\n");
    printf("  -C, --center            Center the flag in the terminal.\n");
    printf("      --border            Enable a border around the flag.\n");
    printf("      --border-char CH    Set specific border character.\n");
    printf("      --border-color-esc ESC Set ANSI escape code for border color.\n");
    printf("  -m TEXT, --message=TEXT Display TEXT on the flag.\n");
    printf("      --message-color-esc ESC Set ANSI escape code for message color.\n");
    printf("      --date[=FORMAT]     Show current date/time in corner.\n");
    printf("      --vertical          Rotate the flag 90 degrees.\n");
    printf("  -h, --help              Show this help message and exit.\n\n");
    printf("Size Presets:\n");
    printf("  tiny, small, medium, big, huge, banner\n");
}

void init_config(Config *cfg) {
    strncpy(cfg->flag_size, DEFAULT_FLAG_SIZE, sizeof(cfg->flag_size) - 1);
    strncpy(cfg->flag_char, DEFAULT_FLAG_CHAR, sizeof(cfg->flag_char) - 1);
    cfg->req_width = DEFAULT_REQ_WIDTH;
    cfg->req_height = DEFAULT_REQ_HEIGHT;
    cfg->auto_center = DEFAULT_AUTO_CENTER;
    cfg->border_enabled = DEFAULT_BORDER_ENABLED;
    strncpy(cfg->border_char, DEFAULT_BORDER_CHAR, sizeof(cfg->border_char) - 1);
    strncpy(cfg->border_color, DEFAULT_BORDER_COLOR, sizeof(cfg->border_color) - 1);
    strncpy(cfg->message_text, DEFAULT_MESSAGE_TEXT, sizeof(cfg->message_text) - 1);
    strncpy(cfg->message_color, DEFAULT_MESSAGE_COLOR, sizeof(cfg->message_color) - 1);
    cfg->date_enabled = DEFAULT_DATE_ENABLED;
    strncpy(cfg->date_format, DEFAULT_DATE_FORMAT, sizeof(cfg->date_format) - 1);
    cfg->vertical_enabled = DEFAULT_VERTICAL_ENABLED;
}

int get_terminal_size(int *cols, int *rows) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 0;
    }
    *cols = w.ws_col;
    *rows = w.ws_row;
    return 1;
}

StripeHeights calculate_stripes(int total_dim) {
    StripeHeights heights;
    int p_blue1 = 8, p_pink1 = 6, p_white = 6, p_pink2 = 6, p_blue2 = 8;
    int total_p = p_blue1 + p_pink1 + p_white + p_pink2 + p_blue2;

    if (total_dim < 5) total_dim = 5;

    heights.blue1 = total_dim * p_blue1 / total_p;
    heights.pink1 = total_dim * p_pink1 / total_p;
    heights.white = total_dim * p_white / total_p;
    heights.pink2 = total_dim * p_pink2 / total_p;
    heights.blue2 = total_dim * p_blue2 / total_p;

    if (heights.blue1 < 1) heights.blue1 = 1;
    if (heights.pink1 < 1) heights.pink1 = 1;
    if (heights.white < 1) heights.white = 1;
    if (heights.pink2 < 1) heights.pink2 = 1;
    if (heights.blue2 < 1) heights.blue2 = 1;

    int current_sum = heights.blue1 + heights.pink1 + heights.white +
                      heights.pink2 + heights.blue2;
    int remainder = total_dim - current_sum;

    while (remainder > 0) {
        heights.blue1++; remainder--; if (remainder == 0) break;
        heights.blue2++; remainder--; if (remainder == 0) break;
        heights.pink1++; remainder--; if (remainder == 0) break;
        heights.pink2++; remainder--; if (remainder == 0) break;
        heights.white++; remainder--;
    }

    while (remainder < 0) {
        if (heights.white > 1) { heights.white--; remainder++; }
        else if (heights.pink1 > 1) { heights.pink1--; remainder++; }
        else if (heights.pink2 > 1) { heights.pink2--; remainder++; }
        else if (heights.blue1 > 1) { heights.blue1--; remainder++; }
        else if (heights.blue2 > 1) { heights.blue2--; remainder++; }
        else break;
    }

    return heights;
}

void draw_horizontal_flag(Config *cfg, int width, int height, int check_height_warning) {
    int term_cols = 80, term_rows = 24;
    get_terminal_size(&term_cols, &term_rows);

    int effective_height = height + (cfg->border_enabled ? 2 : 0);
    int effective_width = width + (cfg->border_enabled ? 2 : 0);

    if (check_height_warning && term_rows < effective_height) {
        printf("%swarning: your terminal (height: %d) might be too small for the flag (height: %d).%s\n",
               WHITE, term_rows, effective_height, RESET);
        printf("proceed anyway? [y/N] ");
        char response = getchar();
        if (response != 'y' && response != 'Y') {
            printf("operation cancelled.\n");
            return;
        }
        while (getchar() != '\n');
    }

    char *block_line = malloc(width * strlen(cfg->flag_char) + 1);
    block_line[0] = '\0';
    for (int i = 0; i < width; i++) {
        strcat(block_line, cfg->flag_char);
    }

    int padding = 0;
    if (cfg->auto_center && term_cols > effective_width) {
        padding = (term_cols - effective_width) / 2;
    }

    char date_str[64] = "";
    int date_len = 0;
    if (cfg->date_enabled) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(date_str, sizeof(date_str), cfg->date_format, t);
        date_len = strlen(date_str);
    }

    StripeHeights stripes = calculate_stripes(height);
    int message_line = -1;
    int message_len = strlen(cfg->message_text);
    if (message_len > 0 && stripes.white > 0) {
        message_line = stripes.blue1 + stripes.pink1 + stripes.white / 2;
    }

    if (cfg->border_enabled) {
        for (int i = 0; i < padding; i++) printf(" ");
        printf("%s", cfg->border_color);
        for (int i = 0; i < effective_width; i++) printf("%s", cfg->border_char);
        printf("%s\n", RESET);
    }

    int line_idx = 0;
    const char *colors[] = {BLUE, PINK, WHITE, PINK, BLUE};
    int heights[] = {stripes.blue1, stripes.pink1, stripes.white, stripes.pink2, stripes.blue2};

    for (int stripe = 0; stripe < 5; stripe++) {
        for (int i = 0; i < heights[stripe]; i++) {
            for (int p = 0; p < padding; p++) printf(" ");

            if (cfg->border_enabled) printf("%s%s", cfg->border_color, cfg->border_char);

            printf("%s", colors[stripe]);

            if (cfg->date_enabled && line_idx == 0 && date_len > 0) {
                int prefix_len = width - date_len;
                if (prefix_len > 0) {
                    for (int j = 0; j < prefix_len; j++) printf("%s", cfg->flag_char);
                }
                printf("%s%s%s", DATE_TEXT_COLOR, date_str, colors[stripe]);
            } else if (message_len > 0 && line_idx == message_line) {
                int msg_padding = (width - message_len) / 2;
                for (int j = 0; j < msg_padding; j++) printf("%s", cfg->flag_char);
                printf("%s%s%s", cfg->message_color, cfg->message_text, colors[stripe]);
                for (int j = msg_padding + message_len; j < width; j++) printf("%s", cfg->flag_char);
            } else {
                printf("%s", block_line);
            }

            if (cfg->border_enabled) printf("%s%s", cfg->border_color, cfg->border_char);
            printf("%s\n", RESET);
            line_idx++;
        }
    }

    if (cfg->border_enabled) {
        for (int i = 0; i < padding; i++) printf(" ");
        printf("%s", cfg->border_color);
        for (int i = 0; i < effective_width; i++) printf("%s", cfg->border_char);
        printf("%s\n", RESET);
    }

    free(block_line);
}

void draw_vertical_flag(Config *cfg, int width, int height, int check_height_warning) {
    int term_cols = 80, term_rows = 24;
    get_terminal_size(&term_cols, &term_rows);

    int effective_height = height + (cfg->border_enabled ? 2 : 0);
    int effective_width = width + (cfg->border_enabled ? 2 : 0);

    if (check_height_warning && term_rows < effective_height) {
        printf("%swarning: your terminal (height: %d) might be too small for the flag (height: %d).%s\n",
               WHITE, term_rows, effective_height, RESET);
        printf("proceed anyway? [y/N] ");
        char response = getchar();
        if (response != 'y' && response != 'Y') {
            printf("operation cancelled.\n");
            return;
        }
        while (getchar() != '\n');
    }

    int padding = 0;
    if (cfg->auto_center && term_cols > effective_width) {
        padding = (term_cols - effective_width) / 2;
    }

    char date_str[64] = "";
    int date_len = 0;
    if (cfg->date_enabled) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(date_str, sizeof(date_str), cfg->date_format, t);
        date_len = strlen(date_str);
    }

    StripeHeights stripes = calculate_stripes(width);
    int message_line = height / 2;
    int message_len = strlen(cfg->message_text);

    if (cfg->border_enabled) {
        for (int i = 0; i < padding; i++) printf(" ");
        printf("%s", cfg->border_color);
        for (int i = 0; i < effective_width; i++) printf("%s", cfg->border_char);
        printf("%s\n", RESET);
    }

    const char *colors[] = {BLUE, PINK, WHITE, PINK, BLUE};
    int widths[] = {stripes.blue1, stripes.pink1, stripes.white, stripes.pink2, stripes.blue2};

    for (int row = 0; row < height; row++) {
        for (int p = 0; p < padding; p++) printf(" ");
        if (cfg->border_enabled) printf("%s%s", cfg->border_color, cfg->border_char);

        int col_idx = 0;
        for (int stripe = 0; stripe < 5; stripe++) {
            for (int i = 0; i < widths[stripe]; i++) {
                if (message_len > 0 && row == message_line) {
                    int msg_start = (width - message_len) / 2;
                    if (col_idx >= msg_start && col_idx < msg_start + message_len) {
                        printf("%s%c", cfg->message_color, cfg->message_text[col_idx - msg_start]);
                        col_idx++;
                        continue;
                    }
                }

                if (cfg->date_enabled && row == 0 && col_idx < date_len) {
                    printf("%s%c", DATE_TEXT_COLOR, date_str[col_idx]);
                } else {
                    printf("%s%s", colors[stripe], cfg->flag_char);
                }
                col_idx++;
            }
        }

        if (cfg->border_enabled) printf("%s%s", cfg->border_color, cfg->border_char);
        printf("%s\n", RESET);
    }

    if (cfg->border_enabled) {
        for (int i = 0; i < padding; i++) printf(" ");
        printf("%s", cfg->border_color);
        for (int i = 0; i < effective_width; i++) printf("%s", cfg->border_char);
        printf("%s\n", RESET);
    }
}

int main(int argc, char *argv[]) {
    Config cfg;
    init_config(&cfg);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            if (i + 1 < argc) cfg.req_width = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--width=", 8) == 0) {
            cfg.req_width = atoi(argv[i] + 8);
        } else if (strcmp(argv[i], "-he") == 0 || strcmp(argv[i], "--height") == 0) {
            if (i + 1 < argc) cfg.req_height = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--height=", 9) == 0) {
            cfg.req_height = atoi(argv[i] + 9);
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--char") == 0) {
            if (i + 1 < argc) strncpy(cfg.flag_char, argv[++i], sizeof(cfg.flag_char) - 1);
        } else if (strncmp(argv[i], "--char=", 7) == 0) {
            strncpy(cfg.flag_char, argv[i] + 7, sizeof(cfg.flag_char) - 1);
        } else if (strcmp(argv[i], "-C") == 0 || strcmp(argv[i], "--center") == 0) {
            cfg.auto_center = 1;
        } else if (strcmp(argv[i], "--border") == 0) {
            cfg.border_enabled = 1;
        } else if (strcmp(argv[i], "--border-char") == 0) {
            if (i + 1 < argc) {
                strncpy(cfg.border_char, argv[++i], sizeof(cfg.border_char) - 1);
                cfg.border_enabled = 1;
            }
        } else if (strcmp(argv[i], "--border-color-esc") == 0) {
            if (i + 1 < argc) {
                strncpy(cfg.border_color, argv[++i], sizeof(cfg.border_color) - 1);
                cfg.border_enabled = 1;
            }
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--message") == 0) {
            if (i + 1 < argc) strncpy(cfg.message_text, argv[++i], sizeof(cfg.message_text) - 1);
        } else if (strncmp(argv[i], "--message=", 10) == 0) {
            strncpy(cfg.message_text, argv[i] + 10, sizeof(cfg.message_text) - 1);
        } else if (strcmp(argv[i], "--message-color-esc") == 0) {
            if (i + 1 < argc) strncpy(cfg.message_color, argv[++i], sizeof(cfg.message_color) - 1);
        } else if (strcmp(argv[i], "--date") == 0) {
            cfg.date_enabled = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                strncpy(cfg.date_format, argv[++i], sizeof(cfg.date_format) - 1);
            }
        } else if (strncmp(argv[i], "--date=", 7) == 0) {
            cfg.date_enabled = 1;
            if (strlen(argv[i]) > 7) {
                strncpy(cfg.date_format, argv[i] + 7, sizeof(cfg.date_format) - 1);
            }
        } else if (strcmp(argv[i], "--vertical") == 0) {
            cfg.vertical_enabled = 1;
        } else if (strcmp(argv[i], "tiny") == 0 || strcmp(argv[i], "small") == 0 ||
                   strcmp(argv[i], "medium") == 0 || strcmp(argv[i], "big") == 0 ||
                   strcmp(argv[i], "huge") == 0 || strcmp(argv[i], "banner") == 0) {
            strncpy(cfg.flag_size, argv[i], sizeof(cfg.flag_size) - 1);
        }
    }

    int final_width = cfg.req_width;
    int final_height = cfg.req_height;
    int check_height = 0;

    if (final_width <= 0) {
        if (strcmp(cfg.flag_size, "tiny") == 0) final_width = 20;
        else if (strcmp(cfg.flag_size, "small") == 0) final_width = 39;
        else if (strcmp(cfg.flag_size, "medium") == 0) final_width = 42;
        else if (strcmp(cfg.flag_size, "big") == 0) final_width = 48;
        else if (strcmp(cfg.flag_size, "huge") == 0 || strcmp(cfg.flag_size, "banner") == 0) {
            int cols, rows;
            if (get_terminal_size(&cols, &rows)) {
                final_width = cols - (cfg.border_enabled ? 2 : 0);
            } else {
                final_width = 80;
            }
        } else {
            final_width = 39;
        }
    }

    if (final_height <= 0) {
        if (strcmp(cfg.flag_size, "tiny") == 0) final_height = 5;
        else if (strcmp(cfg.flag_size, "small") == 0) final_height = 12;
        else if (strcmp(cfg.flag_size, "medium") == 0) final_height = 15;
        else if (strcmp(cfg.flag_size, "big") == 0) final_height = 17;
        else if (strcmp(cfg.flag_size, "banner") == 0) final_height = 15;
        else if (strcmp(cfg.flag_size, "huge") == 0) {
            final_height = 34;
            check_height = 1;
        } else {
            final_height = 12;
        }
    }

    if (final_width < 1) final_width = 1;
    if (final_height < 5) final_height = 5;
    if (cfg.vertical_enabled && final_width < 5) final_width = 5;

    if (cfg.vertical_enabled) {
        draw_vertical_flag(&cfg, final_width, final_height, check_height);
    } else {
        draw_horizontal_flag(&cfg, final_width, final_height, check_height);
    }

    return 0;
}
