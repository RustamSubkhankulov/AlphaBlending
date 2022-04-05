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

    unsigned char* reslt_data = allocate_reslt_data(backg);
    if (!reslt_data)
        return -1;

    float time_passed = overlay_eval(&backg, patch_norm, reslt_data);

    printf("\n passed time %f \n", time_passed);

    save_and_show_result(backg.getSize(), reslt_data, reslt_name, time_passed);

    free((void*) reslt_data);
    free((void*) patch_norm);

    return 0;
}

//===============================================

unsigned char* _allocate_reslt_data(sf::Image backg FOR_LOGS(, LOG_PARAMS))
{
    alpha_b_log_report();

    sf::Vector2u size_vector = backg.getSize();

    return (unsigned char*) aligned_alloc(32, size_vector.x * size_vector.y * 4);
}

//===============================================

int _save_and_show_result(sf::Vector2u size_vector, unsigned char* reslt_data, 
                          const char* reslt_name, float time FOR_LOGS(,LOG_PARAMS))
{
    alpha_b_log_report();

    assert(reslt_data);
    assert(reslt_name);

    sf::Image reslt;
    reslt.create(size_vector.x, size_vector.y, reslt_data);
    reslt.saveToFile(reslt_name);

    int return_value = show_reslt_in_window(&reslt, time);
    if (return_value == -1)
        return -1;

    return 0;
}

//===============================================

const unsigned char* _normalize_patch_pict(sf::Image backg, sf::Image patch, 
                                           const int x_pos, const int y_pos FOR_LOGS(,LOG_PARAMS))
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

int _show_reslt_in_window(sf::Image* reslt, float time FOR_LOGS(, LOG_PARAMS))
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

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::T)
                {
                    show_passed_time(time);
                }
            }
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }

    return 0;
}

//===============================================

int _show_passed_time(float time FOR_LOGS(, LOG_PARAMS))
{
    alpha_b_log_report();

    sf::Font font;
    font.loadFromFile(Font_name);

    sf::Text passed_time_text;

    passed_time_text.setFont(font);
    passed_time_text.setFillColor(Text_color);
    passed_time_text.setCharacterSize(CharacterSize);

    char text_buf[Text_buf_size] = { 0 };
    sprintf(text_buf, "NUMBER OF CYCLES: %d\nTIME: %f", Evaluations_number, time);
    passed_time_text.setString(text_buf);

    sf::RenderWindow window(sf::VideoMode(Time_wndw_width, Time_wndw_height), "INFO");

    while(window.isOpen())
    {
        sf::Event event;
        while(window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

        }

        window.clear();
        window.draw(passed_time_text);
        window.display();
    }

    return 0;
}

//===============================================

float _overlay_eval(sf::Image* backg, const unsigned char* patch_data, 
                                            unsigned char* reslt_data FOR_LOGS(, LOG_PARAMS))
{
    alpha_b_log_report();

    assert(backg);
    assert(patch_data);

    sf::Vector2u backg_size_vector = (*backg).getSize();
    int backg_size = backg_size_vector.x * backg_size_vector.y;

    const sf::Uint8* backg_data = (*backg).getPixelsPtr();

    int x_size = backg_size_vector.x;
    int y_size = backg_size_vector.y;

    sf::Clock clock;

    #ifdef OPT 

        const __m128i vector_char_00    = _mm_set1_epi8 (0);
        const __m128i vector_short_00FF = _mm_set1_epi16(0x00FF);
        
        const unsigned char Zero = 128; 

        for (int eval_ct = 0; eval_ct < Evaluations_number; eval_ct++)
        {
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
                    // mul values of ARGB for each pixel of patch picture
                    // by value of A in ARGB for this pixel

                    ptch = _mm_mullo_epi16 ( ptch, ptch_a );
                    PTCH = _mm_mullo_epi16 ( PTCH, PTCH_A );

                    // for background multiplying by (255 - a)

                    bckg = _mm_mullo_epi16 ( bckg, _mm_sub_epi16 ( vector_short_00FF, ptch_a ) );
                    BCKG = _mm_mullo_epi16 ( BCKG, _mm_sub_epi16 ( vector_short_00FF, PTCH_A ) );

                    // sum - sum of multiplied values of 0 and 1 pixels
                    // SUM - sum of multiplied values of 2 and 3 pixels

                    __m128i sum  = _mm_add_epi16 ( ptch, bckg );
                    __m128i SUM  = _mm_add_epi16 ( PTCH, BCKG );

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

                    _mm_store_si128 ( (__m128i*) ( &reslt_data[offst] ), colors );
                }
            }
        }

    //===============================================================

    #else 

        Color pthc_argb = { 0 };
        Color bckg_argb = { 0 };
        
        for (int eval_ct = 0; eval_ct < Evaluations_number; eval_ct++)
        {
            for (int y = 0; y < y_size; y++)
            {
                int x = 0;
                int offset = 4 * (x + y * x_size);

                for (; x < x_size; x++, offset += 4)
                {
                    pthc_argb.value = *( (unsigned int*) &patch_data[offset] ); 
                    bckg_argb.value = *( (unsigned int*) &backg_data[offset] );

                    unsigned char ptch_a = pthc_argb.argb[0];

                    reslt_data[offset + 0] = ( pthc_argb.argb[3] * ptch_a + bckg_argb.argb[3] * (255 - ptch_a) ) >> 8;
                    reslt_data[offset + 1] = ( pthc_argb.argb[2] * ptch_a + bckg_argb.argb[2] * (255 - ptch_a) ) >> 8;
                    reslt_data[offset + 2] = ( pthc_argb.argb[1] * ptch_a + bckg_argb.argb[1] * (255 - ptch_a) ) >> 8;
                    reslt_data[offset + 3] = ( pthc_argb.argb[0] * ptch_a + bckg_argb.argb[0] * (255 - ptch_a) ) >> 8;
                }
            }
        }       

    #endif 

    return clock.getElapsedTime().asSeconds();
}