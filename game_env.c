#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#define BOARD_SIZE 14
#define MAX_ACTION_NUM 21365
#define CASTLE_K MAX_ACTION_NUM - 2
#define CASTLE_Q MAX_ACTION_NUM - 1

int actions[1000];
int actions_written;

enum directions
{
    N,
    E,
    S,
    W,
    NE,
    SE,
    SW,
    NW,
    LNW, // L for knight moves
    LNE,
    LEN,
    LES,
    LSE,
    LSW,
    LWS,
    LWN
};

int max(int a, int b)
{
    if(a > b)
        return a;
    else
        return b;
}

void draw_board(char board[BOARD_SIZE][BOARD_SIZE][2])
{
    int i, j;
    for(i = BOARD_SIZE - 1; i >= 0; i--)
    {
        for(j = 0; j < BOARD_SIZE; j++)
        {
            printf("%c%c ", board[i][j][0], board[i][j][1]);
        }
        printf("%c", '\n');
    }
}

void draw_pinned(int pinned[BOARD_SIZE][BOARD_SIZE][2])
{
    int i,j;
    for(i = BOARD_SIZE - 1; i >= 0; i--)
    {
        for(j = 0; j < BOARD_SIZE; j++)
        {
            if(pinned[i][j][0] == 0)
                printf("(nn nn) ");
            else
            {
                printf("(");
                if(pinned[i][j][0] < 10)
                    printf("0%d ", pinned[i][j][0]);
                else
                    printf("%d ", pinned[i][j][0]);
                if(pinned[i][j][1] < 10)
                    printf("0%d) ", pinned[i][j][1]);
                else
                    printf("%d) ", pinned[i][j][1]);
            }
        }
        printf("%c", '\n');
    }
}

void draw_checks(int checked_directions[16][2])
{
    int i;
    for(i = 0; i < 16; i++)
        printf("(%d %d) ", checked_directions[i][0], checked_directions[i][1]);
    printf("\n");
}

void get_xy_multipliers(int *x_dir, int *y_dir, int dir)
{
    switch(dir)
    {
        case N:
            *x_dir = 0;
            *y_dir = 1;
            break;
        case E:
            *x_dir = 1;
            *y_dir = 0;
            break;
        case S:
            *x_dir = 0;
            *y_dir = -1;
            break;
        case W:
            *x_dir = -1;
            *y_dir = 0;
            break;
        case NE:
            *x_dir = 1;
            *y_dir = 1;
            break;
        case SE:
            *x_dir = 1;
            *y_dir = -1;
            break;
        case SW:
            *x_dir = -1;
            *y_dir = -1;
            break;
        case NW:
            *x_dir = -1;
            *y_dir = 1;
            break;
    }
}

int is_pos_in_corner(int x, int y)
{
    return (x < 3 && (y < 3 || y > 10)) || (x > 10 && (y < 3 || y > 10));
}

enum directions get_direction(int from_x, int from_y, int to_x, int to_y)
{
    int x_diff, y_diff;
    x_diff = to_x - from_x;
    y_diff = to_y - from_y;
    if(abs(x_diff) == abs(y_diff) || x_diff == 0 || y_diff == 0)
    {
        if(to_y > from_y)
        {
            if(to_x == from_x)
                return N;
            else if(to_x > from_x)
                return NE;
            else
                return NW;
        }
        else if(to_y == from_y)
        {
            if(to_x > from_x)
                return E;
            else
                return W;
        }
        else
        {
            if(to_x == from_x)
                return S;
            else if(to_x > from_x)
                return SE;
            else
                return SW;
        }
    }
    else
    {
        int x_offsets[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
        int y_offsets[8] = {2, 2, 1, -1, -2, -2, -1, 1};
        int i;
        for(i = 0; i < 8; i++)
            if(x_diff == x_offsets[i] && y_diff == y_offsets[i])
                return LNW + i;
    }
    return N; // can't reach here. For warning disabling purpose only.
}

void write_action(int from_x, int from_y, int to_x, int to_y, char promotion)
{
    int x_diff, y_diff, diff;
    enum directions dir;
    int move_num;
    int marg;
    
    dir = get_direction(from_x, from_y, to_x, to_y);
    x_diff = abs(to_x - from_x);
    y_diff = abs(to_y - from_y);
    diff = max(x_diff, y_diff);
    if(promotion == '0')
    {
        if(dir >= N && dir <= W)
            move_num = dir * 13 + diff - 1;
        else if(dir >= NE && dir <= NW)
            move_num = 51 + (dir - NE) * 10 + diff;
        else
            move_num = 92 + dir - LNW;
    }
    else
    {
        if(dir == NW)
            marg = 0;
        else if(dir == N)
            marg = 3;
        else if(dir == NE)
            marg = 6;
        switch(promotion)
        {
            case 'r':
                move_num = 100 + marg;
                break;
            case 'n':
                move_num = 101 + marg;
                break;
            case 'b':
                move_num = 102 + marg;
                break;
        }
    }
    actions[actions_written] = 109 * (14 * from_y + from_x) + move_num;
    actions_written++;
    //printf("<%d: (%d %d) -> (%d %d)> ", actions[actions_written - 1], from_x, from_y, to_x, to_y);
}

void check_direction(char board[BOARD_SIZE][BOARD_SIZE][2], char player_color, int pinned[BOARD_SIZE][BOARD_SIZE][2], int checked_directions[16][2],
                     int king_x, int king_y, char enemy_left_color, char enemy_right_color, char ally_color, enum directions dir)
{
    int x_dir, y_dir;
    int pinned_x, pinned_y;
    int i;
    get_xy_multipliers(&x_dir, &y_dir, dir);
    pinned_x = -1;
    pinned_y = -1;
    for(i = 1; (king_x + x_dir*i >= 0) && (king_x + x_dir*i < BOARD_SIZE) && (king_y + y_dir*i >= 0) && (king_y + y_dir*i < BOARD_SIZE); i++)
    {
        if(board[king_y + y_dir*i][king_x + x_dir*i][0] == '0')
            continue;
        if(board[king_y + y_dir*i][king_x + x_dir*i][0] == player_color)
        {
            if(pinned_x != -1)
                break;
            pinned_x = king_x + x_dir*i;
            pinned_y = king_y + y_dir*i;
            continue;
        }
        if(board[king_y + y_dir*i][king_x + x_dir*i][0] == ally_color)
            break;
        if(board[king_y + y_dir*i][king_x + x_dir*i][0] == enemy_left_color || board[king_y + y_dir*i][king_x + x_dir*i][0] == enemy_right_color)
        {
            if(((x_dir * y_dir == 0 && board[king_y + y_dir*i][king_x + x_dir*i][1] != 'r') ||
               (x_dir * y_dir != 0 && board[king_y + y_dir*i][king_x + x_dir*i][1] != 'b')) &&
               (board[king_y + y_dir*i][king_x + x_dir*i][1] != 'q'))
                break;
            if(y_dir != 0 && 
              ((x_dir == 1 && board[king_y + y_dir*i][king_x + x_dir*i][0] == enemy_right_color && board[king_y + y_dir*i][king_x + x_dir*i][1] != 'p' &&
               board[king_y + y_dir*i][king_x + x_dir*i][1] != 'b' && board[king_y + y_dir*i][king_x + x_dir*i][1] != 'q') ||
              (x_dir == -1 && board[king_y + y_dir*i][king_x + x_dir*i][0] == enemy_left_color && board[king_y + y_dir*i][king_x + x_dir*i][1] != 'p' &&
               board[king_y + y_dir*i][king_x + x_dir*i][1] != 'b' && board[king_y + y_dir*i][king_x + x_dir*i][1] != 'q')))
                break;
            if(pinned_x != -1)
            {
                pinned[pinned_y][pinned_x][0] = king_y + y_dir*i;
                pinned[pinned_y][pinned_x][1] = king_x + x_dir*i;
                break;
            }
            checked_directions[dir][0] = king_y + y_dir*i;
            checked_directions[dir][1] = king_x + x_dir*i;
            break;
        }
    }
}

void get_knight_checks(char board[BOARD_SIZE][BOARD_SIZE][2], char player_color, int checked_directions[16][2],
                       int king_x, int king_y, char enemy_left_color, char enemy_right_color, char ally_color)
{
    int x_offsets[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
    int y_offsets[8] = {2, 2, 1, -1, -2, -2, -1, 1};
    int i;
    for(i = 0; i < 8; i++)
        if(king_y + y_offsets[i] >= 0 && king_y + y_offsets[i] < BOARD_SIZE &&
           king_x + x_offsets[i] >= 0 && king_x + x_offsets[i] < BOARD_SIZE &&
           (board[king_y + y_offsets[i]][king_x + x_offsets[i]][0] == enemy_left_color ||
           board[king_y + y_offsets[i]][king_x + x_offsets[i]][0] == enemy_right_color) &&
           board[king_y + y_offsets[i]][king_x + x_offsets[i]][1] == 'n')
        {
            checked_directions[LNW + i][0] = king_y + y_offsets[i];
            checked_directions[LNW + i][1] = king_x + x_offsets[i];
        }
}

void find_king(int *king_x, int *king_y, char board[BOARD_SIZE][BOARD_SIZE][2], char player_color)
{
    int i, j;
    for(i = 0; i < BOARD_SIZE; i++)
        for(j = 0; j < BOARD_SIZE; j++)
            if(board[i][j][1] == 'k' && board[i][j][0] == player_color)
            {
                *king_x = j;
                *king_y = i;
                return;
            }
}

void get_pinned_and_checks(char board[BOARD_SIZE][BOARD_SIZE][2], char player_color, int pinned[BOARD_SIZE][BOARD_SIZE][2], int checked_directions[16][2],
                           char enemy_left_color, char enemy_right_color, char ally_color)
{
    int i, j;
    int king_x, king_y;
    int pinned_x, pinned_y;

    find_king(&king_x, &king_y, board, player_color);
    //printf("%d %d\n", king_x, king_y);
    check_direction(board, player_color, pinned, checked_directions, king_x, king_y,
                    enemy_left_color, enemy_right_color, ally_color, N);
    check_direction(board, player_color, pinned, checked_directions, king_x, king_y,
                    enemy_left_color, enemy_right_color, ally_color, E);
    check_direction(board, player_color, pinned, checked_directions, king_x, king_y,
                    enemy_left_color, enemy_right_color, ally_color, S);
    check_direction(board, player_color, pinned, checked_directions, king_x, king_y,
                    enemy_left_color, enemy_right_color, ally_color, W);
    check_direction(board, player_color, pinned, checked_directions, king_x, king_y,
                    enemy_left_color, enemy_right_color, ally_color, NE);                   
    check_direction(board, player_color, pinned, checked_directions, king_x, king_y,
                    enemy_left_color, enemy_right_color, ally_color, SE);
    check_direction(board, player_color, pinned, checked_directions, king_x, king_y,
                    enemy_left_color, enemy_right_color, ally_color, SW);
    check_direction(board, player_color, pinned, checked_directions, king_x, king_y,
                    enemy_left_color, enemy_right_color, ally_color, NW);
    get_knight_checks(board, player_color, checked_directions,
                      king_x, king_y, enemy_left_color, enemy_right_color, ally_color);
}

int init_board(const char text_board_representation[], char board[BOARD_SIZE][BOARD_SIZE][2])
{
    int i = 0;
    int j = 0;
    int k = 0;
    for(i = 0; i < BOARD_SIZE; i++)
        for(j = 0; j < BOARD_SIZE; j++)
        {
            board[i][j][0] = '0';
            board[i][j][1] = '0';
        }
    i = 0;
    j = 0;
    for(k = 0; text_board_representation[k] != ' ' && text_board_representation[k] != '\0'; k++)
    {
        if(isdigit(text_board_representation[k]))
        {
            j += 10 * (text_board_representation[k] - '0') + (text_board_representation[k + 1] - '0');
            k++;
        }
        else if(text_board_representation[k] == '/')
        {
            i++;
            j = 0;
        }
        else
        {
            board[i][j][0] = text_board_representation[k];
            board[i][j][1] = text_board_representation[k + 1];
            j++;
            k++;
        }
    }
    return k;
}

void init(const char text_board_representation[], char board[BOARD_SIZE][BOARD_SIZE][2], int pinned[BOARD_SIZE][BOARD_SIZE][2], int checked_directions[16][2],
                char *player_color, char *ally_color, char* enemy_left_color, char *enemy_right_color, int *en_pas_x_l, int *en_pas_y_l, int *en_pas_x_r, int *en_pas_y_r,
                int *castle_k, int *castle_q)
{
    int i = 0;
    int j = 0;
    int k = 0;
    k = init_board(text_board_representation, board);
    for(i = 0; i < BOARD_SIZE; i++)
        for(j = 0; j < BOARD_SIZE; j++)
        {
            pinned[i][j][0] = 0;
            pinned[i][j][1] = 0;
        }
    k++;
    *player_color = text_board_representation[k];
    switch(*player_color)
    {
        case 'r':
            *enemy_left_color = 'b';
            *enemy_right_color = 'g';
            *ally_color = 'y';
            break;
        case 'b':
            *enemy_left_color = 'y';
            *enemy_right_color = 'r';
            *ally_color = 'g';
            break;
        case 'y':
            *enemy_left_color = 'g';
            *enemy_right_color = 'b';
            *ally_color = 'r';
            break;
        case 'g':
            *enemy_left_color = 'r';
            *enemy_right_color = 'y';
            *ally_color = 'b';
            break;
    }
    k+=2;
    while(text_board_representation[k] != ' ')
    {
        if(text_board_representation[k] == *player_color)
        {
            *castle_k = text_board_representation[k + 1] == 'k';
            *castle_q = text_board_representation[k + 2] == 'q';
        }
        k += 3;
    }
    k++;
    switch(*player_color)
    {
        case 'r':
            *en_pas_y_l = (text_board_representation[k + 6] - '0') * 10 + (text_board_representation[k + 7] - '0');
            *en_pas_x_l = (text_board_representation[k + 8] - '0') * 10 + (text_board_representation[k + 9] - '0');
            *en_pas_y_r = (text_board_representation[k + 16] - '0') * 10 + (text_board_representation[k + 17] - '0');
            *en_pas_x_r = (text_board_representation[k + 18] - '0') * 10 + (text_board_representation[k + 19] - '0');
            break;
        case 'b':
            *en_pas_y_l = (text_board_representation[k + 11] - '0') * 10 + (text_board_representation[k + 12] - '0');
            *en_pas_x_l = (text_board_representation[k + 13] - '0') * 10 + (text_board_representation[k + 14] - '0');
            *en_pas_y_r = (text_board_representation[k + 1] - '0') * 10 + (text_board_representation[k + 2] - '0');
            *en_pas_x_r = (text_board_representation[k + 3] - '0') * 10 + (text_board_representation[k + 4] - '0');
            break;
        case 'y':
            *en_pas_y_r = (text_board_representation[k + 6] - '0') * 10 + (text_board_representation[k + 7] - '0');
            *en_pas_x_r = (text_board_representation[k + 8] - '0') * 10 + (text_board_representation[k + 9] - '0');
            *en_pas_y_l = (text_board_representation[k + 16] - '0') * 10 + (text_board_representation[k + 17] - '0');
            *en_pas_x_l = (text_board_representation[k + 18] - '0') * 10 + (text_board_representation[k + 19] - '0');
            break;
        case 'g':
            *en_pas_y_r = (text_board_representation[k + 11] - '0') * 10 + (text_board_representation[k + 12] - '0');
            *en_pas_x_r = (text_board_representation[k + 13] - '0') * 10 + (text_board_representation[k + 14] - '0');
            *en_pas_y_l = (text_board_representation[k + 1] - '0') * 10 + (text_board_representation[k + 2] - '0');
            *en_pas_x_l = (text_board_representation[k + 3] - '0') * 10 + (text_board_representation[k + 4] - '0');
            break;
    }
    for(i = 0; i < 16; i++)
    {
        checked_directions[i][0] = 0;
        checked_directions[i][1] = 0;
    }
}

void get_actions_nonpinned(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2],
                           char player_color, char enemy_left_color, char enemy_right_color, char ally_color,
                           enum directions dir)
{
    int i, j;
    int x_dir, y_dir;
    get_xy_multipliers(&x_dir, &y_dir, dir);
    for(i = 1; (x + x_dir*i >= 0) && (x + x_dir*i < BOARD_SIZE) && (y + y_dir*i >= 0) && (y + y_dir*i < BOARD_SIZE); i++)
    {
        if(board[y + y_dir*i][x + x_dir*i][0] == '0' && !is_pos_in_corner(x + x_dir*i, y + y_dir*i))
            //printf("(%d %d)->(%d %d) ", x, y, x + x_dir*i, y + y_dir*i);
            write_action(x, y, x + x_dir*i, y + y_dir*i, '0');
        if(board[y + y_dir*i][x + x_dir*i][0] == player_color || board[y + y_dir*i][x + x_dir*i][0] == ally_color)
            break;
        if(board[y + y_dir*i][x + x_dir*i][0] == enemy_left_color || (board[y + y_dir*i][x + x_dir*i][0] == enemy_right_color &&
           !is_pos_in_corner(x + x_dir*i, y + y_dir*i)))
        {
            //printf("(%d %d)->(%d %d) ", x, y, x + x_dir*i, y + y_dir*i);
            write_action(x, y, x + x_dir*i, y + y_dir*i, '0');
            break;
        }
    }
    
}

void get_actions_pinned(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2], int pinned[BOARD_SIZE][BOARD_SIZE][2],
                        char player_color, char enemy_left_color, char enemy_right_color, char ally_color,
                        enum directions dir)
{
    switch(dir)
    {
        case N:
            if(pinned[y][x][0] > y && pinned[y][x][1] == x)
                get_actions_nonpinned(x, y, board,
                                      player_color, enemy_left_color, enemy_right_color, ally_color, dir);
            break;
        case E:
            if(pinned[y][x][0] == y && pinned[y][x][1] > x)
                get_actions_nonpinned(x, y, board,
                                      player_color, enemy_left_color, enemy_right_color, ally_color, dir);
            break;
        case S:
            if(pinned[y][x][0] < y && pinned[y][x][1] == x)
                get_actions_nonpinned(x, y, board,
                                      player_color, enemy_left_color, enemy_right_color, ally_color, dir);
            break;
        case W:
            if(pinned[y][x][0] == y && pinned[y][x][1] < x)
                get_actions_nonpinned(x, y, board,
                                      player_color, enemy_left_color, enemy_right_color, ally_color, dir);
            break;
        case NE:
            if(pinned[y][x][0] > y && pinned[y][x][1] > x)
                get_actions_nonpinned(x, y, board,
                                      player_color, enemy_left_color, enemy_right_color, ally_color, dir);
            break;
        case SE:
            if(pinned[y][x][0] < y && pinned[y][x][1] > x)
                get_actions_nonpinned(x, y, board,
                                      player_color, enemy_left_color, enemy_right_color, ally_color, dir);
            break;
        case SW:
            if(pinned[y][x][0] < y && pinned[y][x][1] < x)
                get_actions_nonpinned(x, y, board,
                                      player_color, enemy_left_color, enemy_right_color, ally_color, dir);
            break;
        case NW:
            if(pinned[y][x][0] > y && pinned[y][x][1] < x)
                get_actions_nonpinned(x, y, board,
                                      player_color, enemy_left_color, enemy_right_color, ally_color, dir);
            break;
        default:
            break;
    }
}

int is_protected_by_knight(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2], char piece_color, char piece_ally_color)
{
    int x_offsets[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
    int y_offsets[8] = {2, 2, 1, -1, -2, -2, -1, 1};
    int i;
    for(i = 0; i < 8; i++)
        if(y + y_offsets[i] >= 0 && y + y_offsets[i] < BOARD_SIZE &&
           x + x_offsets[i] >= 0 && x + x_offsets[i] < BOARD_SIZE &&
           board[y + y_offsets[i]][x + x_offsets[i]][1] == 'n' &&
           (board[y + y_offsets[i]][x + x_offsets[i]][0] == piece_color ||
           board[y + y_offsets[i]][x + x_offsets[i]][0] == piece_ally_color))
            return 1;
    return 0;
}

int is_protected_in_dir(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2], char piece_color, char piece_ally_color,
                        char player_color, enum directions dir)
{
    int x_dir, y_dir;
    int i;
    get_xy_multipliers(&x_dir, &y_dir, dir);
    for(i = 1; (x + x_dir*i >= 0) && (x + x_dir*i < BOARD_SIZE) && (y + y_dir*i >= 0) && (y + y_dir*i < BOARD_SIZE); i++)
    {
        if(board[y + y_dir*i][x + x_dir*i][0] == piece_color || board[y + y_dir*i][x + x_dir*i][0] == piece_ally_color)
        {
            if(dir == N || dir == E || dir == S || dir == W)
                return board[y + y_dir*i][x + x_dir*i][1] == 'r' || board[y + y_dir*i][x + x_dir*i][1] == 'q' || board[y + y_dir*i][x + x_dir*i][1] == 'k';
            if(board[y + y_dir*i][x + x_dir*i][1] != 'p' && (dir == NE || dir == SE || dir == SW || dir == NW))
                return board[y + y_dir*i][x + x_dir*i][1] == 'b' || board[y + y_dir*i][x + x_dir*i][1] == 'q' || board[y + y_dir*i][x + x_dir*i][1] == 'k';
            if(board[y + y_dir*i][x + x_dir*i][1] == 'p' && i != 1)
                return 0;
            if(board[y + y_dir*i][x + x_dir*i][1] == 'p')
                switch(player_color)
                {
                    case 'r':
                        return ((dir == NE || dir == SE) && board[y + y_dir*i][x + x_dir*i][0] == 'g') ||
                               ((dir == NW || dir == SW) && board[y + y_dir*i][x + x_dir*i][0] == 'b');
                    case 'b':
                        return ((dir == NE || dir == SE) && board[y + y_dir*i][x + x_dir*i][0] == 'r') ||
                               ((dir == NW || dir == SW) && board[y + y_dir*i][x + x_dir*i][0] == 'y');
                    case 'y':
                        return ((dir == NE || dir == SE) && board[y + y_dir*i][x + x_dir*i][0] == 'b') ||
                               ((dir == NW || dir == SW) && board[y + y_dir*i][x + x_dir*i][0] == 'g');
                    case 'g':
                        return ((dir == NE || dir == SE) && board[y + y_dir*i][x + x_dir*i][0] == 'y') ||
                               ((dir == NW || dir == SW) && board[y + y_dir*i][x + x_dir*i][0] == 'r');
                }   
        }
        else if(board[y + y_dir*i][x + x_dir*i][0] != '0')
            break;
    }
    return 0;
}

int is_protected(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2], char piece_color, char player_color)
{
    char piece_ally_color;
    enum directions i;
    switch(piece_color)
    {
        case 'r':
            piece_ally_color = 'y';
            break;
        case 'y':
            piece_ally_color = 'r';
            break;
        case 'b':
            piece_ally_color = 'g';
            break;
        case 'g':
            piece_ally_color = 'b';
            break;
        case '0':
            if(player_color == 'r' || player_color == 'y')
            {
                piece_color = 'b';
                piece_ally_color = 'g';
            }
            else
            {
                piece_color = 'r';
                piece_ally_color = 'y';
            }
    }
    for(i = N; i <= NW; i++)
        if(is_protected_in_dir(x, y, board, piece_color, piece_ally_color,
                               player_color, i))
            return 1;
    return is_protected_by_knight(x, y, board, piece_color, piece_ally_color);
}

void add_pawn_actions(int from_x, int from_y, int to_x, int to_y)
{
    if(is_pos_in_corner(to_x, to_y))
        return;
    if(from_y == 9)
    {
        //printf("(%d %d)->(%d %d promote) ", from_x, from_y, to_x, to_y);
        write_action(from_x, from_y, to_x, to_y, '0');
        write_action(from_x, from_y, to_x, to_y, 'r');
        write_action(from_x, from_y, to_x, to_y, 'n');
        write_action(from_x, from_y, to_x, to_y, 'b');
    }
    else
        //printf("(%d %d)->(%d %d) ", from_x, from_y, to_x, to_y);
        write_action(from_x, from_y, to_x, to_y, '0');
}

void get_pawn_actions_non_pinned(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2],
                                 char player_color, char enemy_left_color, char enemy_right_color, 
                                 int en_pas_x_l, int en_pas_y_l, int en_pas_x_r, int en_pas_y_r)
{
    if(y == 1 && board[2][x][0] == '0' && board[3][x][0] == '0')
        add_pawn_actions(x, y, x, y + 2);
    if(en_pas_x_l == x - 1 && en_pas_y_l == y + 1)
        add_pawn_actions(x, y, x - 1, y + 1);
    if(en_pas_x_l == x + 1 && en_pas_y_l == y + 1)
        add_pawn_actions(x, y, x + 1, y + 1);
    if(en_pas_x_r == x - 1 && en_pas_y_r == y + 1)
        add_pawn_actions(x, y, x - 1, y + 1);
    if(en_pas_x_r == x + 1 && en_pas_y_r == y + 1)
        add_pawn_actions(x, y, x + 1, y + 1);
    if(x > 0 && (board[y + 1][x - 1][0] == enemy_left_color || board[y + 1][x - 1][0] == enemy_right_color))
        add_pawn_actions(x, y, x - 1, y + 1);
    if(x + 1 < BOARD_SIZE && (board[y + 1][x + 1][0] == enemy_left_color || board[y + 1][x + 1][0] == enemy_right_color))
        add_pawn_actions(x, y, x + 1, y + 1);
    if(board[y + 1][x][0] == '0')
        add_pawn_actions(x, y, x, y + 1);
}

void get_pawn_actions_pinned(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2], int pinned[BOARD_SIZE][BOARD_SIZE][2],
                             char player_color, char enemy_left_color, char enemy_right_color,
                             int en_pas_x_l, int en_pas_y_l, int en_pas_x_r, int en_pas_y_r)
{
    if(pinned[y][x][1] == x && board[y + 1][x][0] == '0')
    {
        add_pawn_actions(x, y, x, y + 1);
        if(y == 1 && board[2][x][0] == '0' && board[3][x][0] == '0')
            add_pawn_actions(x, y, x, y + 2);
    }
    if(pinned[y][x][0] > y)
    {
        if((pinned[y][x][1] == x + 1 && board[y + 1][x + 1][0] != '0') || (en_pas_x_l == x + 1 && en_pas_y_l == y + 1))
            add_pawn_actions(x, y, x + 1, y + 1);
        if((pinned[y][x][1] == x - 1 && board[y + 1][x - 1][0] != '0') || (en_pas_x_l == x - 1 && en_pas_y_l == y + 1))
            add_pawn_actions(x, y, x - 1, y + 1);
        if((pinned[y][x][1] == x + 1 && board[y + 1][x + 1][0] != '0') || (en_pas_x_r == x + 1 && en_pas_y_r == y + 1))
            add_pawn_actions(x, y, x + 1, y + 1);
        if((pinned[y][x][1] == x - 1 && board[y + 1][x - 1][0] != '0') || (en_pas_x_r == x - 1 && en_pas_y_r == y + 1))
            add_pawn_actions(x, y, x - 1, y + 1);
    }
    else
    {
        if(pinned[y][x][1] == x + 1 && en_pas_x_l == x - 1 && en_pas_y_l == y + 1)
            add_pawn_actions(x, y, x - 1, y + 1);
        if(pinned[y][x][1] == x - 1 && en_pas_x_l == x + 1 && en_pas_y_l == y + 1)
            add_pawn_actions(x, y, x + 1, y + 1);
        if(pinned[y][x][1] == x + 1 && en_pas_x_r == x - 1 && en_pas_y_r == y + 1)
            add_pawn_actions(x, y, x - 1, y + 1);
        if(pinned[y][x][1] == x - 1 && en_pas_x_r == x + 1 && en_pas_y_r == y + 1)
            add_pawn_actions(x, y, x + 1, y + 1);
    }
}

void get_king_actions(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2],
                      char player_color, char ally_color, int castle_k, int castle_q)
{
    int x_offsets_k[8] = {0, 1, 0, -1, 1, 1, -1, -1};
    int y_offsets_k[8] = {1, 0, -1, 0, 1, -1, -1, 1};
    int i;
    for(i = 0; i < 8; i++)
        if(y + y_offsets_k[i] >= 0 && y + y_offsets_k[i] < BOARD_SIZE &&
           x + x_offsets_k[i] >= 0 && x + x_offsets_k[i] < BOARD_SIZE &&
           board[y + y_offsets_k[i]][x + x_offsets_k[i]][0] != player_color &&
           board[y + y_offsets_k[i]][x + x_offsets_k[i]][0] != ally_color &&
           !is_protected(x + x_offsets_k[i], y + y_offsets_k[i] , board, board[y + y_offsets_k[i]][x + x_offsets_k[i]][0], player_color) &&
           !is_pos_in_corner(x + x_offsets_k[i], y + y_offsets_k[i]))
           //printf("(%d %d)->(%d %d) ", x, y, x + x_offsets_k[i], y + y_offsets_k[i]);
           write_action(x, y, x + x_offsets_k[i], y + y_offsets_k[i], '0');
    if(castle_k)
    {
        for(i = 0; i < 4; i++)
            if(is_protected(x + i, y, board, '0', player_color))
                return;
        if(board[0][8][0] == '0' && board[0][9][0] == '0')
        {
            //printf("O-O ");
            actions[actions_written] = CASTLE_K;
            actions_written++;
        }
    }
    if(castle_q)
    {
        for(i = 0; i < 5; i++)
            if(is_protected(x - i, y, board, '0', player_color))
                return;
        if(board[0][4][0] == '0' && board[0][5][0] == '0' && board[0][6][0] == '0')
        {
            //printf("O--O ");
            actions[actions_written] = CASTLE_Q;
            actions_written++;
        }
    }
}
    

void get_actions_on_coord(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2], int pinned[BOARD_SIZE][BOARD_SIZE][2],
                          char player_color, char enemy_left_color, char enemy_right_color, char ally_color, int en_pas_x_l, int en_pas_y_l,
                          int en_pas_x_r, int en_pas_y_r, int castle_k, int castle_q)
{
    int x_offsets_n[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
    int y_offsets_n[8] = {2, 2, 1, -1, -2, -2, -1, 1};
    int i, j;
    switch(board[y][x][1])
    {
        case 'r':
            if(pinned[y][x][0] != 0)
            {
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, N);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, E);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, S);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, W);
            }
            else
            {
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, N);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, E);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, S);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, W);
            }
            break;
        case 'n':
            if(pinned[y][x][0] == 0)
                for(i = 0; i < 8; i++)
                    if(y + y_offsets_n[i] >= 0 && y + y_offsets_n[i] < BOARD_SIZE &&
                       x + x_offsets_n[i] >= 0 && x + x_offsets_n[i] < BOARD_SIZE &&
                       board[y + y_offsets_n[i]][x + x_offsets_n[i]][0] != player_color &&
                       board[y + y_offsets_n[i]][x + x_offsets_n[i]][0] != ally_color &&
                       !is_pos_in_corner( x + x_offsets_n[i], y + y_offsets_n[i]))
                        //printf("(%d %d)->(%d %d) ", x, y, x + x_offsets_n[i], y + y_offsets_n[i]);
                        write_action(x, y, x + x_offsets_n[i], y + y_offsets_n[i], '0');
            break;
        case 'b':
            if(pinned[y][x][0] != 0)
            {
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, NE);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, SE);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, SW);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, NW);
            }
            else
            {
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, NE);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, SE);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, SW);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, NW);
            }
            break;
        case 'q':
            if(pinned[y][x][0] != 0)
            {
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, N);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, E);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, S);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, W);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, NE);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, SE);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, SW);
                get_actions_pinned(x, y, board, pinned,
                                   player_color, enemy_left_color, enemy_right_color, ally_color, NW);
            }
            else
            {
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, N);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, E);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, S);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, W);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, NE);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, SE);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, SW);
                get_actions_nonpinned(x, y, board, player_color, enemy_left_color, enemy_right_color, ally_color, NW);
            }
            break;
        case 'k':
            get_king_actions(x, y, board, player_color, ally_color, castle_k, castle_q);
            break;
        case 'p':
            if(pinned[y][x][0] == 0)
                get_pawn_actions_non_pinned(x, y, board, player_color, enemy_left_color, enemy_right_color, en_pas_x_l, en_pas_y_l,
                                            en_pas_x_r, en_pas_y_r);
            else
                get_pawn_actions_pinned(x, y, board, pinned, player_color, enemy_left_color, enemy_right_color, en_pas_x_l, en_pas_y_l,
                                        en_pas_x_r, en_pas_y_r);
            break;
            
    }
}

void get_check_block_in_dir(int x, int y, char board[BOARD_SIZE][BOARD_SIZE][2], int pinned[BOARD_SIZE][BOARD_SIZE][2], 
                            char player_color, enum directions dir)
{
    int x_dir, y_dir;
    int i;
    get_xy_multipliers(&x_dir, &y_dir, dir);
    for(i = 1; (x + x_dir*i >= 0) && (x + x_dir*i < BOARD_SIZE) && (y + y_dir*i >= 0) && (y + y_dir*i < BOARD_SIZE); i++)
    {
        if(board[y + y_dir*i][x + x_dir*i][0] == player_color)
        {
            if((dir == N || dir == E || dir == S || dir == W) &&
               (board[y + y_dir*i][x + x_dir*i][1] == 'r' || board[y + y_dir*i][x + x_dir*i][1] == 'q') &&
               (pinned[y + y_dir*i][x + x_dir*i][0] == 0 && pinned[y + y_dir*i][x + x_dir*i][1] == 0))
                //printf("(%d %d)->(%d %d) ", x + x_dir*i, y + y_dir*i, x, y);
                write_action(x + x_dir*i, y + y_dir*i, x, y, '0');
            
            if((dir == NE || dir == SE || dir == SW || dir == NW) &&
               (board[y + y_dir*i][x + x_dir*i][1] == 'b' || board[y + y_dir*i][x + x_dir*i][1] == 'q') && 
               (pinned[y + y_dir*i][x + x_dir*i][0] == 0 && pinned[y + y_dir*i][x + x_dir*i][1] == 0))
                //printf("(%d %d)->(%d %d) ", x + x_dir*i, y + y_dir*i, x, y);
                write_action(x + x_dir*i, y + y_dir*i, x, y, '0');
            break;
        }
        else if(board[y + y_dir*i][x + x_dir*i][0] != '0')
            break;
    }
}

void get_check_blocks(char board[BOARD_SIZE][BOARD_SIZE][2], int pinned[BOARD_SIZE][BOARD_SIZE][2], int checked_directions[16][2], 
                      char player_color, char enemy_left_color, char enemy_right_color, char ally_color,
                      int en_pas_x_l, int en_pas_y_l, int en_pas_x_r, int en_pas_y_r)
{
    int king_x, king_y;
    int i, j, x_diff, y_diff, diff;
    int attacker_x, attacker_y;
    enum directions check_dir, dir;
    int x_dir, y_dir;
    int x_offsets_n[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
    int y_offsets_n[8] = {2, 2, 1, -1, -2, -2, -1, 1};
    find_king(&king_x, &king_y, board, player_color);
    for(i = 0; i < 16; i++)
    {
        if(checked_directions[i][0] != 0 || checked_directions[i][1] != 0)
        {
            attacker_y = checked_directions[i][0];
            attacker_x = checked_directions[i][1];
            check_dir = i;
            break;
        }
    }
    if(check_dir < LNW)
    {
        get_xy_multipliers(&x_dir, &y_dir, check_dir);
        x_diff = abs(attacker_x - king_x);
        y_diff = abs(attacker_y - king_y);
        diff = max(x_diff, y_diff);
        for(i = 1; i < diff; i++)
        {
            for(dir = N; dir < LNW; dir++)
                get_check_block_in_dir(king_x + i*x_dir, king_y + i*y_dir, board, pinned, player_color, dir);
            if(king_y + y_dir*i > 0 && board[king_y + y_dir*i - 1][king_x + x_dir*i][0] == player_color &&
               board[king_y + y_dir*i - 1][king_x + x_dir*i][1] == 'p')
                add_pawn_actions(king_x + x_dir*i, king_y + y_dir*i - 1, king_x + x_dir*i, king_y + y_dir*i);
            if(king_y + y_dir*i == 3 && board[1][king_x + x_dir*i][0] == player_color &&
               board[2][king_x + x_dir*i][0] == '0' &&
               board[1][king_x + x_dir*i][1] == 'p')
                add_pawn_actions(king_x + x_dir*i, 1, king_x + x_dir*i, 3);
            if(king_y + y_dir*i == en_pas_y_l && king_x + x_dir*i == en_pas_x_l)
            {
                if(board[en_pas_y_l - 1][en_pas_x_l - 1][0] == player_color && board[en_pas_y_l - 1][en_pas_x_l - 1][1] == 'p')
                    add_pawn_actions(en_pas_x_l - 1, en_pas_y_l - 1, en_pas_x_l, en_pas_y_l);
                if(board[en_pas_y_l - 1][en_pas_x_l + 1][0] == player_color && board[en_pas_y_l - 1][en_pas_x_l + 1][1] == 'p')
                    add_pawn_actions(en_pas_x_l + 1, en_pas_y_l - 1, en_pas_x_l, en_pas_y_l);
            }
            if(king_y + y_dir*i == en_pas_y_r && king_x + x_dir*i == en_pas_x_r)
            {
                if(board[en_pas_y_r - 1][en_pas_x_r - 1][0] == player_color && board[en_pas_y_r - 1][en_pas_x_r - 1][1] == 'p')
                    add_pawn_actions(en_pas_x_r - 1, en_pas_y_r - 1, en_pas_x_r, en_pas_y_r);
                if(board[en_pas_y_r - 1][en_pas_x_r + 1][0] == player_color && board[en_pas_y_r - 1][en_pas_x_r + 1][1] == 'p')
                    add_pawn_actions(en_pas_x_r + 1, en_pas_y_r - 1, en_pas_x_r, en_pas_y_r);
            }
            for(j = 0; j < 8; j++)
                if(board[king_y + y_dir*i + y_offsets_n[j]][king_x + x_dir*i + x_offsets_n[j]][0] == player_color &&
                   board[king_y + y_dir*i + y_offsets_n[j]][king_x + x_dir*i + x_offsets_n[j]][1] == 'n')
                    //printf("(%d %d)->(%d %d) ", king_x + x_dir*i + x_offsets_n[j], king_y + y_dir*i + y_offsets_n[j], king_x + x_dir*i, king_y + y_dir*i);
                    write_action(king_x + x_dir*i + x_offsets_n[j], king_y + y_dir*i + y_offsets_n[j], king_x + x_dir*i, king_y + y_dir*i, '0');
        }      
    }
    for(dir = N; dir < LNW; dir++)
        get_check_block_in_dir(attacker_x, attacker_y, board, pinned, player_color, dir);
    for(i = 0; i < 8; i++)
        if(board[attacker_y + y_offsets_n[i]][attacker_x + x_offsets_n[i]][0] == player_color &&
           board[attacker_y + y_offsets_n[i]][attacker_x + x_offsets_n[i]][1] == 'n')
            //printf("(%d %d)->(%d %d) ", attacker_x + x_offsets_n[i], attacker_y + y_offsets_n[i], attacker_x, attacker_y);
            write_action(attacker_x + x_offsets_n[i], attacker_y + y_offsets_n[i], attacker_x, attacker_y, '0');
    if(attacker_y > 0)
    {
        if(attacker_x > 0 && board[attacker_y - 1][attacker_x - 1][0] == player_color && board[attacker_y - 1][attacker_x - 1][1] == 'p')
            add_pawn_actions(attacker_x - 1, attacker_y - 1, attacker_x, attacker_y);
        if(attacker_x < BOARD_SIZE - 1 && board[attacker_y - 1][attacker_x + 1][0] == player_color && board[attacker_y - 1][attacker_x + 1][1] == 'p')
            add_pawn_actions(attacker_x + 1, attacker_y - 1, attacker_x, attacker_y);
    }
}

void get_available_actions(const char text_board_representation[], int a[1000])
{
    char board[BOARD_SIZE][BOARD_SIZE][2];
    int pinned[BOARD_SIZE][BOARD_SIZE][2];
    char player_color;
    int en_pas_x_l, en_pas_y_l, en_pas_x_r, en_pas_y_r;
    int castle_k, castle_q;
    int checked_directions[16][2];
    int i, j;
    int king_x = 0, king_y = 0;
    char enemy_left_color, enemy_right_color, ally_color;
    int checks_count;
    
    for(i = 0; i < 1000; i++)
        actions[i] = -1;
    actions_written = 0;
    
    init(text_board_representation, board, pinned, checked_directions, &player_color,
         &ally_color, &enemy_left_color, &enemy_right_color, &en_pas_x_l, &en_pas_y_l, &en_pas_x_r, &en_pas_y_r, &castle_k, &castle_q);
    
    get_pinned_and_checks(board, player_color, pinned, checked_directions, 
                          enemy_left_color, enemy_right_color, ally_color);
    //draw_board(board);
    //draw_pinned(pinned);
    //draw_checks(checked_directions);
    
    find_king(&king_x, &king_y, board, player_color);
    if(king_x == 0 && king_y == 0)
    {
        for(i = 0; i < 1000; i++)
            a[i] = actions[i];
        return;
    }
    checks_count = 0;
    for(i = 0; i < 16; i++)
        checks_count += (checked_directions[i][0] != 0 || checked_directions[i][1] != 0);   
        
    if(checks_count == 0)
        for(i = 0; i < BOARD_SIZE; i++)
            for(j = 0; j < BOARD_SIZE; j++)
            {
                if(board[i][j][0] == player_color)
                    get_actions_on_coord(j, i, board, pinned,
                                         player_color, enemy_left_color, enemy_right_color, ally_color, en_pas_x_l, en_pas_y_l,
                                         en_pas_x_r, en_pas_y_r, castle_k, castle_q);
            }
    else if(checks_count == 1)
    {
        get_king_actions(king_x, king_y, board, player_color, ally_color, castle_k, castle_q);
        get_check_blocks(board, pinned, checked_directions, 
                         player_color, enemy_left_color, enemy_right_color, ally_color, en_pas_x_l, en_pas_y_l,
                         en_pas_x_r, en_pas_y_r);
    }
    else
        get_king_actions(king_x, king_y, board, player_color, ally_color, castle_k, castle_q);
    for(i = 0; i < 1000; i++)
        a[i] = actions[i];
}

void write_square_to_text(int i, int j, int *k, int *free_squares, char text[500], char board[BOARD_SIZE][BOARD_SIZE][2])
{
    if(board[i][j][0] == '0')
        *free_squares += 1;
    else
    {
        if(*free_squares != 0)  
        {
            text[*k] = '0' + (*free_squares / 10);
            text[*k + 1] = '0' + (*free_squares % 10);
            *free_squares = 0;
            *k += 2;
        }
        text[*k] = board[i][j][0];
        text[*k + 1] = board[i][j][1];
        *k += 2;
    }
}

int board_to_text(char board[BOARD_SIZE][BOARD_SIZE][2], char text[500], int rotate)
{
    int i = 0, j = 0, k = 0;
    int free_squares = 0;
    if(rotate)
        for(j = 0; j < BOARD_SIZE; j++)
        {
            for(i = BOARD_SIZE - 1; i >= 0; i--)
                write_square_to_text(i, j, &k, &free_squares, text, board);
            if(free_squares != 0)
            {
                text[k] = '0' + (free_squares / 10);
                text[k + 1] = '0' + (free_squares % 10);
                free_squares = 0;
                k += 2;
            }
            text[k] = '/';
            k++;
        }
    else
        for(i = 0; i < BOARD_SIZE; i++)
        {
            for(j = 0; j < BOARD_SIZE; j++)
                write_square_to_text(i, j, &k, &free_squares, text, board);
            if(free_squares != 0)
            {
                text[k] = '0' + (free_squares / 10);
                text[k + 1] = '0' + (free_squares % 10);
                free_squares = 0;
                k += 2;
            }
            text[k] = '/';
            k++;
        }
    k--;
    text[k] = '\0';
    k++;
    return k;
}

void change_board(char piece_type, char piece_color, char board[BOARD_SIZE][BOARD_SIZE][2], int x_from, int y_from, int move_num, int action,
                  int *x_to, int *y_to, int *castle_k_flag, int *castle_q_flag, int *progress_move_flag,
                  int en_pas_x_l, int en_pas_y_l, int en_pas_x_r, int en_pas_y_r)
{
    char next_piece_type = piece_type;
    int move_len;
    board[y_from][x_from][0] = '0';
    board[y_from][x_from][1] = '0';
    
    if(piece_type == 'k')
    {
        *castle_q_flag = 1;
        *castle_k_flag = 1;
    }
    if(piece_type == 'r')
    {
        if(x_from == 3 && y_from == 0)
        {
            *castle_q_flag = 1;
        }
        else
        {
            if(x_from == 10 && y_from == 0)
                *castle_k_flag = 1;
        }
    }
    if(y_from == 9 && move_num < 100 && piece_type == 'p')
        next_piece_type = 'q';
    
    if(action == CASTLE_K)
    {
        board[0][7][0] = '0';
        board[0][7][1] = '0';
        board[0][10][0] = '0';
        board[0][10][1] = '0';
        board[0][9][0] = piece_color;
        board[0][9][1] = 'k';
        board[0][8][0] = piece_color;
        board[0][8][1] = 'r';
        *castle_q_flag = 1;
        *castle_k_flag = 1;
    }
    else if(action == CASTLE_Q)
    {
        board[0][7][0] = '0';
        board[0][7][1] = '0';
        board[0][3][0] = '0';
        board[0][3][1] = '0';
        board[0][5][0] = piece_color;
        board[0][5][1] = 'k';
        board[0][6][0] = piece_color;
        board[0][6][1] = 'r';
        *castle_q_flag = 1;
        *castle_k_flag = 1;
    }
    else if(move_num < 13)
    {
        move_len = move_num + 1;
        *x_to = x_from;
        *y_to = y_from + move_len;
    }
    else if(move_num >= 13 && move_num < 26)
    {
        move_len = move_num - 12;
        *x_to = x_from + move_len;
        *y_to = y_from;
    }
    else if(move_num >= 26 && move_num < 39)
    {
        move_len = move_num - 25;
        *x_to = x_from;
        *y_to = y_from - move_len;
    }
    else if(move_num >= 39 && move_num < 52)
    {
        move_len = move_num - 38;
        *x_to = x_from - move_len;
        *y_to = y_from;
    }
    else if(move_num >= 52 && move_num < 62)
    {
        move_len = move_num - 51;
        *x_to = x_from + move_len;
        *y_to = y_from + move_len;
    }
    else if(move_num >= 62 && move_num < 72)
    {
        move_len = move_num - 61;
        *x_to = x_from + move_len;
        *y_to = y_from - move_len;
    }
    else if(move_num >= 72 && move_num < 82)
    {
        move_len = move_num - 71;
        *x_to = x_from - move_len;
        *y_to = y_from - move_len;
    }
    else if(move_num >= 82 && move_num < 92)
    {
        move_len = move_num - 81;
        *x_to = x_from - move_len;
        *y_to = y_from + move_len;
    }
    else if(move_num >= 92 && move_num < 100)
    {
        int x_offsets[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
        int y_offsets[8] = {2, 2, 1, -1, -2, -2, -1, 1};
        int x_diff = x_offsets[move_num - 92];
        int y_diff = y_offsets[move_num - 92];
        *x_to = x_from + x_diff;
        *y_to = y_from + y_diff;
    }
    else if(move_num >= 100 && move_num < 109)
    {
        int promote_to_num, move_to_num;
        promote_to_num = (move_num - 100) % 3;
        move_to_num = (move_num - 100) / 3;
        switch(promote_to_num)
        {
            case 0:
                next_piece_type = 'r';
                break;
            case 1:
                next_piece_type = 'n';
                break;
            case 2:
                next_piece_type = 'b';
                break;              
        }
        switch(move_to_num)
        {
            case 0:
                *x_to = x_from - 1;
                *y_to = y_from + 1;
                break;
            case 1:
                *x_to = x_from;
                *y_to = y_from + 1;
                break;
            case 2:
                *x_to = x_from + 1;
                *y_to = y_from + 1;
                break;          
        }
    }
    if(action != CASTLE_K && action != CASTLE_Q)
    {
        if(board[*y_to][*x_to][0] != '0'|| piece_type == 'p')
            *progress_move_flag = 1;
        board[*y_to][*x_to][0] = piece_color;
        board[*y_to][*x_to][1] = next_piece_type;
    }
    if(piece_type == 'p' && ((*x_to == en_pas_x_l && *y_to == en_pas_y_l) || (*x_to == en_pas_x_r && *y_to == en_pas_y_r)))
    {
        if(*x_to > 0 && board[*y_to][*x_to - 1][1] == 'p')
        {
            board[*y_to][*x_to - 1][0] = '0';
            board[*y_to][*x_to - 1][1] = '0';
        }
        else if(*x_to < BOARD_SIZE - 1 && board[*y_to][*x_to + 1][1] == 'p')
        {
            board[*y_to][*x_to + 1][0] = '0';
            board[*y_to][*x_to + 1][1] = '0';
        }
    }
}

void write_curr_player(int *i, int *k, char *curr_player, const char text_board_representation[], char next_state[500])
{
    *curr_player = text_board_representation[*i];
    *i += 2;
    switch(*curr_player)
    {
        case 'r':
            next_state[*k] = 'b';
            break;
        case 'b':
            next_state[*k] = 'y';
            break;
        case 'y':
            next_state[*k] = 'g';
            break;
        case 'g':
            next_state[*k] = 'r';
            break;
    }
    next_state[*k + 1] = ' ';
    *k += 2;
}

void write_castle_permission(int *i, int *k, char curr_player, const char text_board_representation[], char next_state[500],
                             int castle_k_flag, int castle_q_flag)
{
    while(text_board_representation[*i] != ' ')
    {
        next_state[*k] = text_board_representation[*i];
        if(text_board_representation[*i] == curr_player)
        {
            next_state[*k + 1] = castle_k_flag ? '-': text_board_representation[*i + 1];
            next_state[*k + 2] = castle_q_flag ? '-': text_board_representation[*i + 2];
            *k += 2;
            *i += 2;
        }
        (*k)++;
        (*i)++;
    }
    (*i)++;
    next_state[*k] = ' ';
    (*k)++;
}

void write_en_pas(int *i, int *k, char curr_player, const char text_board_representation[], char next_state[500], char piece_type, int y_to, int y_from, int x_to)
{
    int en_pas_y, en_pas_x, en_pas_y_rot, en_pas_x_rot;
    while(text_board_representation[*i] != ' ')
    {
        next_state[*k] = text_board_representation[*i];
        if(text_board_representation[*i] == curr_player)
        {
            if(piece_type == 'p' && y_to - y_from == 2)
            {
                en_pas_y = y_to - 1;
                en_pas_x = x_to;
            }
            else
            {
                en_pas_y = 0;
                en_pas_x = 0;
            }
        }
        else
        {
            en_pas_y = (text_board_representation[*i + 1] - '0') * 10 + text_board_representation[*i + 2] - '0';
            en_pas_x = (text_board_representation[*i + 3] - '0') * 10 + text_board_representation[*i + 4] - '0';
        }
        en_pas_y_rot = en_pas_x;
        if(en_pas_x != 0 || en_pas_y != 0)
            en_pas_x_rot = BOARD_SIZE - 1 - en_pas_y;
        else
            en_pas_x_rot = 0;
        
        next_state[*k + 1] = en_pas_y_rot / 10 + '0';
        next_state[*k + 2] = en_pas_y_rot % 10 + '0';
        next_state[*k + 3] = en_pas_x_rot / 10 + '0';
        next_state[*k + 4] = en_pas_x_rot % 10 + '0';
        *k += 5;
        *i += 5;
    }
    next_state[*k] = ' ';
    (*k)++;
    (*i)++;
}


void increase_counter(int *i, int *k, const char text_board_representation[], char next_state[500])
{
    int counter = 0, magnitude = 1;
    while(text_board_representation[*i] != ' ' && text_board_representation[*i] != '\0')
    {
        counter = counter * 10 + text_board_representation[*i] - '0';
        (*i)++;
    }
    counter++;
    while(magnitude * 10 <= counter)
        magnitude *= 10;
    while(magnitude)
    {
        next_state[*k] = counter / magnitude + '0';
        counter = counter % magnitude;
        magnitude /= 10;
        (*k)++;
    }
}

void rotate_board(char board[BOARD_SIZE][BOARD_SIZE][2], int angle_x_90deg)
{
    int i, j;
    char tmp_board[BOARD_SIZE][BOARD_SIZE][2];
    for(i = 0; i < BOARD_SIZE; i++)
        for(j = 0; j < BOARD_SIZE; j++)
        {
            tmp_board[i][j][0] = board[i][j][0];
            tmp_board[i][j][1] = board[i][j][1];
        }
    angle_x_90deg %= 4;
    switch(angle_x_90deg)
    {
        case 1:
            for(i = 0; i < BOARD_SIZE; i++)
                for(j = 0; j < BOARD_SIZE; j++)
                {
                    board[j][BOARD_SIZE - 1 - i][0] = tmp_board[i][j][0];
                    board[j][BOARD_SIZE - 1 - i][1] = tmp_board[i][j][1];
                }
            break;
        case 2:
            for(i = 0; i < BOARD_SIZE; i++)
                for(j = 0; j < BOARD_SIZE; j++)
                {
                    board[BOARD_SIZE - 1 - i][BOARD_SIZE - 1 - j][0] = tmp_board[i][j][0];
                    board[BOARD_SIZE - 1 - i][BOARD_SIZE - 1 - j][1] = tmp_board[i][j][1];
                }
            break;
        case 3:
            for(i = 0; i < BOARD_SIZE; i++)
                for(j = 0; j < BOARD_SIZE; j++)
                {
                    board[BOARD_SIZE - 1 - j][i][0] = tmp_board[i][j][0];
                    board[BOARD_SIZE - 1 - j][i][1] = tmp_board[i][j][1];
                }
            break;
    }
}

void get_metainfo_from_text(const char text_board_representation[], int k, char *curr_player, int *r_castle_k, int *r_castle_q,
                            int *b_castle_k, int *b_castle_q, int *y_castle_k, int *y_castle_q, int *g_castle_k, int *g_castle_q,
                            int *no_progress_moves, int *move_count)
{
    int counter = 0, magnitude = 1;
    *curr_player = text_board_representation[k];
    k += 2;
    while(text_board_representation[k] != ' ')
    {
        switch(text_board_representation[k])
        {
            case 'r':
                *r_castle_k = text_board_representation[k + 1] == 'k';
                *r_castle_q = text_board_representation[k + 2] == 'q';
                break;
            case 'b':
                *b_castle_k = text_board_representation[k + 1] == 'k';
                *b_castle_q = text_board_representation[k + 2] == 'q';
                break;
            case 'y':
                *y_castle_k = text_board_representation[k + 1] == 'k';
                *y_castle_q = text_board_representation[k + 2] == 'q';
                break;
            case 'g':
                *g_castle_k = text_board_representation[k + 1] == 'k';
                *g_castle_q = text_board_representation[k + 2] == 'q';
                break;
        }
        k += 3;
    }
    k++;
    while(text_board_representation[k] != ' ')
        k++;
    k++;
    while(text_board_representation[k] != ' ' && text_board_representation[k] != '\0')
    {
        counter = counter * 10 + text_board_representation[k] - '0';
        magnitude *= 10;
        k++;
    }
    *no_progress_moves = counter;
    k++;
    while(text_board_representation[k] != ' ' && text_board_representation[k] != '\0')
    {
        counter = counter * 10 + text_board_representation[k] - '0';
        magnitude *= 10;
        k++;
    }
    *move_count = counter;
}

void text_to_nn_input(char text_board_representation[500], int nn_input[39][14][14], int repetition_count)
{
    char board[BOARD_SIZE][BOARD_SIZE][2];
    int i, j, k, n;
    int color_offset, type_offset, player_color_num;
    char player_color;
    int r_castle_k, r_castle_q,b_castle_k, b_castle_q, y_castle_k, y_castle_q, g_castle_k, g_castle_q, no_progress_moves, move_count;
    n = init_board(text_board_representation, board);
    n++;
    get_metainfo_from_text(text_board_representation, n, &player_color, &r_castle_k, &r_castle_q,
                            &b_castle_k, &b_castle_q, &y_castle_k, &y_castle_q, &g_castle_k, &g_castle_q,
                            &no_progress_moves, &move_count);

    for(i = 0; i < 14; i++)
        for(j = 0; j < 14; j++)
            for(k = 0; k < 39; k++)
                nn_input[k][i][j] = 0;
    for(i = 0; i < BOARD_SIZE; i++)
        for(j = 0; j < BOARD_SIZE; j++)
        {

            if(board[i][j][0] == '0')
                continue;
            if(board[i][j][0] == 'r')
                color_offset = 0;
            else if(board[i][j][0] == 'b')
                color_offset = 6;
            else if(board[i][j][0] == 'y')
                color_offset = 12;
            else if(board[i][j][0] == 'g')
                color_offset = 18;
            if(board[i][j][1] == 'r')
                type_offset = 0;
            else if(board[i][j][1] == 'n')
                type_offset = 1;
            else if(board[i][j][1] == 'b')
                type_offset = 2;
            else if(board[i][j][1] == 'q')
                type_offset = 3;
            else if(board[i][j][1] == 'k')
                type_offset = 4;
            else if(board[i][j][1] == 'p')
                type_offset = 5;
            nn_input[i][j][color_offset + type_offset] = 1;
        }
    if(player_color == 'r')
        player_color_num = 0;
    else if(player_color == 'b')
        player_color_num = 1;
    else if(player_color == 'y')
        player_color_num = 2;
    else if(player_color == 'g')
        player_color_num = 3;
    for(i = 0; i < BOARD_SIZE; i++)
        for(j = 0; j < BOARD_SIZE; j++)
        {
            nn_input[24 + player_color_num][i][j] = 1;
            nn_input[28][i][j] = move_count;
            if(r_castle_k)
                nn_input[29][i][j] = 1;
            if(r_castle_q)
                nn_input[30][i][j] = 1;
            if(b_castle_k)
                nn_input[31][i][j] = 1;
            if(b_castle_q)
                nn_input[32][i][j] = 1;
            if(y_castle_k)
                nn_input[33][i][j] = 1;
            if(y_castle_q)
                nn_input[34][i][j] = 1;
            if(g_castle_k)
                nn_input[35][i][j] = 1;
            if(g_castle_q)
                nn_input[36][i][j] = 1;
            nn_input[37][i][j] = no_progress_moves;
            nn_input[38][i][j] = repetition_count;
        }
}

void get_canonical_board_text(const char text_board_representation[], char canonical_board_text[500])
{
    char board[BOARD_SIZE][BOARD_SIZE][2];
    char canonical_board[BOARD_SIZE][BOARD_SIZE][2];
    int i, j;
    char player_color;
    init_board(text_board_representation, board);
    i = 0;
    while(text_board_representation[i - 1] != ' ')
        i++;
    player_color = text_board_representation[i];
    for(i = 0; i < BOARD_SIZE; i++)
        for(j = 0; j < BOARD_SIZE; j++)
        {
            canonical_board[i][j][0] = board[i][j][0];
            canonical_board[i][j][1] = board[i][j][1];
        }
    switch(player_color)
    {
        case 'b':
            rotate_board(canonical_board, 3);
            break;
        case 'y':
            rotate_board(canonical_board, 2);
            break;
        case 'g':
            rotate_board(canonical_board, 1);
            break;
    }
    board_to_text(canonical_board, canonical_board_text, 0);
}

void get_next_state(const char text_board_representation[], int action, char next_state[500], char canonical_board_text[500])
{
    char board[BOARD_SIZE][BOARD_SIZE][2];
    char canonical_board[BOARD_SIZE][BOARD_SIZE][2];
    int pinned[BOARD_SIZE][BOARD_SIZE][2];
    int i, j, k;
    int x_from, y_from, move_num;
    char piece_color, piece_type;
    int move_len;
    char curr_player;
    int castle_q_flag = 0, castle_k_flag = 0;
    int en_pas_x_l, en_pas_y_l, en_pas_x_r, en_pas_y_r;
    int castle_k, castle_q;
    int checked_directions[16][2];
    char player_color, enemy_left_color, enemy_right_color, ally_color;
    int x_to, y_to;
    int en_pas_x, en_pas_y, en_pas_x_rot, en_pas_y_rot;
    int progress_move_flag = 0;
    x_from = (action / 109) % 14;
    y_from = (action / 109) / 14;
    move_num = action % 109;
    //init_board(text_board_representation, board)
    init(text_board_representation, board, pinned, checked_directions, &player_color,
         &ally_color, &enemy_left_color, &enemy_right_color, &en_pas_x_l, &en_pas_y_l, &en_pas_x_r, &en_pas_y_r, &castle_k, &castle_q);
    piece_color = player_color;
    piece_type = board[y_from][x_from][1];
    change_board(piece_type, piece_color, board, x_from, y_from, move_num, action,
                 &x_to, &y_to, &castle_k_flag, &castle_q_flag, &progress_move_flag, en_pas_x_l, en_pas_y_l, en_pas_x_r, en_pas_y_r);
    //draw_board(board);
    for(i = 0; i < BOARD_SIZE; i++)
        for(j = 0; j < BOARD_SIZE; j++)
        {
            canonical_board[i][j][0] = board[i][j][0];
            canonical_board[i][j][1] = board[i][j][1];
        }
    switch(player_color)
    {
        case 'b':
            rotate_board(canonical_board, 3);
            break;
        case 'y':
            rotate_board(canonical_board, 2);
            break;
        case 'g':
            rotate_board(canonical_board, 1);
            break;
    }
    board_to_text(canonical_board, canonical_board_text, 0);
    k = board_to_text(board, next_state, 1);
    next_state[k - 1] = ' ';
    i = 1;
    while(text_board_representation[i - 1] != ' ')
        i++;
    
    write_curr_player(&i, &k, &curr_player, text_board_representation, next_state);
    
    write_castle_permission(&i, &k, curr_player, text_board_representation, next_state,
                            castle_k_flag, castle_q_flag);
    
    write_en_pas(&i, &k, curr_player, text_board_representation, next_state, piece_type, y_to, y_from, x_to);
    
    if(progress_move_flag)
    {
        while(text_board_representation[i] != ' ')
            i++;
        i++;
        next_state[k] = '0';
        next_state[k + 1] = ' ';
        k += 2;
    }
    else
    {
        increase_counter(&i, &k, text_board_representation, next_state);
        i++;
        next_state[k] = ' ';
        k++;
    }
    increase_counter(&i, &k, text_board_representation, next_state);
    next_state[k] = '\0';
}

void print_action(const char text_board_representation[], int action)
{
    char board[BOARD_SIZE][BOARD_SIZE][2];
    int x_from, y_from, move_num;
    int x_to, y_to;
    char piece_color, piece_type, player_color, next_piece_type;
    int i, move_len;

    init_board(text_board_representation, board);

    x_from = (action / 109) % 14;
    y_from = (action / 109) / 14;
    move_num = action % 109;

    i = 1;
    while(text_board_representation[i - 1] != ' ')
        i++;
    player_color = text_board_representation[i];

    piece_color = player_color;
    piece_type = board[y_from][x_from][1];
    next_piece_type = piece_type;

    if(y_from == 9 && move_num < 100 && piece_type == 'p')
        next_piece_type = 'q';
    
    if(action == CASTLE_K)
    {
        printf("%c O-O\n", piece_color);
        return;
    }
    else if(action == CASTLE_Q)
    {
        printf("%c O--O\n", piece_color);
        return;
    }
    else if(move_num < 13)
    {
        move_len = move_num + 1;
        x_to = x_from;
        y_to = y_from + move_len;
    }
    else if(move_num >= 13 && move_num < 26)
    {
        move_len = move_num - 12;
        x_to = x_from + move_len;
        y_to = y_from;
    }
    else if(move_num >= 26 && move_num < 39)
    {
        move_len = move_num - 25;
        x_to = x_from;
        y_to = y_from - move_len;
    }
    else if(move_num >= 39 && move_num < 52)
    {
        move_len = move_num - 38;
        x_to = x_from - move_len;
        y_to = y_from;
    }
    else if(move_num >= 52 && move_num < 62)
    {
        move_len = move_num - 51;
        x_to = x_from + move_len;
        y_to = y_from + move_len;
    }
    else if(move_num >= 62 && move_num < 72)
    {
        move_len = move_num - 61;
        x_to = x_from + move_len;
        y_to = y_from - move_len;
    }
    else if(move_num >= 72 && move_num < 82)
    {
        move_len = move_num - 71;
        x_to = x_from - move_len;
        y_to = y_from - move_len;
    }
    else if(move_num >= 82 && move_num < 92)
    {
        move_len = move_num - 81;
        x_to = x_from - move_len;
        y_to = y_from + move_len;
    }
    else if(move_num >= 92 && move_num < 100)
    {
        int x_offsets[8] = {-1, 1, 2, 2, 1, -1, -2, -2};
        int y_offsets[8] = {2, 2, 1, -1, -2, -2, -1, 1};
        int x_diff = x_offsets[move_num - 92];
        int y_diff = y_offsets[move_num - 92];
        x_to = x_from + x_diff;
        y_to = y_from + y_diff;
    }
    else if(move_num >= 100 && move_num < 109)
    {
        int promote_to_num, move_to_num;
        promote_to_num = (move_num - 100) % 3;
        move_to_num = (move_num - 100) / 3;
        switch(promote_to_num)
        {
            case 0:
                next_piece_type = 'r';
                break;
            case 1:
                next_piece_type = 'n';
                break;
            case 2:
                next_piece_type = 'b';
                break;              
        }
        switch(move_to_num)
        {
            case 0:
                x_to = x_from - 1;
                y_to = y_from + 1;
                break;
            case 1:
                x_to = x_from;
                y_to = y_from + 1;
                break;
            case 2:
                x_to = x_from + 1;
                y_to = y_from + 1;
                break;          
        }
    }
    if(piece_type == next_piece_type)
        printf("%c %c (%d %d)->(%d %d)\n", piece_color, piece_type, x_from, y_from, x_to, y_to);
    else
        printf("%c %c promote to %c (%d %d)->(%d %d)\n", piece_color, piece_type, next_piece_type,
                x_from, y_from, x_to, y_to);
}
