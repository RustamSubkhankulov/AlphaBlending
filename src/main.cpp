#include "global_conf.h"
#include "alphablending/alpha_b.h"

//===============================================

int main(int argc, char* argv[])
{
    #ifdef LOGS

        FILE* logs_file = open_log_file(Logfile_name);
        if (!logs_file)
            return -1;

    #endif 

    ARGC_CHECK(argc);

    const char* backg_pict = argv[1];
    const char* patch_pict = argv[2];
    const char* reslt_name = argv[3];
    const int x_patch_pos = atoi(argv[4]);
    const int y_patch_pos = atoi(argv[5]);

    int return_value =  
        overlay_pict(backg_pict, patch_pict, reslt_name, x_patch_pos, y_patch_pos);

    if (return_value == -1)
        return -1;

    #ifdef LOGS
        close_log_file();
    #endif 

    return 0;
}