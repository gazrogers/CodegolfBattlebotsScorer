#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NUMBOTS 2
#define BOUTSPERMATCH 5
#define ROUNDSPERBOUT 1000
#define MAXFILENAMESIZE 100
#define MAXWEAPONS 100
#define DISPLAYBOUTS true

typedef struct
{
    int x, y, energy;
    char cmd[5];
} Bot;

int getxmove(char cmd[5]);
int getymove(char cmd[5]);
int newposinbounds(int oldx, int oldy, int dx, int dy);
int directhit(Bot bot, int landmine[2]);
int landminecollision(int landmine1[2], int landmine2[2]);
int inshrapnelrange(Bot bot, int landmine[2]);
int directiontoint(char direction[5], char directions[8][3]);
void deployweapons(Bot *bot, Bot *enemy, int bullets[MAXWEAPONS][3], int missiles[MAXWEAPONS][3], int landmines[MAXWEAPONS][2], char directions[8][3]);
void cleararena(char arena[10][11]);

int main()
{
    FILE *fp;
    Bot b1, b2;
    int bot1, bot2, bot1bouts, bot2bouts;
    int bout, round, loop, totalprojectiles, dx, dy;
    char bots[NUMBOTS][MAXFILENAMESIZE]=
    {
        "./donowt           ",
        "php -f huggybot.php"
    };
    char directions[8][3]={"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    char openstring[5000], argumentstring[4000], bot1string[6], bot2string[6];
    int matcheswon[NUMBOTS],boutswon[NUMBOTS];
    int missiles[MAXWEAPONS][3];
    int bullets[MAXWEAPONS][3];
    int landmines[MAXWEAPONS][2];
    int paralyzedturnsremaining=0;
    bool bot1moved;
    char arena[10][11];
    char projectiles[300][10];

    memset(missiles, -1, sizeof(missiles));
    memset(bullets, -1, sizeof(bullets));
    memset(landmines, -1, sizeof(landmines));
    for(loop=0;loop<NUMBOTS;loop++)
    {
        matcheswon[loop]=0;
        boutswon[loop]=0;
    }

    srand(time(NULL));

    for(bot1=0;bot1<NUMBOTS-1;bot1++)
    {
        for(bot2=bot1+1;bot2<NUMBOTS;bot2++)
        {
            bot1bouts=bot2bouts=0;
            printf("%s vs %s ",bots[bot1],bots[bot2]);
            for(bout=0;bout<BOUTSPERMATCH;bout++)
            {
                printf("%d ",bout);
                //setup the arena for the bout
                b1.x=0;
                b2.x=9;
                b1.y=rand()%10;
                b2.y=rand()%10;
                b1.energy=b2.energy=10;
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
                    // send it and get the command back
                    fp=popen(openstring, "r");
                    fgets(b1.cmd, 5, fp);
                    fflush(NULL);
                    pclose(fp);

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
                    // send it and get the command back
                    fp=popen(openstring, "r");
                    fgets(b2.cmd, 5, fp);
                    fflush(NULL);
                    pclose(fp);

                    if(DISPLAYBOUTS)
                    {
                        arena[b1.y][b1.x]='A';
                        arena[b2.y][b2.x]='B';
                        printf("\033c");
                        printf("Round: %d\n", round);
                        printf("%s", arena);
                        sprintf(bot1string, "A %d\n", b1.energy);
                        sprintf(bot2string, "B %d\n", b2.energy);
                    }

                    //do bot movement phase
                    if(paralyzedturnsremaining==0)
                    {
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
                    }
                    else
                    {
                        paralyzedturnsremaining-=1;
                    }
                    //do weapons firing phase
                    if(strcmp(b1.cmd, "P")==0 || strcmp(b2.cmd, "P")==0)
                    {
                        paralyzedturnsremaining=2;
                    }
                    deployweapons(&b1, &b2, bullets, missiles, landmines, directions);
                    deployweapons(&b2, &b1, bullets, missiles, landmines, directions);
                    //do weapons movement phase
                    int moves;
                    for(loop=0;loop<MAXWEAPONS;loop++)
                    {
                        dx=dy=0;
                        if(bullets[loop][0]!= -1)
                        {
                            dx=getxmove(directions[bullets[loop][2]]);
                            dy=getymove(directions[bullets[loop][2]]);
                            for(moves=0;moves<3;moves++)
                            {
                                if(newposinbounds(bullets[loop][0], bullets[loop][1], dx, dy))
                                {
                                    bullets[loop][0]+=dx;
                                    bullets[loop][1]+=dy;
                                    if(directhit(b1, bullets[loop]))
                                    {
                                        b1.energy-=1;
                                        bullets[loop][0]= -1;
                                        bullets[loop][1]= -1;
                                        bullets[loop][2]= -1;
                                    }
                                    if(directhit(b2, bullets[loop]))
                                    {
                                        b2.energy-=1;
                                        bullets[loop][0]= -1;
                                        bullets[loop][1]= -1;
                                        bullets[loop][2]= -1;
                                    }
                                }
                                else
                                {
                                    bullets[loop][0]= -1;
                                    bullets[loop][1]= -1;
                                    bullets[loop][2]= -1;
                                    dx=dy=0;
                                }
                            }
                        }
                    };
                    for(loop=0;loop<MAXWEAPONS;loop++)
                    {
                        dx=dy=0;
                        if(missiles[loop][0]!= -1)
                        {
                            dx=getxmove(directions[missiles[loop][2]]);
                            dy=getymove(directions[missiles[loop][2]]);
                            for(moves=0;moves<2;moves++)
                            {
                                if(newposinbounds(missiles[loop][0], missiles[loop][1], dx, dy))
                                {
                                    missiles[loop][0]+=dx;
                                    missiles[loop][1]+=dy;
                                    if(directhit(b1, missiles[loop]))
                                    {
                                        b1.energy-=3;
                                        missiles[loop][0]= -1;
                                        missiles[loop][1]= -1;
                                        missiles[loop][2]= -1;
                                    }
                                    if(directhit(b2, missiles[loop]))
                                    {
                                        b2.energy-=3;
                                        missiles[loop][0]= -1;
                                        missiles[loop][1]= -1;
                                        missiles[loop][2]= -1;
                                    }
                                }
                                else
                                {
                                    if(inshrapnelrange(b1, missiles[loop]))
                                    {
                                        b1.energy-=1;
                                    }
                                    if(inshrapnelrange(b2, missiles[loop]))
                                    {
                                        b2.energy-=1;
                                    }
                                    missiles[loop][0]= -1;
                                    missiles[loop][1]= -1;
                                    missiles[loop][2]= -1;
                                    dx=dy=0;
                                }
                            }
                        }
                    }
                    //check if there's a winner
                    if(b1.energy<1 || b2.energy<1)
                    {
                        round=ROUNDSPERBOUT;
                    }
                }
                // who has won the bout
                if(b1.energy<b2.energy)
                {
                    bot1bouts+=1;
                    boutswon[bot1]+=1;
                }
                else if(b2.energy<b1.energy)
                {
                    bot2bouts+=1;
                    boutswon[bot2]+=1;
                }
            }
            if(bot1bouts>bot2bouts)
            {
                matcheswon[bot1]+=1;
            }
            else if(bot2bouts>bot1bouts)
            {
                matcheswon[bot2]+=1;
            }
            printf("\n");
        }
    }
    // output final scores
    printf("\nResults:\n");
    printf("Bot\t\t\tMatches\tBouts\n");
    for(loop=0;loop<NUMBOTS;loop++)
    {
        printf("%s\t%d\t%d\n", bots[loop], matcheswon[loop], boutswon[loop]);
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
    return (bot.x==landmine[0] && bot.y==landmine[1]);
}
int landminecollision(int landmine1[2], int landmine2[2])
{
    return (abs(landmine1[1]-landmine2[1])<2 && abs(landmine1[0]-landmine2[0])<2);
}
int inshrapnelrange(Bot bot, int landmine[2])
{
    return (abs(bot.x-landmine[0])<2 && abs(bot.y-landmine[1])<2);
}
int directiontoint(char direction[5], char directions[8][3])
{
    int loop,returnval=8;
    for(loop=0;loop<8;loop++)
    {
        if(strcmp(directions[loop], direction)==0)
            returnval=loop;
    }
    return returnval;
}
void deployweapons(Bot *bot, Bot *enemy, int bullets[MAXWEAPONS][3], int missiles[MAXWEAPONS][3], int landmines[MAXWEAPONS][2], char directions[8][3])
{
    int loop;
    if(strlen(bot->cmd)>2)
    {
        if(bot->cmd[0]=='B')
        {
            int weaponslot=0;
            while(bullets[weaponslot][0]!= -1)
                weaponslot+=1;
            bullets[weaponslot][0]=bot->x;
            bullets[weaponslot][1]=bot->y;
            bullets[weaponslot][2]=directiontoint(bot->cmd+2, directions);
            if(bullets[weaponslot][2]>7)
            {
                // direction wasn't recognized so clear the weapon
                bullets[weaponslot][0]= -1;
                bullets[weaponslot][1]= -1;
                bullets[weaponslot][2]= -1;
            }
        }
        if(bot->cmd[0]=='M')
        {
            int weaponslot=0;
            while(missiles[weaponslot][0]!= -1)
                weaponslot+=1;
            missiles[weaponslot][0]=bot->x;
            missiles[weaponslot][1]=bot->y;
            missiles[weaponslot][2]=directiontoint(bot->cmd+2, directions);
            if(missiles[weaponslot][2]>7)
            {
                // direction wasn't recognized so clear the weapon
                missiles[weaponslot][0]= -1;
                missiles[weaponslot][1]= -1;
                missiles[weaponslot][2]= -1;
            }
        }
        if(bot->cmd[0]=='L')
        {
            int weaponslot=0;
            while(landmines[weaponslot][0]!= -1)
                weaponslot+=1;
            if(newposinbounds(bot->x, bot->y, getxmove(bot->cmd+2), getymove(bot->cmd+2)))
            {
                landmines[weaponslot][0]=bot->x+getxmove(bot->cmd+2);
                landmines[weaponslot][1]=bot->y+getymove(bot->cmd+2);

                //check for landmine hits
                for(loop=0;loop<MAXWEAPONS;loop++)
                {
                    if(landmines[loop][0]!= -1)
                    {
                        if(landminecollision(landmines[weaponslot], landmines[loop]) && weaponslot!=loop)
                        {
                            if(inshrapnelrange(*bot, landmines[loop]))
                            {
                                bot->energy-=1;
                            }
                            if(inshrapnelrange(*enemy, landmines[loop]))
                            {
                                enemy->energy-=1;
                            }
                            landmines[loop][0]= -1;
                            landmines[loop][1]= -1;
                            landmines[weaponslot][0]= -1;
                            landmines[weaponslot][1]= -1;
                        }
                    }
                }
            }
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
