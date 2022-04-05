#pragma once 

#include "alpha_b_conf.h"

//===============================================

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <immintrin.h>
#include <emmintrin.h>


//===============================================

#include "../logs/errors_and_logs.h"
#include "alpha_b_conf.h"

//===============================================

#ifdef ALPHA_B_LOGS 

    #define alpha_b_log_report() \
            log_report();

#else 

    #define alpha_b_log_report() ""

#endif 

//-----------------------------------------------

#define ARGC_CHECK(arcg)                        \
                                                \
    do                                          \
    {                                           \
        if (arcg != Args_num)                   \
        {                                       \
            printf("\n Not enough arguments."   \
                              "Terminating \n");\
            return -1;                          \
        }                                       \
    } while(0);          

//===============================================

union Color
{
    unsigned int value;
    unsigned char bgra[4];
};

//===============================================

int _overlay_pict(const char* backg_pict, 
                  const char* patch_pict,
                  const char* reslt_name,
                  const int  x_patch_pos,
                  const int  y_patch_pos FOR_LOGS(, LOG_PARAMS));

int _show_reslt_in_window(sf::Image* reslt, float time FOR_LOGS(, LOG_PARAMS));

int _show_passed_time(float time FOR_LOGS(, LOG_PARAMS));

float _overlay_eval(sf::Image* backg, 
                   const unsigned char* patch_data, 
                         unsigned char* reslt_data FOR_LOGS(, LOG_PARAMS));

const unsigned char* _normalize_patch_pict(sf::Image backg, sf::Image patch, 
                                           const int x_pos, const int y_pos FOR_LOGS(,LOG_PARAMS));

int _save_and_show_result(sf::Vector2u size_vector, unsigned char* reslt_data, 
                          const char* reslt_name, float time FOR_LOGS(,LOG_PARAMS));

unsigned char* _allocate_reslt_data(sf::Image backg FOR_LOGS(, LOG_PARAMS));

//===============================================

#define allocate_reslt_data(backg) \
       _allocate_reslt_data(backg FOR_LOGS(, LOG_ARGS))

#define show_passed_time(time) \
       _show_passed_time(time FOR_LOGS(, LOG_ARGS))

#define save_and_show_result(size_vector, reslt_data, reslt_name, time ) \
       _save_and_show_result(size_vector, reslt_data, reslt_name, time FOR_LOGS(,LOG_ARGS))

#define normalize_patch_pict(backg, patch, x_pos, y_pos) \
       _normalize_patch_pict(backg, patch, x_pos, y_pos FOR_LOGS(, LOG_ARGS))

#define show_reslt_in_window(reslt, time) \
       _show_reslt_in_window(reslt, time FOR_LOGS(, LOG_ARGS))

#define overlay_eval(backg, patch_data, reslt_data) \
       _overlay_eval(backg, patch_data, reslt_data FOR_LOGS(, LOG_ARGS))

#define overlay_pict(backg, patch, reslt, x_pos, y_pos) \
       _overlay_pict(backg, patch, reslt, x_pos, y_pos FOR_LOGS(, LOG_ARGS))

//===============================================