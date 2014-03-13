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
 * Internal representation of a bot.
 */
typedef struct
{
  char *cl;
  int x, y, energy;
  char cmd[5];
  int recent_bouts;
  int bouts;
  int matches;
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
void exec_bot(int [][3], int, int [][3], int, int [][2], int, Bot *, Bot *, char [10*11+1]);


/*
 * These functions manipulate the array of bullets in-flight.
 */
void add_bullet(int [][3], int *, int, int, int);
void del_bullet(int [][3], int *, int);
void update_bullets(int [][3], int *, Bot *, Bot *);


/*
 * Ditto missiles in-flight.
 */
void add_missile(int [][3], int *, int, int, int);
void del_missile(int [][3], int *, int);
void update_missiles(int [][3], int *, Bot *, Bot *);


/*
 * Ditto landmines (not in-flight).
 */
void add_landmine(int [][2], int *, int, int);
void del_landmine(int [][2], int *, int);
void update_landmines(int [][2], int *, Bot *, Bot *);
int collide_landmine(int, int, int [][2], int);


/*
 * Main entry point. Call with paths to bots as command line arguments.
 */
int main(int argc, char **argv)
{
  int loop;

  
  /*
   * Set up the bot array from our args. Only one bot? Copy it into the first
   * two spots so it can play against itself.
   */
  int numbots;
  if (argc > 2) numbots = argc - 1;
  else if (argc == 2) numbots = 2;
  else
  {
    printf("Usage:\n%s <bot 1 cmd line> [bot 2 cmd line] ...\n", argv[0]);
	  return 1;
  }
  
  Bot bots[numbots];
  for (loop = 1; loop < argc; loop++)
  {
    bots[loop - 1].cl = argv[loop];
    bots[loop - 1].bouts = 0;
    bots[loop - 1].matches = 0;
  }
  if (argc == 2)
  {
    bots[1].cl = argv[1];
    bots[1].bouts = 0;
    bots[1].matches = 0;
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
       * Bot i will battle bot j in a best-of-n competition.
       */
      Bot *botI = &bots[i];
      Bot *botJ = &bots[j];
      botI->recent_bouts = 0; 
      botJ->recent_bouts = 0;
      
      /*
       * Let the games begin...
       */
      int bout;
      for(bout = 0; bout < BOUTSPERMATCH; bout++)
      {
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
         * Randomly pick a bot to be bot 1, and the other to be bot 2. Bot 2
         * has a slight advantage: if both bots attempt to move to the same
         * position, bot 2 always wins.
         */
        Bot *bot1, *bot2;
        if (rand() % 2)
        {
          bot1 = botI;
          bot2 = botJ;
        }
        else
        {
          bot1 = botJ;
          bot2 = botI;
        }
      
      
        /*
         * Bot 1 starts at the far left, bot 2 starts at the far right.
		     * Vertical positions are random. Both bots start with max energy.
		     */
        bot1->x = 0; bot1->y = rand()%10;
        bot1->energy = 10;
        
        bot2->x = 9; bot2->y = rand()%10;
        bot2->energy = 10;

        
		    /*
		     * The bout continues until the maximum number of rounds is met or
		     * one bot's energy is reduced to 0.
		     */
        int round;
        for(round = 0; round < ROUNDSPERBOUT; round++)
        {
          /*
           * Execute both bots.
           */
          char arena[10*11+1];
          exec_bot(bullets, num_bullets, missiles, num_missiles, landmines, num_landmines, bot1, bot2, arena);
          exec_bot(bullets, num_bullets, missiles, num_missiles, landmines, num_landmines, bot2, bot1, NULL);

          if(DISPLAYBOUTS)
          {
            printf("\033c");

            printf("Match %d of %d\n", match+1, num_matches);
            printf("Y: %s (%d match wins, %d bout wins, %d energy)\n", bot1->cl, bot1->matches, bot1->recent_bouts, bot1->energy);
            printf("X: %s (%d match wins, %d bout wins, %d energy)\n", bot2->cl, bot2->matches, bot2->recent_bouts, bot2->energy);
            printf("Bout %d of %d\n", bout+1, BOUTSPERMATCH);
            printf("Round %d\n\n", round+1);
            printf("%s\n", arena);
            //getchar();
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
            bool moved = move_bot(bot1, bot2);
            move_bot(bot2, bot1);
            if (!moved) move_bot(bot1, bot2);
            
            
            /*
             * Check if either bot stepped on a landmine.
             */
            update_landmines(landmines, &num_landmines, bot1, bot2);
            if (bot1->energy < 1 || bot2->energy < 1) break;
          }
          else
          {
            paralyzedturnsremaining-=1;
          }

          
          /*
           * If either (or both!) bots fired the EMP, both bots will be
           * unable to move for the next two rounds. See the -if- check
           * above. NEW - bot firing EMP loses one energy point.
           */
          if (strcmp(bot1->cmd, "P") == 0)
          {
            paralyzedturnsremaining = 2;
            bot1->energy--;
          }
          else if(strcmp(bot2->cmd, "P") == 0)
          {
            paralyzedturnsremaining = 2;
            bot2->energy--;
          }
          
          
          /*
           * Otherwise, fire whatever weapons the bots selected.
           */
          else
          {
            deploy_weapons(bot1, bot2, bullets, &num_bullets, missiles, &num_missiles, landmines, &num_landmines);
            deploy_weapons(bot2, bot1, bullets, &num_bullets, missiles, &num_missiles, landmines, &num_landmines);
          }
          
          
          /*
           * Move all bullets and missiles along their trajectories and check if either
           * bot is out of energy.
           */
          update_bullets(bullets, &num_bullets, bot1, bot2);
          update_missiles(missiles, &num_missiles, bot1, bot2);
          if (bot1->energy < 1 || bot2->energy < 1) break;
        }
        
        
        /*
         * The bout is over. Update the winner's stats, if we have one.
         */
        if(bot1->energy > bot2->energy)
        {
          bot1->recent_bouts++;
          bot1->bouts++;
        }
        else if(bot2->energy > bot1->energy)
        {
          bot2->recent_bouts++;
          bot2->bouts++;
        }
      }
      
      
      /*
       * The match is over. Update the winner's stats, if we have one.
       */
      if (botI->recent_bouts > botJ->recent_bouts) botI->matches++;
      else if(botJ->recent_bouts > botI->recent_bouts) botJ->matches++;
    }
  }

  
  /*
   * The competition is over. Display the final results.
   */
  printf("\nResults:\n");
  for(loop = 0; loop < numbots; loop++)
  {
    printf("%s: %d match wins (%d total bout wins)\n", bots[loop].cl, bots[loop].matches, bots[loop].bouts);
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
            if (   abs(bot->x - landmines[i][0]) < 2
                && abs(bot->y - landmines[i][1]) < 2)
            {
              bot->energy--;
            }
            if (   abs(other->x - landmines[i][0]) < 2
                && abs(other->y - landmines[i][1]) < 2)
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
 * Execute a bot and receive its command response.
 *
 * Takes:
 *   bullets: bullets array
 *   num_bullets: number of elements in the bullets array
 *   missiles: missiles array
 *   num_missiles: number of elements in the missiles array
 *   landmines: landmine array
 *   num_landmines: number of elements in the landmine array
 *   bot: pointer to the bot being executed
 *   other: pointer to the other bot in the match
 *   debug: if not null, a printf-able arena will be assigned.
 *
 * Returns:
 *   Writes the command response to bot
 *
 */
void exec_bot(int bullets[][3], int num_bullets, int missiles[][3], int num_missiles, int landmines[][2], int num_landmines, Bot *bot, Bot *other, char debug[10*11+1])
{
  /*
   * Build the arena string.
   */
  char arena[10][11];
  memset(arena, '.', 110);
  int i;
  for(i = 0; i < 10; i++) arena[i][10] = '\n';

  
  /*
   * Build the list of projectile strings.
   */
  char proj[MAXWEAPONS*3][10];
  int num_proj = 0;

  int k = num_bullets;
  for(i = 0; k > 0; i++)
  {
    if(bullets[i][0] != -1)
    {
      arena[bullets[i][1]][bullets[i][0]]='B';
      sprintf(proj[num_proj], "%c %d %d %s\n", 'B', bullets[i][0], bullets[i][1], num2heading(bullets[i][2]));
      num_proj++;
      k--;
    }
  }

  k = num_missiles;
  for (i = 0; k > 0; i++)
  {
    if(missiles[i][0]!= -1)
    {
      arena[missiles[i][1]][missiles[i][0]]='M';
      sprintf(proj[num_proj], "%c %d %d %s\n", 'M', missiles[i][0], missiles[i][1], num2heading(missiles[i][2]));
      num_proj++;
      k--;
    }
  }

  k = num_landmines;
  for (i = 0; k > 0; i++)
  {
    if(landmines[i][0]!= -1)
    {
      arena[landmines[i][1]][landmines[i][0]]='L';
      sprintf(proj[num_proj], "%c %d %d\n", 'L', landmines[i][0], landmines[i][1]);
      num_proj++;
      k--;
    }            
  }


  /*
   * Build the command line string for this bot.
   */
  char cl[1024*4];
  arena[bot->y][bot->x] = 'Y';
  arena[other->y][other->x] = 'X';
  char energy[14];
  sprintf(energy, "Y %d\nX %d\n", bot->energy, other->energy);
  strcpy(cl, bot->cl);
  strcat(cl, " '");
  strncat(cl, *arena, 10*11);
  strcat(cl, energy);
  for(i = 0; i < num_proj; i++) strcat(cl, proj[i]);
  strcat(cl, "'");
  

  /*
   * Execute the bot's command line string.
   */
  FILE *fp = popen(cl, "r");
  fgets(bot->cmd, 5, fp);
  fflush(NULL);
  pclose(fp);
  
  
  /*
   * Debug output.
   */
  if (debug != NULL)
  {
    strncpy(debug, *arena, 10*11);
    debug[10*11] = 0;
  }
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
  //printf("TODO: adding bullet @ %d, %d, %d (currently have %d)\n", x, y, d, *num_bullets);
  //getchar();

  if (*num_bullets == MAXWEAPONS) return; // Damn!
  
  int i;
  for (i = 0; i < MAXWEAPONS; i++)
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
  //printf("TODO: Deleting bullet @ %d (currently have %d)\n", index, *num_bullets);
  //getchar();
  bullets[index][0]= -1;
  bullets[index][1]= -1;
  bullets[index][2]= -1;
  (*num_bullets)--;
}



/*
 * Update the position of all bullets, checking for collisions with bots and walls.
 *
 * Takes:
 *   bullets: bullet array
 *   num_bullets: pointer to the number of bullets in the array
 *   bot1: Pointer to one of the bots in the match
 *   bot2: Pointer to the other bot in the match
 *
 * Returns:
 *   Nothing
 */
void update_bullets(int bullets[][3], int *num_bullets, Bot *bot1, Bot *bot2)
{
  int k = *num_bullets;
  int i;
  for(i = 0; k > 0; i++)
  {
    if(bullets[i][0] != -1)
    {
      int dx = heading2dx(bullets[i][2]);
      int dy = heading2dy(bullets[i][2]);
      int moves;
      for(moves = 0; moves < 3; moves++)
      {
        /*
         * If the bullet will still be in-bounds, move it.
         */
        if(newposinbounds(bullets[i][0], bullets[i][1], dx, dy))
        {
          bullets[i][0] += dx;
          bullets[i][1] += dy;
          
          /*
           * Was bot 1 hit by this bullet?
           */
          if (   bot1->x == bullets[i][0]
              && bot1->y == bullets[i][1])
          {
            bot1->energy--;
            del_bullet(bullets, num_bullets, i);
            break;
          }
          
          /*
           * How about bot 2?
           */
          else if (   bot2->x == bullets[i][0]
                   && bot2->y == bullets[i][1])
          {
            bot2->energy--;
            del_bullet(bullets, num_bullets, i);
            break;
          }
        }

        
        /*
         * If the bullet has reached an outer wall, remove it.
         */
        else
        {
          del_bullet(bullets, num_bullets, i);
          break;
        }
      }
      
      k--;
    }
  }
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
  for (i = 0; i < MAXWEAPONS; i++)
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
 * Update the position of all missiles, checking for collisions with bots and walls.
 *
 * Takes:
 *   missiles: missile array
 *   num_missiles: pointer to the number of missiles in the array
 *   bot1: Pointer to one of the bots in the match
 *   bot2: Pointer to the other bot in the match
 *
 * Returns:
 *   Nothing
 */
void update_missiles(int missiles[][3], int *num_missiles, Bot *bot1, Bot *bot2)
{
  int k = *num_missiles;
  int i;
  for(i = 0; k > 0; i++)
  {
    if(missiles[i][0] != -1)
    {
      int dx = heading2dx(missiles[i][2]);
      int dy = heading2dy(missiles[i][2]);
      
      /*
       * Missiles move two steps at a time.
       */
      int moves;
      for(moves = 0; moves < 2; moves++)
      {
        /*
         * Is the missile still in-bounds?
         */
        if(newposinbounds(missiles[i][0], missiles[i][1], dx, dy))
        {
          missiles[i][0] += dx;
          missiles[i][1] += dy;

          
          /*
           * Was bot 1 hit by this missile?
           */
          if (   bot1->x == missiles[i][0]
              && bot1->y == missiles[i][1])
          {
            bot1->energy -= 3;
            
            /*
             * Was bot 2 caught in the splash damage?
             */
            if (   abs(bot2->x - missiles[i][0]) < 2
                && abs(bot2->y - missiles[i][1]) < 2)
            {
              bot2->energy--;
            }
            
            del_missile(missiles, num_missiles, i);
            break;
          }
          
          
          /*
           * How about bot 2?
           */
          else if(   bot2->x == missiles[i][0]
                  && bot2->y == missiles[i][1])
          {
            bot2->energy -= 3;
            
            /*
             * Was bot 1 caught in the splash damage?
             */
            if (   abs(bot1->x - missiles[i][0]) < 2
                && abs(bot1->y - missiles[i][1]) < 2)
            {
              bot1->energy--;
            }
            
            del_missile(missiles, num_missiles, i);
            break;
          }
        }
        
        
        /*
         * Has the missile hit a wall?
         */
        else
        {
          /*
           * Where did it hit the wall?
           */
          missiles[i][0] += dx;
          missiles[i][1] += dy;
          
          /*
           * Was bot 1 caught in the splash damage?
           */
          if (   abs(bot1->x - missiles[i][0]) < 2
              && abs(bot1->y - missiles[i][1]) < 2)
          {
            bot1->energy--;
          }
          
          /*
           * Was bot 2 caught in the splash damage?
           */
          if (   abs(bot2->x - missiles[i][0]) < 2
              && abs(bot2->y - missiles[i][1]) < 2)
          {
            bot2->energy--;
          }
          
          del_missile(missiles, num_missiles, i);
          break;
        }
      }

      k--;
    }
  }
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
 * Check if a bot is standing on a landmine.
 *
 * Takes:
 *   landmines: landmine array
 *   num_landmines: pointer to the number of landmines in the array
 *   bot1: Pointer to one of the bots in the match
 *   bot2: Pointer to the other bot in the match
 *
 * Returns:
 *   Nothing
 */
void update_landmines(int landmines[][2], int *num_landmines, Bot *bot1, Bot *bot2)
{
  /*
   * Loop through each landmine and check if either bot just moved
   * on top of it.
   */
  int k = *num_landmines;
  int i;
  for(i = 0; k > 0; i++)
  {
    if(landmines[i][0] != -1)
    {
      /*
       * Bot 1 is standing on this landmine?
       */
      if(   bot1->x == landmines[i][0]
         && bot1->y == landmines[i][1])
      {
        bot1->energy -= 2;
        
        /*
         * Did bot 2 get hit by shrapnel?
         */
        if(   abs(bot2->x - landmines[i][0]) < 2
           && abs(bot2->y - landmines[i][1]) < 2)
        {
          bot2->energy-=1;
        }
        
        del_landmine(landmines, num_landmines, i);
      }
      
      
      /*
       * Bot 2 is standing on this landmine?
       */
      else if(   bot2->x == landmines[i][0]
              && bot2->y == landmines[i][1])
      {
        bot2->energy -= 2;
        
        /*
         * Did bot 1 get hit by shrapnel?
         */
        if(   abs(bot1->x - landmines[i][0]) < 2
           && abs(bot1->y - landmines[i][1]) < 2)
        {
          bot1->energy-=1;
        }
        
        del_landmine(landmines, num_landmines, i);
      }
      
      k--;
    }
  }
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
