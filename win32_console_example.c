
#include <Windows.h>
#include <stdio.h>

typedef enum text_color_e {
    T_N = 0,
    T_B = FOREGROUND_BLUE,
    T_G = FOREGROUND_GREEN,
    T_R = FOREGROUND_RED,
    T_K = (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED),
} text_color_t;

int setBlock(text_color_t color)
{
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hStdout == INVALID_HANDLE_VALUE)
    {
        printf("Error while getting input handle");
        return EXIT_FAILURE;
    }

    if(color == T_N){
        //sets the color to normal
        SetConsoleTextAttribute(hStdout, T_K);
        printf(" ");
    }
    else{
        //sets the color to intense
        SetConsoleTextAttribute(hStdout, color | FOREGROUND_INTENSITY);
        // print a block
        printf("%c", 219);
    }

    return EXIT_SUCCESS;
}



int main(void)
{
    int i,j;
    text_color_t textBuf[6][5] = {
        {T_N, T_N, T_N, T_R, T_N,},
        {T_N, T_N, T_R, T_R, T_N},
        {T_N, T_R, T_N, T_R, T_N},
        {T_N, T_N, T_N, T_R, T_N},
        {T_N, T_N, T_N, T_R, T_N},
        {T_N, T_R, T_R, T_R, T_R},
    };

    // print out the buffer
    // row
    for(i=0; i<sizeof(textBuf)/sizeof(textBuf[0]); i++){
        // column
        for(j=0; j<sizeof(textBuf[0])/sizeof(textBuf[0][0]); j++){
            (void)setBlock(textBuf[i][j]);
        }
        printf("\n");
    }


    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

    return EXIT_SUCCESS;
}
