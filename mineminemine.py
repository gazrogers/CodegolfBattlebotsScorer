import sys
import random
from itertools import product

def getMyPos(arena):
	x=0
	y=0
	for idx, line in enumerate(arena):
		if(line.find('Y')!= -1):
			x=line.find('Y')
			y=idx
	return [x, y]

def isNearMine(pos, badstuff):
	returnval=False
	for badthing in badstuff:
		thinglist=badthing.split(" ")
		if(thinglist[0]=='L'):
			returnval=returnval or isNear(pos, map(int, thinglist[1:3]))
	return returnval

def isNear(pos1, pos2):
	return ((abs(pos1[0]-pos2[0])<2) and (abs(pos1[1]-pos2[1])<2))

def newpos(mypos, move):
	return [mypos[0]+move[0], mypos[1]+move[1]]

def inBounds(pos):
	return pos[0]<10 and pos[0]>=0 and pos[1]<10 and pos[1]>=0

def randomSafeMove(arena, badstuff):
	mypos=getMyPos(arena)
	badsquares=[mypos] #don't want to stay still
	for badthing in badstuff:
		thinglist=badthing.split(" ")
		if(thinglist[0]=='L'):
			badsquares.append(map(int, thinglist[1:3]))
	possiblemoves=list(product(range(-1, 2), repeat=2))
	possiblemoves=[list(x) for x in possiblemoves]
	safemoves=[x for x in possiblemoves if newpos(mypos, x) not in badsquares]
	safemoves=[x for x in safemoves if inBounds(newpos(mypos, x))]
	move=random.choice(safemoves)
	return (("N S"[move[1]+1])+("W E"[move[0]+1])).strip()

def randomDropMine(arena):
	mypos=getMyPos(arena)
	badsquares=[mypos] #don't want to drop a mine under myself
	possiblemoves=list(product(range(-1, 2), repeat=2))
	possiblemoves=[list(x) for x in possiblemoves]
	possiblemoves=[x for x in possiblemoves if newpos(mypos, x) not in badsquares]
	possiblemoves=[x for x in possiblemoves if inBounds(newpos(mypos, x))]
	move=random.choice(possiblemoves)
	return "L "+(("N S"[move[1]+1])+("W E"[move[0]+1])).strip()

input=sys.argv[1].splitlines()
arena=input[0:10]
energy=input[10:12]
badstuff=input[12:]

if(isNearMine(getMyPos(arena), badstuff)):
	sys.stdout.write(randomSafeMove(arena, badstuff))
else:
	sys.stdout.write(randomDropMine(arena))