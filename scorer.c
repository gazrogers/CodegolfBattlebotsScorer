#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NUMBOTS 2
#define BOUTSPERMATCH 1
#define ROUNDSPERBOUT 3
#define MAXFILENAMESIZE 100
#define MAXWEAPONS 100

typedef struct
{
    int x, y, energy;
    char cmd[5];
} Bot;

void cleararena(char arena[10][11]);
int getxmove(char cmd[5]);
int getymove(char cmd[5]);
int newposinbounds(int oldx, int oldy, int dx, int dy);
int directhit(Bot bot, int landmine[2]);
int inshrapnelrange(Bot bot, int landmine[2]);

int main()
{
    FILE *fp;
    Bot b1, b2;
    int bot1,bot2;
    int bout,round,loop,totalprojectiles,dx,dy;
    char bots[NUMBOTS][MAXFILENAMESIZE]={"./donowt", "php -f huggybot.php"};
    char directions[8][3]={"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    char openstring[5000], argumentstring[4000], bot1string[6], bot2string[6];
    int matcheswon[NUMBOTS],boutswon[NUMBOTS];
    int missiles[MAXWEAPONS][3]={-1};
    int bullets[MAXWEAPONS][3]={-1};
    int landmines[MAXWEAPONS][2]={-1};
    char movequeue[2][3]={"", ""};
    int paralyzedturnsremaining=0;
    bool bot1moved;
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
                // b1.x=0;
                // b2.x=9;
                // b1.y=rand()%10;
                // b2.y=rand()%10;
                //test setup
                b1.x=3;
                b1.y=3;
                b2.x=5;
                b2.y=5;
                b1.energy=b2.energy=10;
                //dummy projectiles for testing
                // bullets[0][0]=5;bullets[0][1]=3;bullets[0][2]=0;
                // bullets[1][0]=6;bullets[1][1]=3;bullets[1][2]=0;
                // bullets[2][0]=9;bullets[2][1]=3;bullets[2][2]=0;
                // missiles[0][0]=5;missiles[0][1]=5;missiles[0][2]=5;
                landmines[0][0]=4;landmines[0][1]=4;
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
                    arena[b1.y][b1.x]='Y';
                    arena[b2.y][b2.x]='X';
                    sprintf(bot1string, "Y %d\n", b1.energy);
                    sprintf(bot2string, "X %d\n", b2.energy);
                    strcpy(argumentstring, "'");
                    strcat(argumentstring, arena);
                    strcat(argumentstring, bot1string);
                    strcat(argumentstring, bot2string);
                    for(loop=0;loop<totalprojectiles;loop++)
                    {
                        strcat(argumentstring, projectiles[loop]);
                    }
                    strcat(argumentstring, "'");
                    sprintf(openstring, "%s %s", bots[bot1], argumentstring);
printf("%s\n", openstring);
                    // send it and get the command back
                    fp=popen(openstring, "r");
                    fgets(b1.cmd, 5, fp);
                    fflush(NULL);
                    pclose(fp);
printf("\nCommand: %s\n", b1.cmd);
                    // create bot2's input
                    arena[b2.y][b2.x]='Y';
                    arena[b1.y][b1.x]='X';
                    sprintf(bot2string, "Y %d\n", b2.energy);
                    sprintf(bot1string, "X %d\n", b1.energy);
                    strcpy(argumentstring, "'");
                    strcat(argumentstring, arena);
                    strcat(argumentstring, bot2string);
                    strcat(argumentstring, bot1string);
                    for(loop=0;loop<totalprojectiles;loop++)
                    {
                        strcat(argumentstring, projectiles[loop]);
                    }
                    strcat(argumentstring, "'");
                    sprintf(openstring, "%s %s", bots[bot2], argumentstring);
printf("%s\n", openstring);
                    // send it and get the command back
                    fp=popen(openstring, "r");
                    fgets(b2.cmd,5,fp);
                    fflush(NULL);
                    pclose(fp);
printf("\nCommand: %s\n", b2.cmd);
//interpret commands
//do bot movement phase
                    // move bot 1 first
                    bot1moved=false;
                    dx=dy=0;
                    dx=getxmove(b1.cmd);
                    dy=getymove(b1.cmd);
                    if(newposinbounds(b1.x, b1.y, dx,dy))
                    {
                        if(!(b1.x+dx==b2.x) || !(b1.y+dy==b2.y))
                        {
                            bot1moved=true;
                            b1.x=b1.x+dx;
                            b1.y=b1.y+dy;
                        }
                    }
                    // move bot 2 next
                    dx=dy=0;
                    dx=getxmove(b2.cmd);
                    dy=getymove(b2.cmd);
                    if(newposinbounds(b2.x, b2.y, dx,dy))
                    {
                        if(!(b2.x+dx==b1.x) || !(b2.y+dy==b1.y))
                        {
                            b2.x=b2.x+dx;
                            b2.y=b2.y+dy;
                        }
                    }
                    if(!bot1moved) // if bot2 was in the way first time, try again
                    {
                        dx=dy=0;
                        dx=getxmove(b1.cmd);
                        dy=getymove(b1.cmd);
                        if(newposinbounds(b1.x, b1.y, dx,dy))
                        {
                            if(!(b1.x+dx==b2.x) || !(b1.y+dy==b2.y))
                            {
                                b1.x=b1.x+dx;
                                b1.y=b1.y+dy;
                            }
                        }
                    }
                    //check for landmine hits
                    for(loop=0;loop<MAXWEAPONS;loop++)
                    {
                        if(landmines[loop][0]!= -1)
                        {
                            if(directhit(b1, landmines[loop]))
                            {
                                b1.energy-=2;
                                if(inshrapnelrange(b2, landmines[loop]))
                                {
                                    b2.energy-=1;
                                }
                                landmines[loop][0]= -1;
                                landmines[loop][1]= -1;
                            }
                            if(directhit(b2, landmines[loop]))
                            {
                                b2.energy-=2;
                                if(inshrapnelrange(b1, landmines[loop]))
                                {
                                    b1.energy-=1;
                                }
                                landmines[loop][0]= -1;
                                landmines[loop][1]= -1;
                            }
                        }
                    }
//do weapons movement phase
//check if there's a winner
                }
            }
            //printf("\n");
        }
    }
}
int getxmove(char cmd[5])
{
    int dx=0;
    if(strcmp(cmd, "NE")==0)
        dx= 1;
    else if(strcmp(cmd, "E")==0)
        dx= 1;
    else if(strcmp(cmd, "SE")==0)
        dx= 1;
    else if(strcmp(cmd, "SW")==0)
        dx= -1;
    else if(strcmp(cmd, "W")==0)
        dx= -1;
    else if(strcmp(cmd, "NW")==0)
        dx= -1;

    return dx;
}
int getymove(char cmd[5])
{
    int dy=0;
    if(strcmp(cmd, "N")==0)
        dy= -1;
    else if(strcmp(cmd, "NE")==0)
        dy= -1;
    else if(strcmp(cmd, "SE")==0)
        dy= 1;
    else if(strcmp(cmd, "S")==0)
        dy= 1;
    else if(strcmp(cmd, "SW")==0)
        dy= 1;
    else if(strcmp(cmd, "NW")==0)
        dy= -1;

    return dy;
}
int newposinbounds(int oldx, int oldy, int dx, int dy)
{
    return (oldx+dx>=0 && oldx+dx<10 && oldy+dy>=0 && oldy+dy<10);
}
int directhit(Bot bot, int landmine[2])
{
    return (bot.x==landmine[1] && bot.y==landmine[0]);
}
int inshrapnelrange(Bot bot, int landmine[2])
{
    return (abs(bot.x-landmine[1])<2 && abs(bot.y-landmine[0])<2);
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
