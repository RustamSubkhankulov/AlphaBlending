#include <new>

//===============================================

#include "alpha_b.h"
#include "../general/general.h"

//===============================================

int _overlay_pict(const char* backg_pict,  const char* patch_pict, const char* reslt_name,
                  const int   x_patch_pos, const int   y_patch_pos FOR_LOGS(, LOG_PARAMS))
{
    alpha_b_log_report();

    FILENAME_CHECK(backg_pict);
    FILENAME_CHECK(patch_pict);
    FILENAME_CHECK(reslt_name);

    //printf("backg name %s x %u y %u \n", backg_pict, backg_size.x, backg_size.y);
    //printf("patch name %s x %u y %u \n", patch_pict, patch_size.x, patch_size.y);

    int is_loaded = 0;
    sf::Image backg;
    sf::Image patch;

    is_loaded = backg.loadFromFile(backg_pict);
    if (is_loaded == 0) return -1;

    is_loaded = patch.loadFromFile(patch_pict);
    if (is_loaded == 0) return -1;

    const unsigned char* patch_norm = normalize_patch_pict(backg, patch, 
                                              x_patch_pos, y_patch_pos);
    if (!patch_norm)
        return -1;

    unsigned char* reslt_data = overlay_eval(&backg, patch_norm);

    sf::Vector2u backg_size_vector = backg.getSize();

    sf::Image reslt;
    reslt.create(backg_size_vector.x, backg_size_vector.y, reslt_data); 
    reslt.saveToFile(reslt_name);

    int return_value = show_reslt_in_window(&reslt);
    if (return_value == -1)
        return -1;

    free(reslt_data);
    free((void*) patch_norm);

    return 0;
}

//===============================================

const unsigned char* _normalize_patch_pict(sf::Image backg, sf::Image patch, const int x_pos, const int y_pos FOR_LOGS(,LOG_PARAMS))
{
    alpha_b_log_report();

    sf::Vector2u backg_size = backg.getSize();
    sf::Vector2u patch_size = patch.getSize();

    unsigned int x_bckg = backg_size.x; 
    unsigned int y_bckg = backg_size.y;
  
    unsigned int x_ptch = patch_size.x;
    unsigned int y_ptch = patch_size.y;

    if (x_ptch > x_bckg || y_ptch > y_bckg)
    {
        error_report(INV_PATCH_PICT_SIZE);
        return NULL;
    }

    if (x_ptch + x_pos > x_bckg || y_ptch + y_pos > y_bckg)
    {
        error_report(INV_PATCH_PICT_POS);
        return NULL;
    }

    const unsigned char* patch_data = patch.getPixelsPtr();

    unsigned char* patch_norm = (unsigned char*) aligned_alloc(32, x_bckg * y_bckg * 4);
    if (!patch_norm)
        return NULL;

    for (int y_ct = 0; y_ct < y_ptch; y_ct++)
    {
        int x_ct = 0;
        int norm_offset = 4 * ( (y_pos + y_ct) * x_bckg + x_pos + x_ct );
        int ptch_offset = 4 * (  y_ct          * x_ptch +         x_ct );

        for (; x_ptch - x_ct >= 8; x_ct += 8)                     
        {
            norm_offset += 32;
            ptch_offset += 32;

            __m256i data = _mm256_loadu_si256( (__m256i*) &patch_data[ptch_offset]);
            _mm256_storeu_si256( (__m256i*) &patch_norm[norm_offset], data); 
        }

                                                                             // complete
        for (; x_ct < x_ptch; x_ct++, ptch_offset++, norm_offset++)
            patch_norm[norm_offset] = patch_data[ptch_offset];
    }    

    return patch_norm;
}

//===============================================

int _show_reslt_in_window(sf::Image* reslt FOR_LOGS(, LOG_PARAMS))
{
    alpha_b_log_report();
    assert(reslt);

    sf::Vector2u window_size_vector = (*reslt).getSize();
    int window_size_x = window_size_vector.x;
    int window_size_y = window_size_vector.y;

    sf::Texture texture;
    texture.create(window_size_x, window_size_y);
    texture.update(*reslt);

    sf::Sprite sprite;
    sprite.setTexture(texture);

    sf::RenderWindow window(sf::VideoMode(window_size_x, window_size_y), "AlphaBlender");

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }

    return 0;
}

//===============================================

unsigned char* _overlay_eval(sf::Image* backg, const unsigned char* patch_data FOR_LOGS(, LOG_PARAMS))
{
    alpha_b_log_report();

    assert(backg);
    assert(patch_data);

    sf::Vector2u backg_size_vector = (*backg).getSize();
    int backg_size = backg_size_vector.x * backg_size_vector.y;


    #ifdef DEBUG
    //
        printf("\n backg_size %d \n", backg_size);
    //
    #endif 

    const sf::Uint8* backg_data = (*backg).getPixelsPtr();

    //
    #ifdef DEBUG 

        FILE* bckg_file = fopen("bckg.txt", "w");
        for (int ct = 1; ct <= backg_size * 4; ct++)
        {
            fprintf(bckg_file, "%02x ", backg_data[ct]);
            if ((ct % 64) == 0)
                fprintf(bckg_file, "\n");
            fflush(stdout);
        }
        fclose(bckg_file);
        //

        //
        FILE* ptch_file = fopen("ptch.txt", "w");
        for (int ct = 1; ct <= backg_size * 4; ct++)
        {
            fprintf(ptch_file, "%02x ", patch_data[ct]);
            if ((ct % 64) == 0)
                fprintf(ptch_file, "\n");
            fflush(stdout);
        }
        fclose(ptch_file);
        //

    #endif 

    //unsigned char* reslt_data = new (std::align_val_t(16)) unsigned char[backg_size * 4];
    unsigned char* reslt_data = (unsigned char*) aligned_alloc(16, backg_size * 4);
    if (!reslt_data)
    {
        error_report(CANNOT_ALLOCATE_MEM);
        return NULL;;
    }

    //
    #ifdef DEBUG 
        printf("\n reslt_data %p \n", reslt_data);
    #endif 
    //
    //

    int x_size = backg_size_vector.x;
    int y_size = backg_size_vector.y;

    //
    #ifdef DEBUG
        printf("\n bckg x %d y %d \n", x_size, y_size);
    #endif 
    //

    #ifdef DEBUG
        FILE* evals = fopen("evals.txt", "w");
    #endif

    #ifdef OPT 

        printf("\n я не упал до альфа блендера \n");
        fflush(stdout);

        const __m128i vector_char_00    = _mm_set1_epi8 (0);
        const __m128i vector_short_00FF = _mm_set1_epi16(0x00FF);
        
        const unsigned char Zero = 128; 

        for (int y = 0; y < y_size; y++)
        {
            int x = 0;
            int offst = 4 * (x + y * x_size);

            for (; x < x_size; x += 4, offst += 16)
            {
                // load values from patch_data (foreground) to ptch and 
                // and from backg_data (backgroud) to bckg
                // each contains 4 * 4  bytes 
                // A3R3G3B3 A2R2G2B2 A1R1G1B1 A0R0G0B0 

                //-----------------------------------------------------------------------
                //         15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
                // ptch = [a3 r3 g3 b3 | a2 r2 g2 b2 | a1 r1 g1 b1 | a0 r0 g0 b0]
                //-----------------------------------------------------------------------

                //int offst = 4 * ( x + y * x_size );

                //
                #ifdef DEBUG 
                    fprintf(evals, "\n cur x: %d cyr y: %d \n", x, y);

                    fprintf(evals, "\n ptch: ");
                    for (int i = 1; i <= 16; i++)
                    {
                        fprintf(evals, "%02x", patch_data[offst + i - 1]);
                        if ((i % 4) == 0)
                            fprintf(evals, " ");
                    }
                    fprintf(evals, "\n");
                    

                    fprintf(evals, "\n bckg: ");
                    for (int i = 1; i <= 16; i++)
                    {
                        fprintf(evals, "%02x", backg_data[offst + i - 1]);
                        if ((i % 4) == 0)
                            fprintf(evals, " ");
                    }
                    fprintf(evals, "\n");
                #endif 
                //

                //printf("\n ptch_offst %d bckg_offst %d \n", ptch_offst, bckg_offst);
                
                __m128i ptch = _mm_loadu_si128 ( (__m128i*) ( &patch_data[offst] ) );
                __m128i bckg = _mm_loadu_si128 ( (__m128i*) ( &backg_data[offst] ) );

                // move high 8 bytes from each to lower bytes of PTCH and BCKG
                // fill high 8 bytes of PTC and BCKG with zeros 

                //-----------------------------------------------------------------------
                //          15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
                //  ptch = [a3 r3 g3 b3 | a2 r2 g2 b2 | a1 r1 g1 b1 | a0 r0 g0 b0]
                //           \  \  \  \    \  \  \  \   xx xx xx xx   xx xx xx xx            
                //            \  \  \  \    \  \  \  \.
                //             \  \  \  \    '--+--+--+-------------+--+--+--.
                //              '--+--+--+------------+--+--+--.     \  \  \  \.
                //                                     \  \  \  \     \  \  \  \.
                //   PTCH = [00 00 00 00 | 00 00 00 00 | a3 r3 g3 b3 | a2 r2 g2 b2]
                //-----------------------------------------------------------------------

                __m128i PTCH = (__m128i) _mm_movehl_ps( (__m128) vector_char_00, (__m128) ptch );
                __m128i BCKG = (__m128i) _mm_movehl_ps( (__m128) vector_char_00, (__m128) bckg );

                // store lower 8 bytes of each vector in whole vector 
                // with __ between bytes  

                //-----------------------------------------------------------------------
                //           15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
                // ptch = [a3 r3 g3 b3 | a2 r2 g2 b2 | a1 r1 g1 b1 | a0 r0 g0 b0]
                //         xx xx xx xx   xx xx xx xx                 /  /   |  |
                //                                           _______/  /   /   |
                //              ...   ...     ...           /     ____/   /    |
                //             /     /       /             /     /       /     |
                // ptch = [00 a1 00 r1 | 00 g1 00 b1 | 00 a0 00 r0 | 00 g0 00 b0]
                //-----------------------------------------------------------------------

                ptch = _mm_cvtepu8_epi16 ( ptch );
                bckg = _mm_cvtepu8_epi16 ( bckg );
    
                PTCH = _mm_cvtepu8_epi16 ( PTCH );
                BCKG = _mm_cvtepu8_epi16 ( BCKG );

                //
                //
                //
                #ifdef DEBUG 

                    alignas(16) unsigned char char_array[16] = { 0 };
                    _mm_store_si128 ( (__m128i*) ( char_array ), ptch);

                    fprintf(evals, "\n ptch: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    //----------

                    _mm_store_si128 ( (__m128i*) ( char_array ), PTCH);

                    fprintf(evals, "\n PTCH: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    //----------

                    _mm_store_si128 ( (__m128i*) ( char_array ), bckg);

                    fprintf(evals, "\n bckg: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    _mm_store_si128 ( (__m128i*) ( char_array ), BCKG);

                    //----------

                    fprintf(evals, "\n BCKG: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                #endif 

                //
                //
                //

                // Sshuffle bytes as in picture 

                //-----------------------------------------------------------------------
                //           15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
                // ptch   = [00 a1 00 r1 | 00 g1 00 b1 | 00 a0 00 r0 | 00 g0 00 b0]
                //              |___________________        |___________________
                //              |     \      \      \       |     \      \      \.
                // ptch_a = [00 a1 00 a1 | 00 a1 00 a1 | 00 a0 00 a0 | 00 a0 00 a0]
                //-----------------------------------------------------------------------

                //-----------------------------------------------------------------------
                //           15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
                // PTCH   = [00 a3 00 r3 | 00 g3 00 b3 | 00 a2 00 r2 | 00 g2 00 b2]
                //              |___________________        |___________________
                //              |     \      \      \       |     \      \      \.
                // PTCH_A = [00 a3 00 a3 | 00 a3 00 a3 | 00 a2 00 a2 | 00 a2 00 a2]
                //-----------------------------------------------------------------------

                // make mask 
                // 6  - src is 6th  char value of dest
                // 14 - src is 14th char value of dest

                __m128i movemask = _mm_set_epi8( Zero, 14, Zero, 14, Zero, 14, Zero, 14,
                                                 Zero,  6, Zero,  6, Zero,  6, Zero,  6);

                //shuffle every vector

                __m128i ptch_a = _mm_shuffle_epi8 ( ptch, movemask ); 
                __m128i PTCH_A = _mm_shuffle_epi8 ( PTCH, movemask );
                //__m128i bckg_a = _mm_shuffle_epi8 ( bckg, movemask );
                //__m128i BCKG_A = _mm_shuffle_epi8 ( BCKG, movemask );

                //
                //
                //

                //alignas(16) char char_array[16] = { 0 };
                #ifdef DEBUG 

                    _mm_store_si128 ( (__m128i*) ( char_array ), ptch_a);

                    fprintf(evals, "\n ptch_a: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    //----------

                    _mm_store_si128 ( (__m128i*) ( char_array ), PTCH_A);

                    fprintf(evals, "\n PTCH_A: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");
                #endif 

                //----------

                //
                //
                //

                // mul values of ARGB for each pixel of patch picture
                // by value of A in ARGB for this pixel

                ptch = _mm_mullo_epi16 ( ptch, ptch_a );
                PTCH = _mm_mullo_epi16 ( PTCH, PTCH_A );

                // for background multiplying by (255 - a)

                bckg = _mm_mullo_epi16 ( bckg, _mm_sub_epi16 ( vector_short_00FF, ptch_a ) );
                BCKG = _mm_mullo_epi16 ( BCKG, _mm_sub_epi16 ( vector_short_00FF, PTCH_A ) );


                //
                //
                //

                //alignas(16) char char_array[16] = { 0 };
                #ifdef DEBUG 
                    _mm_store_si128 ( (__m128i*) ( char_array ), ptch);

                    fprintf(evals, "\n ptch * TF: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    //----------

                    _mm_store_si128 ( (__m128i*) ( char_array ), PTCH);

                    fprintf(evals, "\n PTCH * TF: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    //----------

                    _mm_store_si128 ( (__m128i*) ( char_array ), bckg);

                    fprintf(evals, "\n bckg * TB: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    _mm_store_si128 ( (__m128i*) ( char_array ), BCKG);

                    //----------

                    fprintf(evals, "\n BCKG * TB: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");
                #endif 

                //
                //
                //

                // sum - sum of multiplied values of 0 and 1 pixels
                // SUM - sum of multiplied values of 2 and 3 pixels

                __m128i sum  = _mm_add_epi16 ( ptch, bckg );
                __m128i SUM  = _mm_add_epi16 ( PTCH, BCKG );


                //
                //
                //

                //alignas(16) char char_array[16] = { 0 };
                #ifdef DEBUG 

                    _mm_store_si128 ( (__m128i*) ( char_array ), sum);

                    fprintf(evals, "\n sum: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    //----------

                    _mm_store_si128 ( (__m128i*) ( char_array ), SUM);

                    fprintf(evals, "\n SUM: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");
                #endif 

                //----------

                //
                //
                //

                //Pack values in lower 64 bits as in graph 

                //-----------------------------------------------------------------------
                //        15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
                // sum = [A1 a1 R1 r1 | G1 g1 B1 b1 | A0 a0 R0 r0 | G0 g0 B0 b0]
                //         \     \       \     \       \_____\_______\_____\.
                //          \_____\_______\_____\______________    \  \  \  \.
                //                                    \  \  \  \    \  \  \  \.
                // sum = [00 00 00 00 | 00 00 00 00 | A1 R1 G1 B1 | A0 R0 G0 B0]
                //-----------------------------------------------------------------------

                //-----------------------------------------------------------------------
                //        15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
                // SUM = [A3 a3 R3 r3 | G3 g3 B3 b3 | A2 a2 R2 r2 | G2 g2 B2 b2]
                //         \     \       \     \       \_____\_______\_____\.
                //          \_____\_______\_____\______________    \  \  \  \.
                //                                    \  \  \  \    \  \  \  \.
                // SUM = [00 00 00 00 | 00 00 00 00 | A3 R3 G3 B3 | A2 R2 G2 B2]
                //-----------------------------------------------------------------------

                __m128i packmask = _mm_set_epi8 ( Zero, Zero, Zero, Zero, Zero, Zero, Zero, Zero, 
                                                    15,   13,   11,    9,    7,    5,    3,    1);

                //shuffle sum and SUM

                sum = _mm_shuffle_epi8 ( sum, packmask );
                SUM = _mm_shuffle_epi8 ( SUM, packmask );

                //
                //
                //

                //alignas(16) char char_array[16] = { 0 };
                #ifdef DEBUG 
                    _mm_store_si128 ( (__m128i*) ( char_array ), sum);

                    fprintf(evals, "\n sum after shuffle: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");

                    //----------

                    _mm_store_si128 ( (__m128i*) ( char_array ), SUM);

                    fprintf(evals, "\n SUM after shuffle: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", char_array[ct]);
                    }

                    fprintf(evals, "\n");
                #endif 

                //
                //

                //Move values from two vectors - sum and SUM - into one

                //-----------------------------------------------------------------------
                //          15 14 13 12   11 10  9  8    7  6  5  4    3  2  1  0
                // sum   = [-- -- -- -- | -- -- -- -- | A1 R1 G1 B1 | A0 R0 G0 B0] ->-.
                // SUM   = [-- -- -- -- | -- -- -- -- | A3 R3 G3 B3 | A2 R2 G2 B2]    |
                //                                      /  /  /  /    /  /  /  /      V
                //             .--+--+--+----+--+--+--++--+--+--+----+--+--+--'       |
                //            /  /  /  /    /  /  /  /    ____________________________/
                //           /  /  /  /    /  /  /  /    /  /  /  /    /  /  /  /
                // color = [A3 R3 G3 B3 | A2 R2 G2 B2 | A1 R1 G1 B1 | A0 R0 G0 B0]
                //-----------------------------------------------------------------------

                __m128i colors = (__m128i) _mm_movelh_ps( (__m128) sum, (__m128) SUM );

                //Store in array reslt

                //printf("\n i'm alive before store  x %d y %d  bckg_offst %d \n", x, y, bckg_offst);
                //fflush(stdout);

                _mm_store_si128 ( (__m128i*) ( &reslt_data[offst] ), colors );

                //
                #ifdef DEBUG 

                    alignas(16) int arr_colors[16] = { 0 };
                    _mm_store_si128 ( (__m128i*) ( arr_colors), colors);

                    fprintf(evals, "\n colors: ");

                    for (int ct = 0; ct < 16; ct++)
                    {
                        fprintf(evals, "%02x ", arr_colors[ct]);
                    }

                    fprintf(evals, "\n \n \n \n \n ");

                #endif 
                //

                //printf("\n i'm alive after store  x %d y %d \n", x, y);
                //fflush(stdout);
            }
        }

    //
    #ifdef DEBUG 

        FILE* temp= fopen("reslt.txt", "w");

        for (int ct = 1; ct <= backg_size * 4; ct++)
        {
            fprintf(temp, "%02x ", reslt_data[ct]);
            if ((ct % 64) == 0)
                fprintf(temp, "\n");
            fflush(stdout);
        }

        fclose(temp);
        fclose(evals);

    #endif 
    //

    //===============================================================

    #else 

    for (int y = 0; y < y_size; y++)
    {
        for (int x = 0; x < x_size; x++)
        {
            
        }
    }

    #endif 

    return reslt_data;
}