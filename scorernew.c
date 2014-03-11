#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/*
 * Number of matches: "best-of-n" (must be odd)
 */
#define BOUTSPERMATCH 5


/*
 * Maximum number of rounds in each bout. Bout will end early if
 * a bot's energy is reduced to 0.
 */
#define ROUNDSPERBOUT 1000


/*
 * If true, each match will be displayed as it is processed.
 */
#define DISPLAYBOUTS true


/*
 * Limitation on path lengths to bots.
 */
#define MAXFILENAMESIZE 100


/*
 * This is the maximum number of bullets, missiles, and landmines that
 * can exist at the same time. Each count is independent.
 */
#define MAXWEAPONS 100



/*
 * Internal representation of a bot. Note that the path
 * to the bot's executable is not included here, but tracked
 * separately in main().
 */
typedef struct
{
  int x, y, energy;
  char cmd[5];
} Bot;



/*
 * Headings are stored internally as numbers 0-7, but used externally
 * as headings: "N", "NE", "E", "SE", "S", "SW", "W", and "NW".
 * These functions work with headings, differentials, and positions.
 */
const char* num2heading(int);
int heading2num(char *);
int heading2dx(int);
int heading2dy(int);
int newposinbounds(int, int, int, int);


/*
 * These functions manipulate bots and the arena.
 */
void deploy_weapons(Bot *, Bot *, int [][3], int *, int [][3], int *, int [][2], int *);
void clear_arena(char arena[10][11]);
void make_cl(char *output, char arena[][11], char projectiles[][10], int cnt_projectiles, char* path, Bot bot1, Bot bot2);
bool move_bot(Bot *bot, Bot *other);


/*
 * These functions manipulate the array of bullets in-flight.
 */
void add_bullet(int [][3], int *, int, int, int);
void del_bullet(int [][3], int *, int);
int hit_bullet(Bot, int [][3], int);


/*
 * Ditto missiles in-flight.
 */
void add_missile(int [][3], int *, int, int, int);
void del_missile(int [][3], int *, int);
int hit_missile(Bot, int [][3], int);
int flak_missile(Bot, int [][3], int);


/*
 * Ditto landmines (not in-flight).
 */
void add_landmine(int [][2], int *, int, int);
void del_landmine(int [][2], int *, int);
int hit_landmine(Bot, int [][2], int);
int flak_landmine(Bot, int [][2], int);
int collide_landmine(int, int, int [][2], int);



/*
 * Main entry point. Call with paths to bots as command line arguments.
 */
int main(int argc, char **argv)
{
  int loop;

  
  /*
   * Set up bot command-line array from our args. Only one bot? Copy it
   * into the first two spots so it can play against itself.
   */
  int numbots;
  if (argc > 2) numbots = argc-1;
  else if (argc == 2) numbots = 2;
  else
  {
    printf("Usage:\n%s <bot 1 cmd line> [bot 2 cmd line] ...\n", argv[0]);
	return 1;
  }
  
  char bots[numbots][MAXFILENAMESIZE];
  for (loop = 1; loop < argc; loop++)
  {
    strncpy(bots[loop-1], argv[loop], MAXFILENAMESIZE);
  }
  if (argc == 2) strncpy(bots[1], argv[1], MAXFILENAMESIZE);

  
  /*
   * We track individual bouts won for each bot, along with matches won.
   */
  int matcheswon[numbots];
  int boutswon[numbots];
  for(loop = 0; loop < numbots; loop++)
  {
    matcheswon[loop] = 0;
    boutswon[loop] = 0;
  }

  
  /*
   * We randomize starting positions and who gets to be bot 2 in each match.
   */
  srand(time(NULL));

  
  /*
   * Round-robin: each bot plays every other bot exactly once.
   */
  int num_matches = (numbots / 2) * (numbots - 1);
  int match = 0;
  int i;
  for(i = 0; i < numbots - 1; i++)
  {
    int j;
    for(j = i + 1; j < numbots; j++)
    {
      /*
       * Bots i and j are going to compete in a best-of-n series of matches.
       */
      int bot_i_bouts = 0;
	    int bot_j_bouts = 0;
      
      
      /*
       * bullets and missiles will contain the coordinates and directions of
       * all projectiles in flights. Non-existent entries are marked with -1
       * and the total number of bullets/missiles in existence is tracked.
       */
      int bullets[MAXWEAPONS][3];
      int num_bullets = 0;
      int missiles[MAXWEAPONS][3];
      int num_missiles = 0;
      
      
      /*
       * landmines will contain the coordinates of all landmines dropped and
       * otherwise work the same as bullets/missiles.
       */
      int landmines[MAXWEAPONS][2];
      int num_landmines = 0;
      
      
      /*
       * Make sure all our bullet/missile/landmine slots start out empty.
       */
      for (loop = 0; loop < MAXWEAPONS; loop++)
      {
        bullets[loop][0] = bullets[loop][1] = bullets[loop][2] = -1;
        missiles[loop][0] = missiles[loop][1] = missiles[loop][2] = -1;
        landmines[loop][0] = landmines[loop][1] = -1;
      }
      
      
      /*
       * If either bot fires the EMP, this variable will count down the
       * number of turns paralyzed.
       */
      int paralyzedturnsremaining=0;
      
      
      /*
       * Let the games begin...
       */
      int bout;
      for(bout = 0; bout < BOUTSPERMATCH; bout++)
      {
        /*
         * Randomly pick a bot to be bot 1, and the other to be bot 2. Bot 2
         * has a slight advantage: if both bots attempt to move to the same
         * position, bot 2 always wins.
         */
        int bot1, bot2;
        if (rand() % 2)
        {
          bot1 = i;
          bot2 = j;
        }
        else
        {
          bot1 = j;
          bot2 = i;
        }
      
      
        /*
         * Bot 1 starts at the far left, bot 2 starts at the far right.
		     * Vertical positions are random. Both bots start with max energy.
		     */
        Bot b1, b2;
        b1.x=0;
        b2.x=9;
        b1.y=rand()%10;
        b2.y=rand()%10;
        b1.energy=b2.energy=10;
		
		
		    /*
		     * The bout continues until the maximum number of rounds is met or
		     * one bot's energy is reduced to 0.
		     */
        int round;
        for(round = 0; round < ROUNDSPERBOUT; round++)
        {
          /*
           * Draw the arena to a string array. Note that missiles in-flight
           * conceal bullets in-flight, and land-mines on the ground conceal 
           * both. Each line ends with a newline character. Each projectile
           * gets its own line in the projectiles list, too, describing its
           * current position and direction.
           */
          char arena[10][11];
          clear_arena(arena);

          char projectiles[300][10];
          int totalprojectiles=0;
          
          int k = num_bullets;
          for(loop = 0; k > 0; loop++)
          {
            if(bullets[loop][0] != -1)
            {
              arena[bullets[loop][1]][bullets[loop][0]]='B';
              sprintf(projectiles[totalprojectiles], "%c %d %d %s\n", 'B', bullets[loop][0], bullets[loop][1], num2heading(bullets[loop][2]));
              totalprojectiles++;
              k--;
            }
          }

          k = num_missiles;
          for (loop = 0; k > 0; loop++)
          {
            if(missiles[loop][0]!= -1)
            {
              arena[missiles[loop][1]][missiles[loop][0]]='M';
              sprintf(projectiles[totalprojectiles], "%c %d %d %s\n", 'M', missiles[loop][0], missiles[loop][1], num2heading(missiles[loop][2]));
              totalprojectiles++;
              k--;
            }
          }
          
          k = num_landmines;
          for (loop = 0; k > 0; loop++)
          {
            if(landmines[loop][0]!= -1)
            {
              arena[landmines[loop][1]][landmines[loop][0]]='L';
              sprintf(projectiles[totalprojectiles], "%c %d %d\n", 'L', landmines[loop][0], landmines[loop][1]);
              totalprojectiles++;
              k--;
            }            
          }
          
          
          /*
           * Call the first bot and get its response.
           */
          char cl1[5000];
          make_cl(cl1, arena, projectiles, totalprojectiles, bots[bot1], b1, b2);
          FILE *fp1 = popen(cl1, "r");
          fgets(b1.cmd, 5, fp1);
          fflush(NULL);
          pclose(fp1);

          
          /*
           * Call the second bot and get its response.
           */
          char cl2[5000];
          make_cl(cl2, arena, projectiles, totalprojectiles, bots[bot2], b2, b1);
          FILE *fp2 = popen(cl2, "r");
          fgets(b2.cmd, 5, fp2);
          fflush(NULL);
          pclose(fp2);
          
          
          /*
           * Optional pretty printing.
           */
          if(DISPLAYBOUTS)
          {
            printf("\033c");

            printf("Match %d of %d\n", match+1, num_matches);
            printf("A: %s (%d match wins, %d bout wins, %d energy)\n", bots[bot1], matcheswon[bot1], boutswon[bot1], b1.energy);
            printf("B: %s (%d match wins, %d bout wins, %d energy)\n", bots[bot2], matcheswon[bot2], boutswon[bot2], b2.energy);
            printf("Bout %d of %d\n", bout+1, BOUTSPERMATCH);
            printf("Round %d\n\n", round+1);

            arena[b1.y][b1.x]='A';
            arena[b2.y][b2.x]='B';
            printf("%s\n", arena);
          }
          
          /*
           * Process bot movement first, assuming they are not still paralyzed
           * by a previous EMP. In case of a conflict, bot 2 gets to move.
           */
          if(paralyzedturnsremaining == 0)
          {
            /*
             * Move bot 1, then bot 2, then, if bot 1 had previously been
             * blocked by bot 2, try moving it again.
             */
            bool moved = move_bot(&b1, &b2);
            move_bot(&b2, &b1);
            if (!moved) move_bot(&b1, &b2);
            
            
            /*
             * Loop through each landmine and check if either bot just moved
             * on top of it.
             */
            k = num_landmines;
            for(loop = 0; k > 0; loop++)
            {
              if(landmines[loop][0] != -1)
              {
                /*
                 * Bot 1 is standing on this landmine?
                 */
                if(hit_landmine(b1, landmines, loop))
                {
                  /*
                   * Bomb went off, bot 1 takes the brunt of the damage.
                   */
                  b1.energy -= 2;
                  
                  /*
                   * Did bot 2 get hit by shrapnel?
                   */
                  if(flak_landmine(b2, landmines, loop))
                  {
                    b2.energy-=1;
                  }
                  
                  /*
                   * This landmine is gone now.
                   */
                  del_landmine(landmines, &num_landmines, loop);
                }
                
                /*
                 * Bot 2 is standing on this landmine?
                 */
                else if(hit_landmine(b2, landmines, loop))
                {
                  /*
                   * Bomb went off, bot 2 takes the brunt of the damage.
                   */
                  b2.energy-=2;
                  
                  
                  /*
                   * Did bot 2 get hit by shrapnel?
                   */
                  if(flak_landmine(b1, landmines, loop))
                  {
                    b1.energy-=1;
                  }
                  
                  /*
                   * This landmine is gone now.
                   */
                  del_landmine(landmines, &num_landmines, loop);
                }
                
                k--;
              }
            }
          }
          else
          {
            paralyzedturnsremaining-=1;
          }
          
          
          /*
           * The bout might be over right now, if one or both bots were
           * hit by a landmine/shrapnel. If so, we shouldn't let them fire
           * any more shots.
           */
          if (b1.energy < 1 || b2.energy < 1) break;
          
          
          /*
           * If either (or both!) bots fired the EMP, both bots will be
           * unable to move for the next two rounds. See the -if- check
           * above. NEW - bot firing EMP loses one energy point.
           */
           if (strcmp(b1.cmd, "P") == 0)
           {
             paralyzedturnsremaining = 2;
             b1.energy--;
           }
           else if(strcmp(b2.cmd, "P") == 0)
           {
             paralyzedturnsremaining = 2;
             b2.energy--;
           }
          
          
          /*
           * Otherwise, fire whatever weapons the bots selected.
           */
          else
          {
            deploy_weapons(&b1, &b2, bullets, &num_bullets, missiles, &num_missiles, landmines, &num_landmines);
            deploy_weapons(&b2, &b1, bullets, &num_bullets, missiles, &num_missiles, landmines, &num_landmines);
          }
          
          
          /*
           * Move all bullets three places along their trajectories.
           */
          k = num_bullets;
          for(loop = 0; k > 0; loop++)
          {
            if(bullets[loop][0] != -1)
            {
              int dx = heading2dx(bullets[loop][2]);
              int dy = heading2dy(bullets[loop][2]);
              int moves;
              for(moves = 0; moves < 3; moves++)
              {
                /*
                 * If the bullet will still be in-bounds, move it.
                 */
                if(newposinbounds(bullets[loop][0], bullets[loop][1], dx, dy))
                {
                  bullets[loop][0] += dx;
                  bullets[loop][1] += dy;
                  
                  /*
                   * Was bot 1 hit by this bullet? If so, remove it.
                   */
                  if (hit_bullet(b1, bullets, loop))
                  {
                    b1.energy--;
                    del_bullet(bullets, &num_bullets, loop);
                  }
                  
                  /*
                   * How about bot 2?
                   */
                  else if (hit_bullet(b2, bullets, loop))
                  {
                    b2.energy--;
                    del_bullet(bullets, &num_bullets, loop);
                  }
                }
                
                /*
                 * If the bullet has reached an outer wall, remove it.
                 */
                else
                {
                  del_bullet(bullets, &num_bullets, loop);
                }
              }
              
              k--;
            }
          };
          
          
          /*
           * Move all missiles two places along their trajectories.
           */
          k = num_missiles;
          for(loop = 0; k > 0; loop++)
          {
            if(missiles[loop][0] != -1)
            {
              int dx = heading2dx(missiles[loop][2]);
              int dy = heading2dy(missiles[loop][2]);
              
              /*
               * If the missile will still be in-bounds, move it.
               */
              int moves;
              for(moves=0;moves<2;moves++)
              {
                bool removed = false;
                
                if(newposinbounds(missiles[loop][0], missiles[loop][1], dx, dy))
                {
                  missiles[loop][0] += dx;
                  missiles[loop][1] += dy;


                  /*
                   * Was bot 1 hit by this missile? If so, note that it should
                   * be removed.
                   */
                  if(hit_missile(b1, missiles, loop))
                  {
                    b1.energy -= 3;
                    removed = true;
                  }
                  
                  
                  /*
                   * How about bot 2?
                   */
                  else if(hit_missile(b2, missiles, loop))
                  {
                    b2.energy -= 3;
                    removed = true;
                  }
                }
                
                /*
                 * If the missile has reached an outer wall, note that it
                 * should be removed.
                 */
                else
                {
                  removed = true;
                }
                
                
                /*
                 * If a missile is to be removed, it causes shrapnel damage
                 * first.
                 */
                if (removed)
                {
                  /*
                   * Is bot 1 catching flak?
                   */
                  if(flak_missile(b1, missiles, loop))
                  {
                    b1.energy--;
                  }
                  
                  
                  /*
                   * Is bot 2 catching flak?
                   */
                  if(flak_missile(b2, missiles, loop))
                  {
                    b2.energy--;
                  }
                  
                  
                  /*
                   * Remove the missile.
                   */
                  del_missile(missiles, &num_missiles, loop);
                }
              }
              
              k--;
            }
          }

          
          /*
           * If either bot is out of energy, the bout is over.
           */
          if (b1.energy < 1 || b2.energy < 1) break;
        }
        
        
        /*
         * The bout is over. Update the winner's stats, if we have one.
         */
        if(b1.energy > b2.energy)
        {
          if (bot1 == i) bot_i_bouts++;
          else bot_j_bouts++;
          boutswon[bot1]++;
        }
        else if(b2.energy > b1.energy)
        {
          if (bot2 == i) bot_i_bouts++;
          else bot_j_bouts++;
          boutswon[bot2]++;
        }
      }
      
      
      /*
       * The match is over. Update the winner's stats, if we have one.
       */
      if(bot_i_bouts > bot_j_bouts)
      {
        matcheswon[i]++;
      }
      else if(bot_i_bouts < bot_j_bouts)
      {
        matcheswon[j]++;
      }
    }
  }

  
  /*
   * The competition is over. Display the final results.
   */
  printf("\nResults:\n");
  printf("Bot\t\t\tMatches\tBouts\n");
  for(loop = 0; loop < numbots; loop++)
  {
    printf("%s\t%d\t%d\n", bots[loop], matcheswon[loop], boutswon[loop]);
  }
}



/*
 * Get alphabetical heading from numerical value.
 *
 * Takes:
 *   dir: 0-7
 *
 * Returns:
 *   "NE", "N", "NW", "SE", "SW", "W", or "NW"
 */
const char* num2heading(int dir)
{
  if (dir == 0)
    return "N";
  else if (dir == 1)
    return "NE";
  else if (dir == 2)
    return "E";
  else if (dir == 3)
    return "SE";
  else if (dir == 4)
    return "S";
  else if (dir == 5)
    return "SW";
  else if (dir == 6)
    return "W";
  else if (dir == 7)
    return "NW";
  else
    return "";
}



/*
 * Get numerical value from alphabetical heading.
 *
 * Takes:
 *   cmd: "NE", "E", "SE", "SW", "W", or "NW"
 *
 * Returns:
 *   0-7
 */
int heading2num(char *cmd)
{
  if (strcmp(cmd, "N") == 0)
    return 0;
  if(strcmp(cmd, "NE")==0)
    return 1;
  else if(strcmp(cmd, "E")==0)
    return 2;
  else if(strcmp(cmd, "SE")==0)
    return 3;
  else if (strcmp(cmd, "S") == 0)
    return 4;
  else if(strcmp(cmd, "SW")==0)
    return 5;
  else if(strcmp(cmd, "W")==0)
    return 6;
  else if(strcmp(cmd, "NW")==0)
    return 7;
  else
    return -1;
}
    


/*
 * Get x-coordinate differential from numerical heading.
 *
 * Takes:
 *   dir: 0-7
 *
 * Returns:
 *   -1, 0, or 1
 */
int heading2dx(int dir)
{
  if (dir == 0)      return 0;
  else if (dir == 1) return 1;
  else if (dir == 2) return 1;
  else if (dir == 3) return 1;
  else if (dir == 4) return 0;
  else if (dir == 5) return -1;
  else if (dir == 6) return -1;
  else if (dir == 7) return -1;
  else return 0;
}



/*
 * Get y-coordinate differential from numerical heading.
 *
 * Takes:
 *   dir: 0-7
 *
 * Returns:
 *   -1, 0, or 1
 */
int heading2dy(int dir)
{
  if (dir == 0)      return -1;
  else if (dir == 1) return -1;
  else if (dir == 2) return 0;
  else if (dir == 3) return 1;
  else if (dir == 4) return 1;
  else if (dir == 5) return 1;
  else if (dir == 6) return 0;
  else if (dir == 7) return -1;
  else return 0;
}



/*
 * Determine if a new position is within the bounds of the arena.
 *
 * Takes:
 *   oldx, oldy: current position
 *   dx, dy: differential
 *
 * Returns:
 *   0 or 1
 */
int newposinbounds(int oldx, int oldy, int dx, int dy)
{
  return (oldx+dx>=0 && oldx+dx<10 && oldy+dy>=0 && oldy+dy<10);
}



/*
 * Execute a bot's weapon command.
 *
 * Takes:
 *   bot: Pointer to the bot who is executing
 *   other: Pointer to the other bot
 *   bullets: bullet array
 *   num_bullets: pointer to the number of bullets in the array
 *   missiles: missile array
 *   num_missiles: pointer to the number of missiles in the array
 *   landmines: landmine array
 *   num_landmines: pointer to the number of landmines in the array
 *
 * Returns:
 *   Modifies bot and other in-place, as well as bullets, missiles, and
 *   landmines arrays and their counts.
 */
void deploy_weapons(Bot *bot, Bot *other, int bullets[][3], int *num_bullets, int missiles[][3], int *num_missiles, int landmines[][2], int *num_landmines)
{
  /*
   * Bot command string must be at least three characters.
   */
  if (strlen(bot->cmd) < 3) return;
  

  /*
   * Firing a bullet? Add it to the bullet array.
   */
  if(bot->cmd[0] == 'B')
  {
    int dir = heading2num(bot->cmd + 2);
    if (dir != -1) add_bullet(bullets, num_bullets, bot->x, bot->y, dir);
  }
  
  
  /*
   * Firing a missile? Add it to the missile array.
   */
  else if (bot->cmd[0] == 'M')
  {
    int dir = heading2num(bot->cmd + 2);
    if (dir != -1) add_missile(missiles, num_missiles, bot->x, bot->y, dir);
  }
  
  
  /*
   * Dropping a landmine? More work to do...
   */
 else if (bot->cmd[0] == 'L')
 {
   int dir = heading2num(bot->cmd + 2);
   if (dir != -1)
   {
     /*
      * Make sure we're dropping it in-bounds.
      */
      if (!newposinbounds(bot->x, bot->y, heading2dx(dir), heading2dy(dir))) return;
      
      
      /*
       * So where are we dropping it?
       */
      int x = bot->x + heading2dx(dir);
      int y = bot->y + heading2dy(dir);
      
      
      /*
       * Are we dropping it onto an existing mine?
       */
      int k = *(num_landmines);
      int i;
      for (i = 0; k > 0; i++)
      {
        if (landmines[i][0] != -1)
        {
          if (collide_landmine(x, y, landmines, i))
          {
            /*
             * Both landmines will explode. We can't be directly hit, since
             * we're the one that deployed the mine, nor can the other bot,
             * since he would have set off the existing mine when we stepped
             * on it, but we can both be caught in the blast.
             */
            if (flak_landmine(*bot, landmines, i))
            {
              bot->energy--;
            }
            if (flak_landmine(*other, landmines, i))
            {
              other->energy--;
            }
            
            /*
             * Remove the mine it landed on and bail out - won't continue to
             * place our new mine.
             */
            del_landmine(landmines, num_landmines, i);
            return;
          }
          k--;
        }
      }
      
      
      /*
       * Add the new mine to the landmine array.
       */
      add_landmine(landmines, num_landmines, x, y);
    }
  }
}



/*
 * Initialize an arena array.
 *
 * Takes:
 *   arena: Arena array to be initialized.
 *
 * Returns:
 *   Nothing
 */
void clear_arena(char arena[10][11])
{
  int loop;
  memset(arena, '.', 110);
  for(loop=0;loop<10;loop++)
  {
    arena[loop][10]='\n';
  }
}



/*
 * Make the command line to be sent to a bot.
 *
 * Takes:
 *   output: pointer to the destination where the command line
*    string should be written.
 *   arena: Arena array
 *   projectiles: Projectile strings
 *   cnt_projectiles: Number of projectile strings
 *   bot1: The bot for whom this command line argument is intended
 *   bot2: The first bot's opponent
 *
 * Returns:
 *   Nothing; modifies output in-place
 */
void make_cl(char *output, char arena[][11], char projectiles[][10], int cnt_projectiles, char* path, Bot bot1, Bot bot2)
{
  /*
   * Write the first bot as a "Y" and the second bot as an "X" in the arena,
   * remembering what was there originally.
   */
  char at_y = arena[bot1.y][bot1.x];
  char at_x = arena[bot2.y][bot2.x];
  arena[bot1.y][bot1.x]='Y';
  arena[bot2.y][bot2.x]='X';

  
  /*
   * Create two more lines listing each bot's energy.
   */
  char energy[14];
  sprintf(energy, "Y %d\nX %d\n", bot1.energy, bot2.energy);

  
  /*
   * Concatenate the arena, the energy lines, and all projectile lines with
   * bot 1's command path to form the output to be sent to bot 1. Surrounding
   * quotes are needed for the argument.
   */
  strcpy(output, path);
  strcat(output, " '");
  strncat(output, *arena, 10*11);
  strcat(output, energy);
  int loop;
  for(loop = 0; loop < cnt_projectiles; loop++)
  {
    strcat(output, projectiles[loop]);
  }
  strcat(output, "'");
  
  
  /*
   * Restore the arena.
   */
  arena[bot1.y][bot1.x] = at_y;
  arena[bot2.y][bot2.x] = at_x;
}



/*
 * Move a bot, if not blocked by another bot.
 *
 * Takes:
 *   bot: Pointer to the bot to move
 *   other: Pointer to the other bot
 *
 * Returns:
 *   bool: indicates whether or not the bot was successfully moved
 *         (Returns true if the bot declined to move.)
 */
bool move_bot(Bot *bot, Bot *other)
{
  int dx = heading2dx(heading2num(bot->cmd));
  int dy = heading2dy(heading2num(bot->cmd));
  
  if(newposinbounds(bot->x, bot->y, dx, dy))
  {
    if(   !(bot->x + dx == other->x)
       || !(bot->y + dy == other->y))
    {
      bot->x += dx;
      bot->y += dy;
      return true;
    }
  }

  return false;
}



/*
 * Add a new bullet to a bullets array.
 *
 * Takes:
 *   bullets: bullets array
 *   num_bullets: pointer to the number of bullets currently in the array
 *   x, y, d: Location and direction of the new bullet.
 *
 * Returns:
 *   Nothing
 */
void add_bullet(int bullets[][3], int *num_bullets, int x, int y, int d)
{
  if (*(num_bullets) == MAXWEAPONS) return; // Damn!
  
  int i;
  for (i == 0; i < MAXWEAPONS; i++)
  {
    if (bullets[i][0] == -1)
    {
      bullets[i][0] = x;
      bullets[i][1] = y;
      bullets[i][2] = d;
      (*num_bullets)++;
      return;
    }
  }
}



/*
 * Remove a bullet from the bullets array.
 *
 * Takes:
 *   bullets: bullets array
 *   num_bullets: pointer to the number of bullets currently in the array
 *   index: Index in the array of the bullet being removed.
 *
 * Returns:
 *   Nothing
 */
void del_bullet(int bullets[][3], int *num_bullets, int index)
{
  bullets[index][0]= -1;
  bullets[index][1]= -1;
  bullets[index][2]= -1;
  (*num_bullets)--;
}



/*
 * Determine if a bot is collocated with a bullet.
 *
 * Takes:
 *   bot: bot to check
 *   bullets: bullet array
 *   index: index into the bullet array to check
 *
 * Returns:
 *   0 or 1
 */
int hit_bullet(Bot bot, int bullets[][3], int index)
{
  return (   bot.x == bullets[index][0]
          && bot.y == bullets[index][1]);
}



/*
 * Add a new missile to a missiles array.
 *
 * Takes:
 *   missiles: missiles array
 *   num_missiles: pointer to the number of missiles currently in the array
 *   x, y, d: Location and direction of the new missile.
 *
 * Returns:
 *   Nothing
 */
void add_missile(int missiles[][3], int *num_missiles, int x, int y, int d)
{
  if (*(num_missiles) == MAXWEAPONS) return; // Damn!
  
  int i;
  for (i == 0; i < MAXWEAPONS; i++)
  {
    if (missiles[i][0] == -1)
    {
      missiles[i][0] = x;
      missiles[i][1] = y;
      missiles[i][2] = d;
      (*num_missiles)++;
      return;
    }
  }
}



/*
 * Remove a missile from the missiles array.
 *
 * Takes:
 *   missiles: missiles array
 *   num_missiles: pointer to the number of missiles currently in the array
 *   index: Index in the array of the missile being removed.
 *
 * Returns:
 *   Nothing
 */
void del_missile(int missiles[][3], int *num_missiles, int index)
{
  missiles[index][0]= -1;
  missiles[index][1]= -1;
  missiles[index][2]= -1;
  (*num_missiles)--;
}



/*
 * Determine if a bot is collocated with a missile.
 *
 * Takes:
 *   bot: bot to check
 *   missiles: missile array
 *   index: index into the missile array to check
 *
 * Returns:
 *   0 or 1
 */
int hit_missile(Bot bot, int missiles[][3], int index)
{
  return (   bot.x == missiles[index][0]
          && bot.y == missiles[index][1]);
}



/*
 * Determine if a bot is standing within flak-range of a missile.
 *
 * Takes:
 *   bot: bot to check
 *   missiles: missiles array
 *   index: index into the missiles array to check
 *
 * Returns:
 *   0 or 1
 */
int flak_missile(Bot bot, int missiles[][3], int index)
{
  return (   abs(bot.x - missiles[index][0]) < 2
          && abs(bot.y - missiles[index][1]) < 2);
}



/*
 * Add a new landmine to a landmines array.
 *
 * Takes:
 *   landmines: landmines array
 *   num_landmines: pointer to the number of landmines currently in the array
 *   x, y: Location of the new landmine.
 *
 * Returns:
 *   Nothing
 */
void add_landmine(int landmines[][2], int *num_landmines, int x, int y)
{
  if (*(num_landmines) == MAXWEAPONS) return; // Damn!
  
  int i;
  for (i = 0; i < MAXWEAPONS; i++)
  {
    if (landmines[i][0] == -1)
    {
      landmines[i][0] = x;
      landmines[i][1] = y;
      (*num_landmines)++;
      return;
    }
  }
}



/*
 * Remove a landmine from the landmines array.
 *
 * Takes:
 *   landmines: landmines array
 *   num_landmines: pointer to the number of landmines currently in the array
 *   index: Index in the array of the landmine being removed.
 *
 * Returns:
 *   Nothing
 */
void del_landmine(int landmines[][2], int *num_landmines, int index)
{
  landmines[index][0]= -1;
  landmines[index][1]= -1;
  (*num_landmines)--;
}


/*
 * Determine if a bot is standing on a landmine.
 *
 * Takes:
 *   bot: bot to check
 *   landmines: landmines array
 *   index: index into the landmine array to check
 *
 * Returns:
 *   0 or 1
 */
int hit_landmine(Bot bot, int landmines[][2], int index)
{
  return (   bot.x == landmines[index][0]
          && bot.y == landmines[index][1]);
}



/*
 * Determine if a bot is standing within flak-range of a landmine.
 *
 * Takes:
 *   bot: bot to check
 *   landmines: landmines array
 *   index: index into the landmine array to check
 *
 * Returns:
 *   0 or 1
 */
int flak_landmine(Bot bot, int landmines[][2], int index)
{
  return (   abs(bot.x - landmines[index][0]) < 2
          && abs(bot.y - landmines[index][1]) < 2);
}



/*
 * Determine whether a potential landmine drop position is already
 * occupied by a landmine.
 *
 * Takes:
 *   x, y: new landmine position
 *   landmines: landmine array
 *   index: index into the landmine array to check
 *
 * Returns:
 *   0 or 1
 */
int collide_landmine(int x, int y, int landmines[][2], int index)
{
  return (   x == landmines[index][0]
          && y == landmines[index][1]);
}