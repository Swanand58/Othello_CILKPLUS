#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cilk/cilk.h>
#include <cilk/reducer_max.h>
#include <climits>

#define BIT 0x1

#define X_BLACK 0
#define O_WHITE 1
#define OTHERCOLOR(c) (1-(c))

/* 
	represent game board squares as a 64-bit unsigned integer.
	these macros index from a row,column position on the board
	to a position and bit in a game board bitvector
*/
#define BOARD_BIT_INDEX(row,col) ((8 - (row)) * 8 + (8 - (col)))
#define BOARD_BIT(row,col) (0x1LL << BOARD_BIT_INDEX(row,col))
#define MOVE_TO_BOARD_BIT(m) BOARD_BIT(m.row, m.col)

/* all of the bits in the row 8 */
#define ROW8 \
  (BOARD_BIT(8,1) | BOARD_BIT(8,2) | BOARD_BIT(8,3) | BOARD_BIT(8,4) |	\
   BOARD_BIT(8,5) | BOARD_BIT(8,6) | BOARD_BIT(8,7) | BOARD_BIT(8,8))
			  
/* all of the bits in column 8 */
#define COL8 \
  (BOARD_BIT(1,8) | BOARD_BIT(2,8) | BOARD_BIT(3,8) | BOARD_BIT(4,8) |	\
   BOARD_BIT(5,8) | BOARD_BIT(6,8) | BOARD_BIT(7,8) | BOARD_BIT(8,8))

/* all of the bits in column 1 */
#define COL1 (COL8 << 7)

#define IS_MOVE_OFF_BOARD(m) (m.row < 1 || m.row > 8 || m.col < 1 || m.col > 8)
#define IS_DIAGONAL_MOVE(m) (m.row != 0 && m.col != 0)
#define MOVE_OFFSET_TO_BIT_OFFSET(m) (m.row * 8 + m.col)

typedef unsigned long long ull;

/* 
	game board represented as a pair of bit vectors: 
	- one for x_black disks on the board
	- one for o_white disks on the board
*/
typedef struct { ull disks[2]; } Board;

typedef struct { int row; int col; } Move;

Board start = { 
	BOARD_BIT(4,5) | BOARD_BIT(5,4) /* X_BLACK */, 
	BOARD_BIT(4,4) | BOARD_BIT(5,5) /* O_WHITE */
};
 
Move offsets[] = {
  {0,1}		/* right */,		{0,-1}		/* left */, 
  {-1,0}	/* up */,		{1,0}		/* down */, 
  {-1,-1}	/* up-left */,		{-1,1}		/* up-right */, 
  {1,1}		/* down-right */,	{1,-1}		/* down-left */
};

int noffsets = sizeof(offsets)/sizeof(Move);
char diskcolor[] = { '.', 'X', 'O', 'I' };


void PrintDisk(int x_black, int o_white)
{
  printf(" %c", diskcolor[x_black + (o_white << 1)]);
}

void PrintBoardRow(int x_black, int o_white, int disks)
{
  if (disks > 1) {
    PrintBoardRow(x_black >> 1, o_white >> 1, disks - 1);
  }
  PrintDisk(x_black & BIT, o_white & BIT);
}

void PrintBoardRows(ull x_black, ull o_white, int rowsleft)
{
  if (rowsleft > 1) {
    PrintBoardRows(x_black >> 8, o_white >> 8, rowsleft - 1);
  }
  printf("%d", rowsleft);
  PrintBoardRow((int)(x_black & ROW8),  (int) (o_white & ROW8), 8);
  printf("\n");
}

void PrintBoard(Board b)
{
  printf("  1 2 3 4 5 6 7 8\n");
  PrintBoardRows(b.disks[X_BLACK], b.disks[O_WHITE], 8);
}

/* 
	place a disk of color at the position specified by m.row and m,col,
	flipping the opponents disk there (if any) 
*/
void PlaceOrFlip(Move m, Board *b, int color) 
{
  ull bit = MOVE_TO_BOARD_BIT(m);
  b->disks[color] |= bit;
  b->disks[OTHERCOLOR(color)] &= ~bit;
}

/* 
	try to flip disks along a direction specified by a move offset.
	the return code is 0 if no flips were done.
	the return value is 1 + the number of flips otherwise.
*/
int TryFlips(Move m, Move offset, Board *b, int color, int verbose, int domove)
{
  Move next;
  next.row = m.row + offset.row;
  next.col = m.col + offset.col;

  if (!IS_MOVE_OFF_BOARD(next)) {
    ull nextbit = MOVE_TO_BOARD_BIT(next);
    if (nextbit & b->disks[OTHERCOLOR(color)]) {
      int nflips = TryFlips(next, offset, b, color, verbose, domove);
      if (nflips) {
	if (verbose) printf("flipping disk at %d,%d\n", next.row, next.col);
	if (domove) PlaceOrFlip(next,b,color);
	return nflips + 1;
      }
    } else if (nextbit & b->disks[color]) return 1;
  }
  return 0;
} 

int FlipDisks(Move m, Board *b, int color, int verbose, int domove)
{
  int i;
  int nflips = 0;
	
  /* try flipping disks along each of the 8 directions */
  for(i=0;i<noffsets;i++) {
    int flipresult = TryFlips(m,offsets[i], b, color, verbose, domove);
    nflips += (flipresult > 0) ? flipresult - 1 : 0;
  }
  return nflips;
}

void ReadMove(int color, Board *b)
{
  Move m;
  ull movebit;
  for(;;) {
    printf("Enter %c's move as 'row,col': ", diskcolor[color+1]);
    scanf("%d,%d",&m.row,&m.col);
		
    /* if move is not on the board, move again */
    if (IS_MOVE_OFF_BOARD(m)) {
      printf("Illegal move: row and column must both be between 1 and 8\n");
      PrintBoard(*b);
      continue;
    }
    movebit = MOVE_TO_BOARD_BIT(m);
		
    /* if board position occupied, move again */
    if (movebit & (b->disks[X_BLACK] | b->disks[O_WHITE])) {
      printf("Illegal move: board position already occupied.\n");
      PrintBoard(*b);
      continue;
    }
		
    /* if no disks have been flipped */ 
    {
      int nflips = FlipDisks(m, b,color, 1, 1);
      if (nflips == 0) {
	printf("Illegal move: no disks flipped\n");
	PrintBoard(*b);
	continue;
      }
      PlaceOrFlip(m, b, color);
      printf("You flipped %d disks\n", nflips);
      PrintBoard(*b);
    }
    break;
  }
}

/*
	return the set of board positions adjacent to an opponent's
	disk that are empty. these represent a candidate set of 
	positions for a move by color.
*/
Board NeighborMoves(Board b, int color)
{
  int i;
  Board neighbors = {0,0};
  for (i = 0;i < noffsets; i++) {
    ull colmask = (offsets[i].col != 0) ? 
      ((offsets[i].col > 0) ? COL1 : COL8) : 0;
    int offset = MOVE_OFFSET_TO_BIT_OFFSET(offsets[i]);

    if (offset > 0) {
      neighbors.disks[color] |= 
	(b.disks[OTHERCOLOR(color)] >> offset) & ~colmask;
    } else {
      neighbors.disks[color] |= 
	(b.disks[OTHERCOLOR(color)] << -offset) & ~colmask;
    }
  }
  neighbors.disks[color] &= ~(b.disks[X_BLACK] | b.disks[O_WHITE]);
  return neighbors;
}

/*
	return the set of board positions that represent legal
	moves for color. this is the set of empty board positions  
	that are adjacent to an opponent's disk where placing a
	disk of color will cause one or more of the opponent's
	disks to be flipped.
*/
int EnumerateLegalMoves(Board b, int color, Board *legal_moves)
{
  static Board no_legal_moves = {0,0};
  Board neighbors = NeighborMoves(b, color);
  ull my_neighbor_moves = neighbors.disks[color];
  int row;
  int col;
	
  int num_moves = 0;
  *legal_moves = no_legal_moves;
	
  for(row=8; row >=1; row--) {
    ull thisrow = my_neighbor_moves & ROW8;
    for(col=8; thisrow && (col >= 1); col--) {
      if (thisrow & COL8) {
	Move m = { row, col };
	if (FlipDisks(m, &b, color, 0, 0) > 0) {
	  legal_moves->disks[color] |= BOARD_BIT(row,col);
	  num_moves++;
	}
      }
      thisrow >>= 1;
    }
    my_neighbor_moves >>= 8;
  }
  return num_moves;
}

int HumanTurn(Board *b, int color)
{
  Board legal_moves;
  int num_moves = EnumerateLegalMoves(*b, color, &legal_moves);
  if (num_moves > 0) {
    ReadMove(color, b);
    return 1;
  } else return 0;
}

// function to check if the game is over/concluded
bool isGameOver(Board b)
{
  //this loop checks if the board is fully occupied with all bits set to 1
  if((b.disks[X_BLACK] | b.disks[O_WHITE]) == 0xFFFFFFFFFFFFFFFF)
  {
    return true;
  }

  Board legal_moves_b;
  Board legal_moves_w;
  
  // We will use the given EnumerateLegalMoves function to check if both our players dont have legal move our game will end.
  if (EnumerateLegalMoves(b, X_BLACK, &legal_moves_b) == 0 && EnumerateLegalMoves(b, O_WHITE, &legal_moves_w) == 0)
  { 
    return true;
  }

  return false;

}

// This function will make a move in the game for a player at a specific location. It will make use of the already
// given functions PlaceorFlip() and FlipDisks().
void MakeMove(Board* b, Move move, int color) {
    PlaceOrFlip(move, b, color);
    FlipDisks(move, b, color, 0, 1);
}

int CountBitsOnBoard(Board *b, int color)
{
  ull bits = b->disks[color];
  int ndisks = 0;
  for (; bits ; ndisks++) {
    bits &= bits - 1; // clear the least significant bit set
  }
  return ndisks;
}

// This function will calculate a heuristic score by comparing the number of disks on the board
//positive count value means opponent has less disks, negative value means opponent has more disks and zero count
// value means equal number of disks.
int EvaluateBoard(Board b, int color)
{
  int count_a = CountBitsOnBoard(&b, color);
  int count_b = CountBitsOnBoard(&b, OTHERCOLOR(color)); 

  return count_a - count_b;
}

void EndGame(Board b)
{
  int o_score = CountBitsOnBoard(&b,O_WHITE);
  int x_score = CountBitsOnBoard(&b,X_BLACK);
  printf("Game over. \n");
  if (o_score == x_score)  {
    printf("Tie game. Each player has %d disks\n", o_score);
  } else { 
    printf("X has %d disks. O has %d disks. %c wins.\n", x_score, o_score, 
	      (x_score > o_score ? 'X' : 'O'));
  }
}

//This algorithm is the variant of minimax, where one player's gain is the other player's loss.
int NegaMaxAlgo(Board b, int color, int depth)
{
  // Base case if the search has reached maximum depth or if the game is over we will return the score.
    if (depth == 0 || isGameOver(b)){
      return EvaluateBoard(b, color);
    }

    Board legal_moves;

    int num_moves = EnumerateLegalMoves(b, color, &legal_moves);

    if(num_moves == 0)
    {
      return - NegaMaxAlgo(b, OTHERCOLOR(color), depth - 1);
    }

    // int maxScore = INT_MIN;

    // cilk reducer for maxscore.
    cilk::reducer_max<int> maxScore(INT_MIN);

    cilk_for(int row = 8; row >= 1; row--)
    {
      for (int col = 8; col >= 1; col--)
      {
        if(legal_moves.disks[color] & BOARD_BIT(row, col)){
          Board next = b;
          Move m1 = {row, col};
          MakeMove(&next, m1 , color);
          int score = -NegaMaxAlgo(next, OTHERCOLOR(color), depth - 1); // here we will recurssively call negamax to go deeper into the game tree
          maxScore.calc_max(score); // keep track of the max score
          // if(score >= maxScore){
          //   maxScore = score;
          // }
        }
      }
    }
    return maxScore.get_value(); // return the best score.
}

//the computer turn will use the negamax algorithm to select the best possible move
int CompTurn(Board *b, int color, int depth) {

    int bestScore = INT_MIN;
    Move bestMove = {-1, -1};
    Board legal_moves;

    int num_moves = EnumerateLegalMoves(*b, color, &legal_moves);
    if (num_moves == 0) {
        printf("Computer has no valid moves.\n");
        return 0;
    }

    for (int row = 8; row >= 1; row--) {
        for (int col = 8; col >= 1; col--) {
            if (legal_moves.disks[color] & BOARD_BIT(row, col)) {
                Board nextBoard = *b;
                Move m2 = {row, col};
                MakeMove(&nextBoard, m2, color);
                int score = -NegaMaxAlgo(nextBoard, OTHERCOLOR(color), depth - 1);
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = m2;
                }
            }
        }
    }
    if (bestMove.row != -1 && bestMove.col != -1) {
        printf("Computer places %c at %d, %d .\n",diskcolor[color+1], bestMove.row, bestMove.col);
        PlaceOrFlip(bestMove, b, color);
        FlipDisks(bestMove, b, color, 0, 1);
        PrintBoard(*b);
    } else {
        printf("Computer has no valid moves.\n");
    }
  return 1;
}

int main (int argc, const char * argv[]) 
{
  Board gameboard = start;
  int move_possible;
  char player1, player2;
  int depth1, depth2;

  printf("Enter the first player h for human, c for computer: ");
  scanf(" %c", &player1);
  printf("Enter depth of the first player: ");
  scanf("%d",&depth1);
  printf("Enter the second player h for human, c for computer: ");
  scanf(" %c", &player2);
  printf("Enter depth of the second player: ");
  scanf("%d", &depth2);

  PrintBoard(gameboard);
  do {

    if(player1 == 'c'){
      move_possible = CompTurn(&gameboard, X_BLACK, depth1);
    } 
    else if(player1 == 'h'){
      move_possible = HumanTurn(&gameboard, X_BLACK);
    }

    if(isGameOver(gameboard)) break;

    if(player2 == 'c'){
      move_possible |= CompTurn(&gameboard, O_WHITE, depth2);
    } 
    else if (player2 == 'h'){
      move_possible |= HumanTurn(&gameboard, O_WHITE);
    }

    // move_possible = 
    //   // HumanTurn(&gameboard, X_BLACK) 
    //   CompTurn(&gameboard, X_BLACK, depth)| 
    //   CompTurn(&gameboard, O_WHITE, depth);
  } while(move_possible);
	
  EndGame(gameboard);
	
  return 0;
}
