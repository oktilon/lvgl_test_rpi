/*******************************************************************
 *
 * main.c - LVGL test for GNU/Linux on Raspberry Pi
 *
 ******************************************************************/
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include "lvgl/lvgl.h"

typedef enum app_modes_enum {
    unknown = -1,
    defaultMode = 0,
    dispHelp,
    dispVer,
    dispRainbow,
    solidFill
} appMode;


/* Internal functions */
static appMode parseArguments(int argc, char ** argv);
static void printVersion(void);
static void printHelp(char *cmd);

unsigned long userColor = 0;
const char * devicePath = "/dev/fb0";
int rainbowSize = 200;
int rainbowDir = -1;
uint32_t rainbow[7] = {
    0xff0000,
    0xffa500,
    0xffff00,
    0x00ff00,
    0x00ffff,
    0x0000ff,
    0xff00ff
};

/**
 * @brief Print versions
 */
static void printVersion(void) {
    printf("%d.%d.%d\n", 0, 0, 0);
    printf("LVGL: %d.%d.%d-%s\n",
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH,
            LVGL_VERSION_INFO);
}

/**
 * @brief Print help
 */
static void printHelp(char *cmd) {
    printf("\n%s [-s hex_color] [-r] [-V] [-h]\n\n",
        cmd
    );
    printf("-s   fill screen with solid color\n");
    printf("-r   fill screen with rainbow\n");
    printf("-V   print versions\n");
    printf("-h   show this help\n");
}

/**
 * @brief Configure simulator
 * @description process arguments received by the program to select
 * appropriate options
 * @param argc the count of arguments in argv
 * @param argv The arguments
 * @returns int mode
 */
static appMode parseArguments(int argc, char ** argv) {
    int opt = 0;
    appMode mode = defaultMode;

    /* Parse the command-line options. */
    while((opt = getopt(argc, argv, "s:rVh")) != -1) {
        switch(opt) {
            case 'h':
                mode = dispHelp;
                break;
            case 'V':
                mode = dispVer;
                break;
            case 'r':
                mode = dispRainbow;
                break;
            case 's':
                userColor = strtoul(optarg, NULL, 16);
                mode = solidFill;
                break;
            case ':':
                mode = unknown;
                printf("Option -%c requires an argument.\n", optopt);
                break;
            case '?':
                mode = unknown;
                printf("Unknown option -%c.\n", optopt);
                break;
        }
    }

    return mode;
}

static void areaTimerCallback(lv_timer_t *timer) {
    lv_obj_invalidate((lv_obj_t *) lv_timer_get_user_data(timer));
    if(rainbowDir < 0) rainbowSize--;
    else rainbowSize++;
    if(rainbowSize == 200) rainbowDir = -1;
    else if(rainbowSize == 100) rainbowDir = 1;
}

static void areaDrawCallback(lv_event_t *e) {
    // lv_obj_t *area = lv_event_get_target_obj(e);
    lv_draw_task_t * drawTask = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t * baseDsc = (lv_draw_dsc_base_t *) lv_draw_task_get_draw_dsc(drawTask);
    if(baseDsc->part == LV_PART_MAIN) {
        lv_draw_rect_dsc_t drawDsc;
        lv_draw_rect_dsc_init(&drawDsc);
        int wd = rainbowSize / 7;
        for (int i = 0; i < 7; i++) {
            drawDsc.bg_color = lv_color_hex(rainbow[i]);
            lv_area_t a = {i * wd, 0, i * wd + wd, rainbowSize};
            // lv_area_t objCoords;
            // lv_obj_get_coords(area, &objCoords);
            // lv_area_align(&objCoords, &a, LV_ALIGN_CENTER, 0, 0);
            lv_draw_rect(baseDsc->layer, &drawDsc, &a);
        }
    }
}

/**
 * @brief entry point
 * @description start a demo
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
int main(int argc, char ** argv) {
    uint32_t idle_time;
    lv_display_t * disp;
    appMode mode;

    mode = parseArguments(argc, argv);
    if (mode == dispHelp) {
        printHelp(argv[0]);
        return EXIT_SUCCESS;
    } else if (mode == dispVer) {
        printVersion();
        return EXIT_SUCCESS;
    } else if (mode == unknown) {
        return EINVAL;
    }

    /* Initialize LVGL. */
    lv_init();

    disp = lv_linux_fbdev_create();
    if(disp == NULL) {
        printf("Init display error\n");
        return EIO;
    }

    lv_linux_fbdev_set_file(disp, devicePath);

    lv_obj_t *screen = lv_screen_active();

    if (mode == dispRainbow) {
        lv_obj_t *area = lv_obj_create(screen);
        lv_obj_set_size(area, 200, 200);
        lv_obj_center(area);
        lv_obj_add_event_cb(area, areaDrawCallback, LV_EVENT_DRAW_TASK_ADDED, NULL);
        lv_obj_add_flag(area, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
        lv_timer_create(areaTimerCallback, 30, area); // 30 ms
    } else if (mode == solidFill) {
        uint32_t color = (uint32_t)(userColor & 0xFFFFFF);
        lv_obj_set_style_bg_color(screen, lv_color_hex(color), LV_PART_MAIN);
    } else { // defaultMode
        uint32_t color = (uint32_t)(userColor & 0x000000);
        lv_obj_set_style_bg_color(screen, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_t *btn = lv_button_create(screen);
        lv_obj_set_size(btn, lv_pct(60), lv_pct(20));
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, "First text");
        // lv_obj_set_style_text_color(lbl, lv_color_hex(0x00FF00), 0);
    }

    /* Enter the run loop of the selected backend */
    while(true) {
        /* Returns the time to the next timer execution */
        idle_time = 1; //lv_timer_handler();
        usleep(idle_time * 1000);
    }

    return EXIT_SUCCESS;
}
