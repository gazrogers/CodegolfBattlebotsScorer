#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMBOTS 2
#define BOUTSPERMATCH 5
#define ROUNDSPERBOUT 1
#define MAXFILENAMESIZE 100
#define MAXWEAPONS 100

void outputarena(char arena[10][10]);

int main()
{
    int bot1,bot2,bout,round,bot1x,bot1y,bot2x,bot2y,bot1energy,bot2energy;
    int loop;
    char bots[NUMBOTS][MAXFILENAMESIZE]={"./donowt", "php -f huggybot.php"};
    char argumentstring[1000];
    int matcheswon[NUMBOTS],boutswon[NUMBOTS];
    int missiles[MAXWEAPONS][3]={-1};
    int bullets[MAXWEAPONS][3]={-1};
    int landmines[MAXWEAPONS][2]={-1};
    int paralyzedturnsremaining=0;
    char arena[10][10];

    memset(missiles, -1, sizeof(missiles));
    memset(bullets, -1, sizeof(bullets));
    memset(landmines, -1, sizeof(landmines));
    srand(time(NULL));

    for(bot1=0;bot1<NUMBOTS-1;bot1++)
    {
        for(bot2=bot1+1;bot2<NUMBOTS;bot2++)
        {
            printf("%s vs %s ",bots[bot1],bots[bot2]);
            for(bout=0;bout<BOUTSPERMATCH;bout++)
            {
                printf("Bout %d\n",bout);
                //setup the arena for the bout
                bot1x=0;
                bot2x=9;
                bot1y=rand()%10;
                bot2y=rand()%10;
                bot1energy=bot2energy=10;
                for(round=0;round<ROUNDSPERBOUT;round++)
                {
                    //draw the arena based on current state
                    memset(arena, '.', sizeof(arena));
                    for(loop=0;loop<MAXWEAPONS;loop++)
                    {
                        if(missiles[loop][0]!= -1)
                        {
                            arena[missiles[loop][0]][missiles[loop][1]]='M';
                        }
                        if(bullets[loop][0]!= -1)
                        {
                            arena[bullets[loop][0]][bullets[loop][1]]='B';
                        }
                        if(landmines[loop][0]!= -1)
                        {
                            arena[landmines[loop][0]][landmines[loop][1]]='L';
                        }
                    }
                    
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

void outputarena(char arena[10][10])
{
    int i,j;
    for(j=0;j<10;j++)
    {
        for(i=0;i<10;i++)
        {
            printf("%c", arena[i][j]);
        }
        printf("\n");
    }
}