#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMBOTS 2
#define BOUTSPERMATCH 1
#define ROUNDSPERBOUT 1
#define MAXFILENAMESIZE 100
#define MAXWEAPONS 100

void cleararena(char arena[10][11]);

int main()
{
    FILE *fp;
    int bot1,bot2,bot1x,bot1y,bot2x,bot2y,bot1energy,bot2energy;
    int bout,round,loop,totalprojectiles;
    char bots[NUMBOTS][MAXFILENAMESIZE]={"./donowt", "php -f huggybot.php"};
    char directions[8][3]={"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    char openstring[5000], argumentstring[4000], bot1string[6], bot2string[6], bot1cmd[5], bot2cmd[5];
    int matcheswon[NUMBOTS],boutswon[NUMBOTS];
    int missiles[MAXWEAPONS][3]={-1};
    int bullets[MAXWEAPONS][3]={-1};
    int landmines[MAXWEAPONS][2]={-1};
    int paralyzedturnsremaining=0;
    char arena[10][11];
    char projectiles[300][10];

    memset(missiles, -1, sizeof(missiles));
    memset(bullets, -1, sizeof(bullets));
    memset(landmines, -1, sizeof(landmines));
    srand(time(NULL));

    for(bot1=0;bot1<NUMBOTS-1;bot1++)
    {
        for(bot2=bot1+1;bot2<NUMBOTS;bot2++)
        {
            //printf("%s vs %s ",bots[bot1],bots[bot2]);
            for(bout=0;bout<BOUTSPERMATCH;bout++)
            {
                //printf("Bout %d\n",bout);
                //setup the arena for the bout
                bot1x=0;
                bot2x=9;
                bot1y=rand()%10;
                bot2y=rand()%10;
                bot1energy=bot2energy=10;
                //dummy projectiles for testing
                bullets[0][0]=5;bullets[0][1]=3;bullets[0][2]=0;
                bullets[1][0]=6;bullets[1][1]=3;bullets[1][2]=0;
                bullets[2][0]=9;bullets[2][1]=3;bullets[2][2]=0;
                missiles[0][0]=5;missiles[0][1]=5;missiles[0][2]=5;
                landmines[0][0]=5;landmines[0][1]=7;
                for(round=0;round<ROUNDSPERBOUT;round++)
                {
                    //draw the arena based on current state
                    cleararena(arena);
                    totalprojectiles=0;
                    for(loop=0;loop<MAXWEAPONS;loop++)
                    {
                        if(bullets[loop][0]!= -1)
                        {
                            arena[bullets[loop][1]][bullets[loop][0]]='B';
                            sprintf(projectiles[totalprojectiles], "%c %d %d %s\n", 'B', bullets[loop][0], bullets[loop][1], directions[bullets[loop][2]]);
                            totalprojectiles+=1;
                        }
                        if(missiles[loop][0]!= -1)
                        {
                            arena[missiles[loop][1]][missiles[loop][0]]='M';
                            sprintf(projectiles[totalprojectiles], "%c %d %d %s\n", 'M', missiles[loop][0], missiles[loop][1], directions[missiles[loop][2]]);
                            totalprojectiles+=1;
                        }
                        if(landmines[loop][0]!= -1)
                        {
                            arena[landmines[loop][1]][landmines[loop][0]]='L';
                            sprintf(projectiles[totalprojectiles], "%c %d %d\n", 'L', landmines[loop][0], landmines[loop][1]);
                            totalprojectiles+=1;
                        }
                    }

//send the arena to both bots to get the commands
                    // create bot1's input
                    arena[bot1y][bot1x]='Y';
                    arena[bot2y][bot2x]='X';
                    sprintf(bot1string, "Y %d\n", bot1energy);
                    sprintf(bot2string, "X %d\n", bot2energy);
                    strcpy(argumentstring, "'");
                    strcat(argumentstring, arena);
                    strcat(argumentstring, bot1string);
                    strcat(argumentstring, bot2string);
                    for(loop=0;loop<totalprojectiles;loop++)
                    {
                        strcat(argumentstring, projectiles[loop]);
                    }
                    strcat(argumentstring, "'");
                    sprintf(openstring,"%s %s",bots[bot1],argumentstring);
printf("%s\n",openstring);
                    fp=popen(openstring,"r");

                    fgets(bot1cmd,5,fp);
                    fflush(NULL);
                    pclose(fp);
printf("\nCommand: %s\n", bot1cmd);
                    // create bot2's input
                    arena[bot2y][bot2x]='Y';
                    arena[bot1y][bot1x]='X';
                    sprintf(bot2string, "Y %d\n", bot2energy);
                    sprintf(bot1string, "X %d\n", bot1energy);
                    strcpy(argumentstring, "'");
                    strcat(argumentstring, arena);
                    strcat(argumentstring, bot2string);
                    strcat(argumentstring, bot1string);
                    for(loop=0;loop<totalprojectiles;loop++)
                    {
                        strcat(argumentstring, projectiles[loop]);
                    }
                    strcat(argumentstring, "'");
                    sprintf(openstring,"%s %s",bots[bot2],argumentstring);
printf("%s\n",openstring);
                    fp=popen(openstring,"r");

                    fgets(bot2cmd,5,fp);
                    fflush(NULL);
                    pclose(fp);
printf("\nCommand: %s\n", bot2cmd);
//interpret commands
//do bot movement phase
//do weapons movement phase
//check if there's a winner
                }
            }
            //printf("\n");
        }
    }
}
void cleararena(char arena[10][11])
{
    int loop;
    memset(arena, '.', 110);
    for(loop=0;loop<10;loop++)
    {
        arena[loop][10]='\n';
    }
}
