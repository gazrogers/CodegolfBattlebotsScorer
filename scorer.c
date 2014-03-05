#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMBOTS 2
#define BOUTSPERMATCH 5
#define ROUNDSPERBOUT 1000
#define MAXFILENAMESIZE 100

int main()
{
    int bot1,bot2,bout,round,bot1x,bot1y,bot2x,bot2y,bot1energy,bot2energy;
    char bots[NUMBOTS][MAXFILENAMESIZE]={"./donowt", "php -f huggybot.php"};
    int matcheswon[NUMBOTS],boutswon[NUMBOTS];
    int missiles[100][3];
    int bullets[100][3];
    char blankarena[10][10]={'.'};
    char arena[10][10];

    srand(time(NULL));

    for(bot1=0;bot1<NUMBOTS-1;bot1++)
    {
        for(bot2=bot1+1;bot2<NUMBOTS;bot2++)
        {
            printf("%s vs %s ",bots[bot1],bots[bot2]);
            for(bout=0;bout<BOUTSPERMATCH;bout++)
            {
                printf("%d ",bout);
                //setup the arena for the bout
                memcpy(arena, blankarena, sizeof(a));
                bot1x=0;
                bot2x=9;
                bot1y=rand()%10;
                bot2y=rand()%10;
                bot1energy=bot2energy=10;
                for(round=0;round<ROUNDSPERBOUT;round++)
                {
//draw the arena based on current state
//send the arena to both bots to get the commands
//interpret commands
//do bot movement phase
//do weapons movement phase
//check if there's a winner
                }
            }
            printf("\n");
        }
    }
}
